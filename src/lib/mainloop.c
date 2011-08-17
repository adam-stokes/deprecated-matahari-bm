/*
 * Copyright (C) 2009 Andrew Beekhof <andrew@beekhof.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

#if __linux__
#include <sys/wait.h>
#include <sys/times.h>
#endif

#include "matahari/mainloop.h"
#include "matahari/logging.h"

static GHashTable *mainloop_process_table = NULL;

static gboolean
mainloop_trigger_prepare(GSource *source, gint *timeout)
{
    mainloop_trigger_t *trig = (mainloop_trigger_t *) source;
    return trig->trigger;
}

static gboolean
mainloop_trigger_check(GSource *source)
{
    mainloop_trigger_t *trig = (mainloop_trigger_t *) source;
    return trig->trigger;
}

static gboolean
mainloop_trigger_dispatch(GSource *source, GSourceFunc callback,
                          gpointer userdata)
{
    mainloop_trigger_t *trig = (mainloop_trigger_t *) source;
    trig->trigger = FALSE;

    if (callback) {
        return callback(trig->user_data);
    }
    return TRUE;
}

static GSourceFuncs mainloop_trigger_funcs = {
    mainloop_trigger_prepare,
    mainloop_trigger_check,
    mainloop_trigger_dispatch,
    NULL
};

static mainloop_trigger_t *
mainloop_setup_trigger(GSource *source, int priority,
                       gboolean (*dispatch)(gpointer user_data),
                       gpointer userdata)
{
    mainloop_trigger_t *trigger = NULL;
    trigger = (mainloop_trigger_t *) source;

    trigger->id = 0;
    trigger->trigger = FALSE;
    trigger->user_data = userdata;

    if (dispatch) {
        g_source_set_callback(source, dispatch, trigger, NULL);
    }

    g_source_set_priority(source, priority);
    g_source_set_can_recurse(source, FALSE);

    trigger->id = g_source_attach(source, NULL);
    return trigger;
}

mainloop_trigger_t *
mainloop_add_trigger(int priority,
                     gboolean (*dispatch)(gpointer user_data),
                     gpointer userdata)
{
    GSource *source = NULL;

    MH_ASSERT(sizeof(mainloop_trigger_t) > sizeof(GSource));
    source = g_source_new(&mainloop_trigger_funcs, sizeof(mainloop_trigger_t));
    MH_ASSERT(source != NULL);

    return mainloop_setup_trigger(source, priority, dispatch, userdata);
}

void
mainloop_set_trigger(mainloop_trigger_t *source)
{
    source->trigger = TRUE;
}

gboolean
mainloop_destroy_trigger(mainloop_trigger_t *source)
{
    source->trigger = FALSE;
    if (source->id > 0) {
        g_source_remove(source->id);
    }
    return TRUE;
}

typedef struct signal_s {
        mainloop_trigger_t trigger; /**< must be first */
        void (*handler)(int sig);
        int signal;
} mainloop_signal_t;

static mainloop_signal_t *mainloop_signals[NSIG];

static gboolean
mainloop_signal_dispatch(GSource *source, GSourceFunc callback,
                         gpointer userdata)
{
    mainloop_signal_t *sig = (mainloop_signal_t *) source;
#if __linux__
    mh_info("Invoking handler for signal %d: %s",
            sig->signal, strsignal(sig->signal));
#endif
    sig->trigger.trigger = FALSE;
    if (sig->handler) {
        sig->handler(sig->signal);
    }
    return TRUE;
}

static void
mainloop_signal_handler(int sig)
{
    if (sig > 0 && sig < NSIG && mainloop_signals[sig] != NULL) {
        mainloop_set_trigger((mainloop_trigger_t *) mainloop_signals[sig]);
    }
}

static GSourceFuncs mainloop_signal_funcs = {
    mainloop_trigger_prepare,
    mainloop_trigger_check,
    mainloop_signal_dispatch,
    NULL
};

gboolean
mainloop_signal(int sig, void (*dispatch)(int sig))
{
#if __linux__
    sigset_t mask;
    struct sigaction sa = {
        .sa_handler = dispatch,
        .sa_flags = SA_RESTART,
    };
    struct sigaction old;

    if (sigemptyset(&mask) < 0) {
        mh_perror(LOG_ERR, "Call to sigemptyset failed");
        return FALSE;
    }

    sa.sa_mask = mask;

    if (sigaction(sig, &sa, &old) < 0) {
        mh_perror(LOG_ERR, "Could not install signal handler for signal %d",
                  sig);
        return FALSE;
    }

    return TRUE;
#else
    mh_perror(LOG_ERR, "Could not install signal handler for signal %d", sig);
    return FALSE;
#endif
}

gboolean
mainloop_add_signal(int sig, void (*dispatch)(int sig))
{
    GSource *source = NULL;
    int priority = G_PRIORITY_HIGH - 1;
    if (sig == SIGTERM) {
        /* TERM is higher priority than other signals,
         *   signals are higher priority than other ipc.
         * Yes, minus: smaller is "higher"
         */
        priority--;
    }

    if (sig >= NSIG || sig < 0) {
        mh_err("Signal %d is out of range", sig);
        return FALSE;

    } else if (mainloop_signals[sig] != NULL) {
        mh_err("Signal handler for %d is already installed", sig);
        return FALSE;
    }

    MH_ASSERT(sizeof(mainloop_signal_t) > sizeof(GSource));
    source = g_source_new(&mainloop_signal_funcs, sizeof(mainloop_signal_t));

    mainloop_signals[sig] = (mainloop_signal_t *) mainloop_setup_trigger(
            source, priority, NULL, NULL);
    if (mainloop_signals[sig] == NULL) {
        return FALSE;
    }

    mainloop_signals[sig]->handler = dispatch;
    mainloop_signals[sig]->signal = sig;

    if (mainloop_signal(sig, mainloop_signal_handler) == FALSE) {
        mainloop_signal_t *tmp = mainloop_signals[sig];
        mainloop_signals[sig] = NULL;

        mainloop_destroy_trigger((mainloop_trigger_t *) tmp);
        return FALSE;
    }

    return TRUE;
}

gboolean
mainloop_destroy_signal(int sig)
{
    mainloop_signal_t *tmp = NULL;

    if (sig >= NSIG || sig < 0) {
        mh_err("Signal %d is out of range", sig);
        return FALSE;

    } else if (mainloop_signal(sig, NULL) == FALSE) {
        mh_perror(LOG_ERR, "Could not uninstall signal handler for signal %d",
                  sig);
        return FALSE;

    } else if (mainloop_signals[sig] == NULL) {
        return TRUE;
    }

    tmp = mainloop_signals[sig];
    mainloop_signals[sig] = NULL;
    mainloop_destroy_trigger((mainloop_trigger_t *) tmp);
    return TRUE;
}

static gboolean
mainloop_fd_prepare(GSource *source, gint *timeout)
{
    return FALSE;
}

static gboolean
mainloop_fd_check(GSource* source)
{
    mainloop_fd_t *trig = (mainloop_fd_t *) source;
    if (trig->gpoll.revents) {
        return TRUE;
    }
    return FALSE;
}

static gboolean
mainloop_fd_dispatch(GSource *source, GSourceFunc callback, gpointer userdata)
{
    mainloop_fd_t *trig = (mainloop_fd_t *) source;
    mh_trace("%p", source);
    /*
     * Is output now unblocked?
     *
     * If so, turn off OUTPUT_EVENTS to avoid going into
     * a tight poll(2) loop.
     */
    if (trig->gpoll.revents & G_IO_OUT) {
        trig->gpoll.events &= ~G_IO_OUT;
    }

    if (trig->dispatch != NULL
        && trig->dispatch(trig->gpoll.fd, trig->user_data) == FALSE) {
        g_source_remove_poll(source, &trig->gpoll);
        g_source_unref(source); /* Really? */
        return FALSE;
    }
    return TRUE;
}

static void
mainloop_fd_destroy(GSource *source)
{
    mainloop_fd_t *trig = (mainloop_fd_t *) source;
    mh_trace("%p", source);

    if (trig->dnotify) {
        trig->dnotify(trig->user_data);
    }
}

static GSourceFuncs mainloop_fd_funcs = {
    mainloop_fd_prepare,
    mainloop_fd_check,
    mainloop_fd_dispatch,
    mainloop_fd_destroy,
};

mainloop_fd_t *
mainloop_add_fd(int priority, int fd,
                gboolean (*dispatch)(int fd, gpointer userdata),
                GDestroyNotify notify, gpointer userdata)
{
    GSource *source = NULL;
    mainloop_fd_t *fd_source = NULL;
    MH_ASSERT(sizeof(mainloop_fd_t) > sizeof(GSource));
    source = g_source_new(&mainloop_fd_funcs, sizeof(mainloop_fd_t));
    MH_ASSERT(source != NULL);

    fd_source = (mainloop_fd_t *) source;
    fd_source->id = 0;
    fd_source->gpoll.fd = fd;
    fd_source->gpoll.events = G_IO_ERR|G_IO_NVAL|G_IO_IN|G_IO_PRI|G_IO_HUP;
    fd_source->gpoll.revents = 0;

    /*
     * Normally we'd use g_source_set_callback() to specify the dispatch
     * function, but we want to supply the fd too, so we store it in
     * fd_source->dispatch instead
     */
    fd_source->dnotify = notify;
    fd_source->dispatch = dispatch;
    fd_source->user_data = userdata;

    g_source_set_priority(source, priority);
    g_source_set_can_recurse(source, FALSE);
    g_source_add_poll(source, &fd_source->gpoll);

    fd_source->id = g_source_attach(source, NULL);
    return fd_source;
}

gboolean
mainloop_destroy_fd(mainloop_fd_t *source)
{
    g_source_remove_poll((GSource *) source, &source->gpoll);
    g_source_remove(source->id);
    source->id = 0;
    g_source_unref((GSource *) source);

    return TRUE;
}

static gboolean
child_timeout_callback(gpointer p)
{
    pid_t pid = (pid_t) GPOINTER_TO_INT(p);
    mainloop_child_t *pinfo = (mainloop_child_t *) g_hash_table_lookup(
            mainloop_process_table, p);

    if (pinfo == NULL) {
        return FALSE;
    }

    pinfo->timerid = 0;
    if (pinfo->timeout) {
        mh_crit("%s process (PID %d) will not die!", pinfo->desc, (int)pid);
        return FALSE;
    }

    pinfo->timeout = TRUE;
    mh_warn("%s process (PID %d) timed out", pinfo->desc, (int)pid);

#if __linux__
    if (kill(pinfo->pid, SIGKILL) < 0) {
        if (errno == ESRCH) {
            /* Nothing left to do */
            return FALSE;
        }
        mh_perror(LOG_ERR, "kill(%d, KILL) failed", pid);
    }

    pinfo->timerid = g_timeout_add(5000, child_timeout_callback, p);
#endif
    return FALSE;
}

static void
mainloop_child_destroy(gpointer data)
{
    mainloop_child_t *p = data;

    free(p->desc);

    g_free(p);
}

/* Create/Log a new tracked process
 * To track a process group, use -pid
 */
void
mainloop_add_child(pid_t pid, int timeout, const char *desc, void * privatedata,
                   void (*callback)(mainloop_child_t *p, int status, int signo,
                                    int exitcode))
{
    mainloop_child_t *p = g_new(mainloop_child_t, 1);

    if (mainloop_process_table == NULL) {
        mainloop_process_table = g_hash_table_new_full(
            g_direct_hash, g_direct_equal, NULL, mainloop_child_destroy);
    }

    p->pid = pid;
    p->timerid = 0;
    p->timeout = FALSE;
    p->desc = strdup(desc);
    p->privatedata = privatedata;
    p->callback = callback;

    if (timeout) {
        p->timerid = g_timeout_add(
            timeout, child_timeout_callback, GINT_TO_POINTER(pid));
    }

    g_hash_table_insert(mainloop_process_table, GINT_TO_POINTER(abs(pid)), p);
}

#if __linux__
static void
child_death_dispatch(int sig)
{
    int status = 0;
    while (TRUE) {
        pid_t pid = wait3(&status, WNOHANG, NULL);
        if (pid > 0) {
            int signo = 0, exitcode = 0;

            mainloop_child_t *p = g_hash_table_lookup(mainloop_process_table,
                                                      GINT_TO_POINTER(pid));
            mh_trace("Managed process %d exited: %p", pid, p);
            if (p == NULL) {
                continue;
            }

            if (WIFEXITED(status)) {
                exitcode = WEXITSTATUS(status);
                mh_trace("Managed process %d (%s) exited with rc=%d", pid,
                         p->desc, exitcode);

            } else if (WIFSIGNALED(status)) {
                signo = WTERMSIG(status);
                mh_trace("Managed process %d (%s) exited with signal=%d", pid,
                         p->desc, signo);
            }
#ifdef WCOREDUMP
            if (WCOREDUMP(status)) {
                mh_err("Managed process %d (%s) dumped core", pid, p->desc);
            }
#endif
            if (p->timerid != 0) {
                mh_trace("Removing timer %d", p->timerid);
                g_source_remove(p->timerid);
                p->timerid = 0;
            }
            p->callback(p, status, signo, exitcode);
            g_hash_table_remove(mainloop_process_table, GINT_TO_POINTER(pid));
            mh_trace("Removed process entry for %d", pid);
            return;

        } else {
            if (errno != ECHILD) {
                mh_perror(LOG_ERR, "wait3() failed");
            }
            break;
        }
    }
}
#endif

void
mainloop_track_children(int priority)
{
    if (mainloop_process_table == NULL) {
        mainloop_process_table = g_hash_table_new_full(
            g_direct_hash, g_direct_equal, NULL, NULL/*TODO: Add destructor */);
    }

#if __linux__
    mainloop_add_signal(SIGCHLD, child_death_dispatch);
#endif
}
