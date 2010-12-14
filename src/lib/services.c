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
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

#if __linux__
#include <sys/wait.h>
#endif

#include "matahari/logging.h"
#include "matahari/mainloop.h"
#include "matahari/services.h"

struct rsc_op_private_s 
{
	char *exec;
	char *args[4];
	gboolean cancel;
	
	guint repeat_timer;
	void (*callback)(rsc_op_t *op);
	
	int            stderr_fd;
	mainloop_fd_t *stderr_gsource;

	int            stdout_fd;
	mainloop_fd_t *stdout_gsource;
};

#define set_fd_opts(fd,opts) do {				    \
	int flag;						    \
	if ((flag = fcntl(fd, F_GETFL)) >= 0) {			    \
	    if (fcntl(fd, F_SETFL, flag|opts) < 0) {		    \
		mh_perror(LOG_ERR, "fcntl() write failed");	    \
	    }							    \
	} else {						    \
	    mh_perror(LOG_ERR, "fcntl() read failed");		    \
	}							    \
    } while(0)

GHashTable *recurring_actions = NULL;

rsc_op_t *create_service_op(
    const char *name, const char *action, int interval, int timeout)
{
    rsc_op_t *op = malloc(sizeof(rsc_op_t));
    memset(op, 0, sizeof(rsc_op_t));

    op->opaque = malloc(sizeof(rsc_op_private_t));
    memset(op->opaque, 0, sizeof(rsc_op_private_t));
    
    op->rsc = strdup(name);
    op->action = strdup(action);
    op->interval = interval;
    op->timeout = timeout;

    op->rclass = strdup("lsb");
    op->agent = strdup(name);

    op->id = malloc(500);
    snprintf(op->id, 500, "%s_%s_%d", name, action, interval);

    if(strcmp("enable", action) == 0) {
	op->opaque->exec = strdup("/sbin/chkconfig");
	op->opaque->args[0] = strdup(op->opaque->exec);
	op->opaque->args[1] = strdup(name);
	op->opaque->args[2] = strdup("on");
	op->opaque->args[3] = NULL;
	
    } else if(strcmp("disable", action) == 0) {
	op->opaque->exec = strdup("/sbin/chkconfig");
	op->opaque->args[0] = strdup(op->opaque->exec);
	op->opaque->args[1] = strdup(name);
	op->opaque->args[2] = strdup("off");
	op->opaque->args[3] = NULL;

    } else {
	op->opaque->exec = malloc(500);
	snprintf(op->opaque->exec, 500, "%s/%s", LSB_ROOT, name);
	op->opaque->args[0] = strdup(op->opaque->exec);
	op->opaque->args[1] = strdup(action);
	op->opaque->args[2] = NULL;
    }
    
    return op;
}

rsc_op_t *create_ocf_op(
    const char *name, const char *provider, const char *agent,
    const char *action, int interval, int timeout, GHashTable *params)
{
    rsc_op_t *op = malloc(sizeof(rsc_op_t));
    memset(op, 0, sizeof(rsc_op_t));

    op->rsc = strdup(name);
    op->action = strdup(action);
    op->interval = interval;
    op->timeout = timeout;

    op->rclass = strdup("ocf");
    op->agent = strdup(agent);
    op->provider = strdup(provider);

    /* op->params = params;  / Will need to make a copy */

    op->id = malloc(500);
    snprintf(op->id, 500, "%s_%s_%d", name, action, interval);

    op->opaque->exec = malloc(500);
    snprintf(op->id, 500, "%s/%s/%s", OCF_ROOT, name, provider);

    return op;
}

void free_operation(rsc_op_t *op)
{
    if(op == NULL) {
	return;
    }
    
    free(op->id);
    free(op->opaque->exec);

    free(op->opaque->args[0]);
    free(op->opaque->args[1]);
    free(op->opaque->args[2]);
    free(op->opaque->args[3]);

    free(op->rsc);
    free(op->action);

    free(op->rclass);
    free(op->agent);
    free(op->provider);

    free(op->stdout_data);
    free(op->stderr_data);

    if(op->params) {
	g_hash_table_destroy(op->params);
	op->params = NULL;
    }

    free(op);
}

static gboolean 
read_output(int fd, gpointer user_data)
{
    char * data = NULL;
    int rc = 0, len = 0;
    gboolean is_err = FALSE;
    rsc_op_t* op = (rsc_op_t *)user_data;

    if(fd == op->opaque->stderr_fd) {
	is_err = TRUE;
	if(op->stderr_data) {
	    len = strlen(op->stderr_data);
	    data = op->stderr_data;
	}
    } else if(op->stdout_data) {
	len = strlen(op->stdout_data);
	data = op->stdout_data;
    }

    do {
	char buf[500];
	rc = read(fd, buf, 500);
	if(rc > 0) {
	    buf[rc] = 0;
	    data = realloc(data, len + rc + 1);
	    sprintf(data+len, "%s", buf);
	    len += rc;
	    
	} else if(errno != EINTR) {
	    /* error or EOF
	     * Cleanup happens in pipe_done()
	     */
	    rc = FALSE;
	    break;
	}
	
    } while (rc == 500 || rc < 0);

    if (data != NULL && is_err) {
	op->stderr_data = data;
    } else if(data != NULL) {
	op->stdout_data = data;
    }

    return rc;
}

static void
pipe_out_done(gpointer user_data)
{
    rsc_op_t* op = (rsc_op_t *)user_data;
    op->opaque->stdout_gsource = NULL;
    if (op->opaque->stdout_fd > STDERR_FILENO) {
	close(op->opaque->stdout_fd);
    }
    op->opaque->stdout_fd = -1;
}

static void
pipe_err_done(gpointer user_data)
{
    rsc_op_t* op = (rsc_op_t *)user_data;
    op->opaque->stderr_gsource = NULL;
    if (op->opaque->stderr_fd > STDERR_FILENO) {
	close(op->opaque->stderr_fd);
    }
    op->opaque->stderr_fd = -1;
}

static void
set_ocf_env(gpointer key, gpointer value, gpointer user_data)
{
#if __linux__
    if (setenv(key, value, 1) != 0) {
	mh_err("setenv failed in raexecocf.");
    }
#endif
}

static void
set_ocf_env_with_prefix(gpointer key, gpointer value, gpointer user_data)
{
    /* TODO: Add OCF_RESKEY_ prefix to 'key' */
    set_ocf_env(key, value, user_data);
}

static void
add_OCF_env_vars(rsc_op_t *op)
{
    if (strcmp("ocf", op->rclass) != 0) {
	return;
    }

    if(op->params) {
	g_hash_table_foreach(op->params, set_ocf_env_with_prefix, NULL);
    }
    
    set_ocf_env("OCF_RA_VERSION_MAJOR", "1", NULL);
    set_ocf_env("OCF_RA_VERSION_MINOR", "0", NULL);
    set_ocf_env("OCF_ROOT", OCF_ROOT, NULL);

    if(op->rsc) {
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

static gboolean recurring_op_timer(gpointer data)
{
    rsc_op_t *op = data;
    mh_debug("Scheduling another invokation of %s", op->id);

    /* Clean out the old result */
    free(op->stdout_data); op->stdout_data = NULL;
    free(op->stderr_data); op->stderr_data = NULL;
    
    perform_async_action(op, NULL);
    return FALSE;
}

gboolean cancel_action(const char *name, const char *action, int interval)
{
    rsc_op_t* op = NULL;
    gboolean found = FALSE;
    char *id = malloc(500);

    snprintf(id, 500, "%s_%s_%d", name, action, interval);
    op = g_hash_table_lookup(recurring_actions, id);
    free(id);

    if(op) {
	found = TRUE;
	op->opaque->cancel = TRUE;
	mh_debug("Removing %s", op->id);
	if(op->opaque->repeat_timer) {
	    g_source_remove(op->opaque->repeat_timer);
	}
	free_operation(op);
    }
    
    return found;
}

static void
operation_finished(mainloop_child_t *p, int status, int signo, int exitcode)
{
#if __linux__
    char *next = NULL;
    char *offset = NULL;
    rsc_op_t *op = p->privatedata;

    p->privatedata = NULL;
    op->status = LRM_OP_DONE;
    MH_ASSERT(op->pid == p->pid);

    if( signo ) {
	if( p->timeout ) {
	    mh_warn("%s:%d - timed out after %dms", op->id, op->pid, op->timeout);
	    op->status = LRM_OP_TIMEOUT;

	} else {
	    mh_warn("%s:%d - terminated with signal %d", op->id, op->pid, signo);
	    op->status = LRM_OP_ERROR;
	}

    } else {
	op->rc = exitcode;
	mh_debug("%s:%d - exited with rc=%d", op->id, op->pid, exitcode);

	if(op->stdout_data) {
	    next = op->stdout_data;
	    do {
		offset = next;
		next = strchrnul(offset, '\n');
		mh_debug("%s:%d [ %.*s ]", op->id, op->pid, (int)(next-offset), offset);
		if(next[0] != 0) {
		    next++;
		}
		
	    } while(next != NULL && next[0] != 0);
	}
	
	if(op->stderr_data) {
	    next = op->stderr_data;
	    do {
		offset = next;
		next = strchrnul(offset, '\n');
		mh_notice("%s:%d [ %.*s ]", op->id, op->pid, (int)(next-offset), offset);
		if(next[0] != 0) {
		    next++;
		}
		
	    } while(next != NULL && next[0] != 0);
	}
    }
    
    if(op->interval && op->opaque->cancel == FALSE) {
	op->opaque->repeat_timer = g_timeout_add(op->interval, recurring_op_timer, (void*)op);
    }

    op->pid = 0;
    if(op->opaque->callback) {
	/* Callback might call cancel which would result in the message being free'd
	 * Do not access 'op' after this line
	 */
	op->opaque->callback(op);

    } else if(op->opaque->repeat_timer == 0) {
	free_operation(op);
    }
    
#endif
}

gboolean
perform_action(rsc_op_t* op, gboolean synchronous)
{
#if __linux__
    int rc, lpc;
    int stdout_fd[2];
    int stderr_fd[2];

    if ( pipe(stdout_fd) < 0 ) {
	mh_perror(LOG_ERR, "pipe() failed");
    }

    if ( pipe(stderr_fd) < 0 ) {
	mh_perror(LOG_ERR, "pipe() failed");
    }

    op->pid = fork();
    switch(op->pid) {
	case -1:
	    mh_perror(LOG_ERR, "fork() failed");
	    close(stdout_fd[0]);
	    close(stdout_fd[1]);
	    close(stderr_fd[0]);
	    close(stderr_fd[1]);
	    return FALSE;

	case 0:		/* Child */
	    /* Man: The call setpgrp() is equivalent to setpgid(0,0)
	     * _and_ compiles on BSD variants too
	     * need to investigate if it works the same too.
	     */
	    setpgid(0,0);
	    close(stdout_fd[0]);
	    close(stderr_fd[0]);
	    if (STDOUT_FILENO != stdout_fd[1]) {
		if (dup2(stdout_fd[1], STDOUT_FILENO)!=STDOUT_FILENO) {
		    mh_perror(LOG_ERR, "dup2() failed (stdout)");
		}
		close(stdout_fd[1]);
	    }
	    if (STDERR_FILENO != stderr_fd[1]) {
		if (dup2(stderr_fd[1], STDERR_FILENO)!=STDERR_FILENO) {
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
	    exit(rc);
    }

    /* Only the parent reaches here */
    close(stdout_fd[1]);
    close(stderr_fd[1]);

    /*
     * No any obviouse proof of lrmd hang in pipe read yet.
     * Bug 475 may be a duplicate of bug 499.
     * Anyway, via test, it's proved that NOBLOCK read will
     * obviously reduce the RA execution time (bug 553).
     */
    /* Let the read operations be NONBLOCK */ 

    op->opaque->stdout_fd = stdout_fd[0];
    set_fd_opts(op->opaque->stdout_fd, O_NONBLOCK);

    op->opaque->stderr_fd = stderr_fd[0];
    set_fd_opts(op->opaque->stderr_fd, O_NONBLOCK);

    if(synchronous) {
	int status = 0;
	mh_err("Waiting for %d", op->pid);
	while(waitpid(op->pid, &status, WNOHANG) <= 0) {
	    sleep(1);
	}

	mh_err("Child done: %d", op->pid);
	if (WIFEXITED(status)) {
	    op->status = LRM_OP_DONE;
	    op->rc = WEXITSTATUS(status);
	    mh_err("Managed %s process %d exited with rc=%d", op->id, op->pid, op->rc);
	    
	} else if (WIFSIGNALED(status)) {
	    int signo = WTERMSIG(status);
	    op->status = LRM_OP_ERROR;
#if 0
	    /* How to do outside of mainloop? */
	    if( p->timeout ) {
		mh_warn("%s:%d - timed out after %dms", op->id, op->pid, op->timeout);
		op->status = LRM_OP_TIMEOUT;
	    }
#endif
	    mh_err("Managed %s process %d exited with signal=%d", op->id, op->pid, signo);
	}
#ifdef WCOREDUMP
	if (WCOREDUMP(status)) {
	    mh_err("Managed %s process %d dumped core", op->id, op->pid);
	}
#endif
	
	read_output(op->opaque->stdout_fd, op);
	read_output(op->opaque->stderr_fd, op);
	
    } else {
	mh_err("Async waiting for %d - %s", op->pid, op->opaque->exec);
	mainloop_add_child(op->pid, op->timeout, op->id, op, operation_finished);

	op->opaque->stdout_gsource = mainloop_add_fd(
	    G_PRIORITY_LOW, op->opaque->stdout_fd, read_output, pipe_out_done, op);

	op->opaque->stderr_gsource = mainloop_add_fd(
	    G_PRIORITY_LOW, op->opaque->stderr_fd, read_output, pipe_err_done, op);
    }
    
#endif
    return TRUE;
}

gboolean
perform_async_action(rsc_op_t* op, void (*action_callback)(rsc_op_t*))
{
    if(action_callback) {
	op->opaque->callback = action_callback;
    }

    if(recurring_actions == NULL) {
	recurring_actions = g_hash_table_new_full(
	    g_str_hash, g_str_equal, NULL, NULL);
    }

    if(op->interval > 0) {
	g_hash_table_replace(recurring_actions, op->id, op);
    }
    
    return perform_action(op, FALSE);
}

gboolean
perform_sync_action(rsc_op_t* op)
{
    gboolean rc = perform_action(op, TRUE);
    mh_trace(" > %s_%s_%d: %s = %d", op->rsc, op->action, op->interval, op->opaque->exec, op->rc);
    if(op->stdout_data) {
	mh_trace("   > stdout: %s", op->stdout_data);
    }
    if(op->stderr_data) {
	mh_trace("   > stderr: %s", op->stderr_data);
    }
    return rc;
}

GList *
get_directory_list(const char *root, gboolean files)
{
    GList* list = NULL;
#if __linux__
    struct dirent **namelist;
    int entries = 0, lpc = 0;
    char buffer[FILENAME_MAX+1];

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

	snprintf(buffer, FILENAME_MAX, "%s/%s", root, namelist[lpc]->d_name);
	
	stat(buffer, &sb);
	if(S_ISDIR(sb.st_mode)) {
	    if(files) {
		free(namelist[lpc]);
		continue;
	    }
	    
	} else if(S_ISREG(sb.st_mode)) {
	    if(files == FALSE) {
		free(namelist[lpc]);
		continue;
		
	    } else if( (sb.st_mode & S_IXUSR) == 0
		       && (sb.st_mode & S_IXGRP) == 0
		       && (sb.st_mode & S_IXOTH) == 0 ) {
		free(namelist[lpc]);
		continue;
	    }
	}

	list = g_list_append(list, namelist[lpc]->d_name);
    }
    
    free(namelist);
#endif
    return list;
}
