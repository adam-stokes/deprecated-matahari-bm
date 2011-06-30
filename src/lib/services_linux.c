/*
 * Copyright (C) 2010 Andrew Beekhof <andrew@beekhof.net>
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

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>

#include "matahari/logging.h"
#include "matahari/mainloop.h"
#include "matahari/services.h"

#include "services_private.h"
#include "sigar.h"

static inline void
set_fd_opts(int fd, int opts)
{
    int flag;
    if ((flag = fcntl(fd, F_GETFL)) >= 0) {
        if (fcntl(fd, F_SETFL, flag | opts) < 0) {
            mh_perror(LOG_ERR, "fcntl() write failed");
        }
    } else {
        mh_perror(LOG_ERR, "fcntl() read failed");
    }
}

static gboolean
read_output(int fd, gpointer user_data)
{
    char * data = NULL;
    int rc = 0, len = 0;
    gboolean is_err = FALSE;
    svc_action_t* op = (svc_action_t *) user_data;
    char buf[500];
    static const size_t buf_read_len = sizeof(buf) - 1;

    if (fd == op->opaque->stderr_fd) {
        is_err = TRUE;
        if (op->stderr_data) {
            len = strlen(op->stderr_data);
            data = op->stderr_data;
        }
    } else if (op->stdout_data) {
        len = strlen(op->stdout_data);
        data = op->stdout_data;
    }

    do {
        rc = read(fd, buf, buf_read_len);
        if (rc > 0) {
            buf[rc] = 0;
            data = realloc(data, len + rc + 1);
            sprintf(data + len, "%s", buf);
            len += rc;

        } else if (errno != EINTR) {
            /* error or EOF
             * Cleanup happens in pipe_done()
             */
            rc = FALSE;
            break;
        }

    } while (rc == buf_read_len || rc < 0);

    if (data != NULL && is_err) {
        op->stderr_data = data;
    } else if (data != NULL) {
        op->stdout_data = data;
    }

    return rc;
}

static void
pipe_out_done(gpointer user_data)
{
    svc_action_t* op = (svc_action_t *) user_data;
    op->opaque->stdout_gsource = NULL;
    if (op->opaque->stdout_fd > STDERR_FILENO) {
        close(op->opaque->stdout_fd);
    }
    op->opaque->stdout_fd = -1;
}

static void
pipe_err_done(gpointer user_data)
{
    svc_action_t* op = (svc_action_t *) user_data;
    op->opaque->stderr_gsource = NULL;
    if (op->opaque->stderr_fd > STDERR_FILENO) {
        close(op->opaque->stderr_fd);
    }
    op->opaque->stderr_fd = -1;
}

static void
set_ocf_env(gpointer key, gpointer value, gpointer user_data)
{
    if (setenv(key, value, 1) != 0) {
        mh_err("setenv failed in raexecocf.");
    }
}

static void
set_ocf_env_with_prefix(gpointer key, gpointer value, gpointer user_data)
{
    /* TODO: Add OCF_RESKEY_ prefix to 'key' */
    char buffer[500];
    snprintf(buffer, sizeof(buffer), "OCF_RESKEY_%s", (char *) key);
    set_ocf_env(buffer, value, user_data);
}

static void
add_OCF_env_vars(svc_action_t *op)
{
    if (strcmp("ocf", op->standard) != 0) {
        return;
    }

    if (op->params) {
        g_hash_table_foreach(op->params, set_ocf_env_with_prefix, NULL);
    }

    set_ocf_env("OCF_RA_VERSION_MAJOR", "1", NULL);
    set_ocf_env("OCF_RA_VERSION_MINOR", "0", NULL);
    set_ocf_env("OCF_ROOT", OCF_ROOT, NULL);

    if (op->rsc) {
        set_ocf_env("OCF_RESOURCE_INSTANCE", op->rsc, NULL);
    }

    if (op->agent != NULL) {
        set_ocf_env("OCF_RESOURCE_TYPE", op->agent, NULL);
    }

    /* Notes: this is not added to specification yet. Sept 10,2004 */
    if (op->provider != NULL) {
        set_ocf_env("OCF_RESOURCE_PROVIDER", op->provider, NULL);
    }
}

static gboolean recurring_action_timer(gpointer data)
{
    svc_action_t *op = data;
    mh_debug("Scheduling another invokation of %s", op->id);

    /* Clean out the old result */
    free(op->stdout_data); op->stdout_data = NULL;
    free(op->stderr_data); op->stderr_data = NULL;

    services_action_async(op, NULL);
    return FALSE;
}

static void
operation_finished(mainloop_child_t *p, int status, int signo, int exitcode)
{
    char *next = NULL;
    char *offset = NULL;
    svc_action_t *op = p->privatedata;
    int recurring = 0;

    p->privatedata = NULL;
    op->status = LRM_OP_DONE;
    MH_ASSERT(op->pid == p->pid);

    if (signo) {
        if (p->timeout) {
            mh_warn("%s:%d - timed out after %dms", op->id, op->pid,
                    op->timeout);
            op->status = LRM_OP_TIMEOUT;
            op->rc = OCF_TIMEOUT;

        } else {
            mh_warn("%s:%d - terminated with signal %d", op->id, op->pid,
                    signo);
            op->status = LRM_OP_ERROR;
            op->rc = OCF_SIGNAL;
        }

    } else {
        op->rc = exitcode;
        mh_debug("%s:%d - exited with rc=%d", op->id, op->pid, exitcode);

        if (op->stdout_data) {
            next = op->stdout_data;
            do {
                offset = next;
                next = strchrnul(offset, '\n');
                mh_debug("%s:%d [ %.*s ]", op->id, op->pid,
                         (int) (next - offset), offset);
                if (next[0] != 0) {
                    next++;
                }

            } while (next != NULL && next[0] != 0);
        }

        if (op->stderr_data) {
            next = op->stderr_data;
            do {
                offset = next;
                next = strchrnul(offset, '\n');
                mh_notice("%s:%d [ %.*s ]", op->id, op->pid,
                          (int) (next - offset), offset);
                if (next[0] != 0) {
                    next++;
                }

            } while (next != NULL && next[0] != 0);
        }
    }

    if (op->interval) {
        recurring = 1;
        op->opaque->repeat_timer = g_timeout_add(op->interval,
                                                 recurring_action_timer,
                                                 (void *) op);
    }

    op->pid = 0;

    if (op->opaque->callback) {
        op->opaque->callback(op);
    }

    if (!recurring) {
        /*
         * If this is a recurring action, do not free explicitly.
         * It will get freed whenever the action gets cancelled.
         */
        services_action_free(op);
    }
}

gboolean
services_os_action_execute(svc_action_t* op, gboolean synchronous)
{
    int rc, lpc;
    int stdout_fd[2];
    int stderr_fd[2];

    if (pipe(stdout_fd) < 0) {
        mh_perror(LOG_ERR, "pipe() failed");
    }

    if (pipe(stderr_fd) < 0) {
        mh_perror(LOG_ERR, "pipe() failed");
    }

    op->pid = fork();
    switch (op->pid) {
    case -1:
        mh_perror(LOG_ERR, "fork() failed");
        close(stdout_fd[0]);
        close(stdout_fd[1]);
        close(stderr_fd[0]);
        close(stderr_fd[1]);
        return FALSE;

    case 0:                /* Child */
        /* Man: The call setpgrp() is equivalent to setpgid(0,0)
         * _and_ compiles on BSD variants too
         * need to investigate if it works the same too.
         */
        setpgid(0, 0);
        close(stdout_fd[0]);
        close(stderr_fd[0]);
        if (STDOUT_FILENO != stdout_fd[1]) {
            if (dup2(stdout_fd[1], STDOUT_FILENO) != STDOUT_FILENO) {
                mh_perror(LOG_ERR, "dup2() failed (stdout)");
            }
            close(stdout_fd[1]);
        }
        if (STDERR_FILENO != stderr_fd[1]) {
            if (dup2(stderr_fd[1], STDERR_FILENO) != STDERR_FILENO) {
                mh_perror(LOG_ERR, "dup2() failed (stderr)");
            }
            close(stderr_fd[1]);
        }

        /* close all descriptors except stdin/out/err and channels to logd */
        for (lpc = getdtablesize() - 1; lpc > STDERR_FILENO; lpc--) {
            close(lpc);
        }

        /* Setup environment correctly */
        add_OCF_env_vars(op);

        /* execute the RA */
        execvp(op->opaque->exec, op->opaque->args);

        switch (errno) { /* see execve(2) */
        case ENOENT:  /* No such file or directory */
        case EISDIR:   /* Is a directory */
            rc = OCF_NOT_INSTALLED;
            break;
        case EACCES:   /* permission denied (various errors) */
            rc = OCF_INSUFFICIENT_PRIV;
            break;
        default:
            rc = OCF_UNKNOWN_ERROR;
            break;
        }
        _exit(rc);
    }

    /* Only the parent reaches here */
    close(stdout_fd[1]);
    close(stderr_fd[1]);

    op->opaque->stdout_fd = stdout_fd[0];
    set_fd_opts(op->opaque->stdout_fd, O_NONBLOCK);

    op->opaque->stderr_fd = stderr_fd[0];
    set_fd_opts(op->opaque->stderr_fd, O_NONBLOCK);

    if (synchronous) {
        int status = 0;
        int timeout = 1 + op->timeout / 1000;
        mh_trace("Waiting for %d", op->pid);
        while (timeout > 0 && waitpid(op->pid, &status, WNOHANG) <= 0) {
            sleep(1);
            timeout--;
        }

        mh_trace("Child done: %d", op->pid);
        if (timeout == 0) {
            int killrc = sigar_proc_kill(op->pid, 9 /*SIGKILL*/);

            op->status = LRM_OP_TIMEOUT;
            mh_warn("%s:%d - timed out after %dms", op->id, op->pid,
                    op->timeout);

            if (killrc != SIGAR_OK && killrc != ESRCH) {
                mh_err("kill(%d, KILL) failed: %d", op->pid, killrc);
            }

        } else if (WIFEXITED(status)) {
            op->status = LRM_OP_DONE;
            op->rc = WEXITSTATUS(status);
            mh_err("Managed %s process %d exited with rc=%d", op->id, op->pid,
                   op->rc);

        } else if (WIFSIGNALED(status)) {
            int signo = WTERMSIG(status);
            op->status = LRM_OP_ERROR;
            mh_err("Managed %s process %d exited with signal=%d", op->id,
                   op->pid, signo);
        }
#ifdef WCOREDUMP
        if (WCOREDUMP(status)) {
            mh_err("Managed %s process %d dumped core", op->id, op->pid);
        }
#endif

        read_output(op->opaque->stdout_fd, op);
        read_output(op->opaque->stderr_fd, op);

    } else {
        mh_trace("Async waiting for %d - %s", op->pid, op->opaque->exec);
        mainloop_add_child(op->pid, op->timeout, op->id, op,
                           operation_finished);

        op->opaque->stdout_gsource = mainloop_add_fd(G_PRIORITY_LOW,
                op->opaque->stdout_fd, read_output, pipe_out_done, op);

        op->opaque->stderr_gsource = mainloop_add_fd(G_PRIORITY_LOW,
                op->opaque->stderr_fd, read_output, pipe_err_done, op);
    }

    return TRUE;
}

GList *
services_os_get_directory_list(const char *root, gboolean files)
{
    GList *list = NULL;
#if __linux__
    struct dirent **namelist;
    int entries = 0, lpc = 0;
    char buffer[PATH_MAX];

    entries = scandir(root, &namelist, NULL, alphasort);
    if (entries <= 0) {
        return list;
    }

    for (lpc = 0; lpc < entries; lpc++) {
        struct stat sb;
        if ('.' == namelist[lpc]->d_name[0]) {
            free(namelist[lpc]);
            continue;
        }

        snprintf(buffer, sizeof(buffer), "%s/%s", root, namelist[lpc]->d_name);

        stat(buffer, &sb);
        if (S_ISDIR(sb.st_mode)) {
            if (files) {
                free(namelist[lpc]);
                continue;
            }

        } else if (S_ISREG(sb.st_mode)) {
            if (files == FALSE) {
                free(namelist[lpc]);
                continue;

            } else if ((sb.st_mode & S_IXUSR) == 0
                       && (sb.st_mode & S_IXGRP) == 0
                       && (sb.st_mode & S_IXOTH) == 0) {
                free(namelist[lpc]);
                continue;
            }
        }

        list = g_list_append(list, strdup(namelist[lpc]->d_name));

        free(namelist[lpc]);
    }

    free(namelist);
#endif
    return list;
}

void
services_os_set_exec(svc_action_t *op)
{
    if (strcmp("enable", op->action) == 0) {
        op->opaque->exec = strdup("/sbin/chkconfig");
        op->opaque->args[0] = strdup(op->opaque->exec);
        op->opaque->args[1] = strdup(op->agent);
        op->opaque->args[2] = strdup("on");
        op->opaque->args[3] = NULL;

    } else if (strcmp("disable", op->action) == 0) {
        op->opaque->exec = strdup("/sbin/chkconfig");
        op->opaque->args[0] = strdup(op->opaque->exec);
        op->opaque->args[1] = strdup(op->agent);
        op->opaque->args[2] = strdup("off");
        op->opaque->args[3] = NULL;

    } else {
        asprintf(&op->opaque->exec, "%s/%s", LSB_ROOT, op->agent);
        op->opaque->args[0] = strdup(op->opaque->exec);
        op->opaque->args[1] = strdup(op->action);
        op->opaque->args[2] = NULL;
    }
}

GList *
services_os_list(void)
{
    return get_directory_list(LSB_ROOT, TRUE);
}


GList *
resources_os_list_ocf_providers(void)
{
    return get_directory_list(OCF_ROOT "/resource.d", FALSE);
}

GList *
resources_os_list_ocf_agents(const char *provider)
{
    if (provider) {
        char buffer[500];
        snprintf(buffer, sizeof(buffer), "%s//resource.d/%s", OCF_ROOT,
                 provider);
        return get_directory_list(buffer, TRUE);
    }
    return NULL;
}
