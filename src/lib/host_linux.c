/* host_linux.c - Copyright (C) 2010 Red Hat, Inc.
 * Written by Darryl L. Pierce <dpierce@redhat.com>
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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/reboot.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

#include <linux/reboot.h>
#include <linux/kd.h>

#include <pcre.h>
#include <uuid/uuid.h>
#include <curl/curl.h>

#include "matahari/logging.h"
#include "matahari/host.h"

#include "utilities_private.h"
#include "host_private.h"


#define UUID_STR_BUF_LEN 37

#define BUFSIZE 4096

const char *
host_os_get_cpu_flags(void)
{
    static const char regexstr[] = "(.*\\S)\\s*:\\s*(\\S.*)";
    static char *flags = NULL;

    size_t read_chars = 0;
    size_t data_length = 0;
    char *buffer = NULL;
    FILE *input = NULL;
    const char *pcre_error;
    int pcre_error_offset;
    pcre *regex = NULL;
    char *cur, *next;

    if (flags) {
        return flags;
    }

    if (!(input = fopen("/proc/cpuinfo", "r"))) {
        goto done;
    }

    do {
        static const size_t chunk = 512;

        buffer = realloc(buffer, chunk + data_length + 1);
        read_chars = fread(buffer + data_length, 1, chunk, input);
        data_length += read_chars;
    } while (read_chars > 0);

    if (data_length == 0) {
        mh_warn("Could not read from /proc/cpuinfo");
        goto done;
    }

    buffer[data_length] = '\0';

    regex = pcre_compile(regexstr, 0, &pcre_error, &pcre_error_offset, NULL);
    if (!regex) {
        mh_err("Unable to compile regular expression '%s' at offset %d: %s",
               regexstr, pcre_error_offset, pcre_error);
        goto done;
    }

    next = buffer;
    while ((cur = strsep(&next, "\n"))) {
        static const int expected = 3;
        size_t len;
        int match;
        int found[9];

        match = pcre_exec(regex, NULL, cur, strlen(cur),
                          0, PCRE_NOTEMPTY, found,
                          sizeof(found) / sizeof(found[0]));

        if (match != expected) {
            continue;
        }

        // PowerPC
        if (strncmp(cur + found[2], "cpu ", 4)) {
            char *p = strstr(cur + found[4], "altivec supported");
            if (p && (p - cur) < found[5]) {
                flags = strdup("altivec");
                break;
            }
        }

        if (strncmp(cur + found[2], "flags", 5) &&
            strncmp(cur + found[2], "features", 8)) {
            continue;
        }

        len = 1 + found[5] - found[4];
        if (!(flags = malloc(len))) {
            goto done;
        }
        mh_string_copy(flags, cur + found[4], len);
        break;
    }

done:
    if (input) {
        fclose(input);
    }

    free(buffer);

    if (regex) {
        pcre_free(regex);
    }

    if (flags == NULL) {
        flags = strdup("unknown");
    }

    return flags;
}

void
host_os_reboot(void)
{
    gint rc = 0;
    GError *err = NULL;
    if(g_spawn_command_line_sync("reboot", NULL, NULL, &rc, &err) == FALSE) {
        mh_err("reboot command failed (rc=%d) - falling back to brut force", rc);
        sync();
        reboot(LINUX_REBOOT_CMD_RESTART);
    }
}

void
host_os_shutdown(void)
{
    gint rc = 0;
    GError *err = NULL;
    if(g_spawn_command_line_sync("shutdown -h now", NULL, NULL, &rc, &err) == FALSE) {
        mh_err("shutdown command failed (rc=%d) - falling back to brut force", rc);
        sync();
        reboot(LINUX_REBOOT_CMD_HALT);
    }
}

int
host_os_identify(void)
{
    static const long DURATION = 1000; /* 1 second */
    static const long FREQ = 440; /* 440 Hz */

    int fd = open("/dev/tty", O_NOCTTY);
    int res;

    if (fd == -1) {
        return -1;
    }

    /*
     * Reference info on KDMKTONE:
     *     http://tldp.org/LDP/lpg/node83.html
     */
    res = ioctl(fd, KDMKTONE, (DURATION << 32) + (1190000 / FREQ));

    close(fd);

    return res;
}

char *
host_os_machine_uuid(void)
{
    gchar *output = NULL;
    gchar **lines = NULL;
    unsigned int i;
    static const gint max_lines = 256;
    pcre *regex;
    const char *pcre_error = NULL;
    int pcre_error_offset = 0;
    static const char regex_str[] = "\\s+UUID:\\s+(.*)";
    GError *error = NULL;
    gboolean res;
    char *uuid = NULL;
    gchar *argv[] = { "dmidecode", "-t", "system", NULL };

    /*
     * libsmbios doesn't expose the UUID.  dmidecode already does a good job
     * of getting it, but it doesn't have a library.  Executing dmidecode
     * doesn't seem pretty, but it should at least provide reliable info.
     */

    res = g_spawn_sync(NULL, argv, NULL,
                G_SPAWN_SEARCH_PATH | G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, NULL, &error);

    if (res == FALSE) {
        mh_err("Failed to run dmidecode to get UUID: %s\n", error->message);
        g_error_free(error);
        error = NULL;
    }

    if (!output) {
        mh_err("Got no output from dmidecode when trying to get UUID.\n");
        return NULL;
    }

    lines = g_strsplit(output, "\n", max_lines);

    g_free(output);
    output = NULL;

    regex = pcre_compile(regex_str, 0, &pcre_error, &pcre_error_offset, NULL);
    if (!regex) {
        mh_err("Unable to compile regular expression '%s' at offset %d: %s",
               regex_str, pcre_error_offset, pcre_error);
        uuid = strdup("(regex-compile-failed)");
        goto cleanup;
    }

    for (i = 0; lines && lines[i]; i++) {
        int match;
        int found[8] = { 0, };

        match = pcre_exec(regex, NULL, lines[i], strlen(lines[i]),
                          0, 0, found,
                          sizeof(found) / sizeof(found[0]));

        if (match == 2) {
            uuid = strdup(lines[i] + found[2]);
            break;
        }
    }

cleanup:
    if (lines) {
        g_strfreev(lines);
    }

    if (regex) {
        pcre_free(regex);
    }

    return uuid;
}

struct curl_write_cb_data {
    char buf[256];
    size_t used;
};

static size_t
curl_write_cb(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    struct curl_write_cb_data *buf = userdata;
    size_t len;

    len = size * nmemb;

    if (len >= (sizeof(buf->buf) - buf->used)) {
        mh_err("Buffer not large enough to hold received UC2 instance ID.");
        return 0;
    }

    memcpy(buf->buf + buf->used, ptr, len);

    return len;
}

char *
host_os_ec2_instance_id(void)
{
    CURL *curl;
    CURLcode curl_res;
    long response = 0;
    static const char URI[] = "http://169.254.169.254/latest/meta-data/instance-id";
    struct curl_write_cb_data buf = {
        .used = 0,
    };

    if (mh_curl_init() != MH_RES_SUCCESS) {
        return NULL;
    }

    if (!(curl = curl_easy_init())) {
        mh_warn("Failed to curl_easy_init()");
        return NULL;
    }

    curl_res = curl_easy_setopt(curl, CURLOPT_URL, URI);
    if (curl_res != CURLE_OK) {
        mh_warn("curl_easy_setopt of URI '%s' failed. (%d)", URI, curl_res);
        goto return_cleanup;
    }

    curl_res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
    if (curl_res != CURLE_OK) {
        mh_warn("curl_easy_setopt of WRITEFUNCTION failed. (%d)", curl_res);
        goto return_cleanup;
    }

    curl_res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    if (curl_res != CURLE_OK) {
        mh_warn("curl_easy_setopt of WRITEDATA failed. (%d)", curl_res);
        goto return_cleanup;
    }

    curl_res = curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long) 3);
    if (curl_res != CURLE_OK) {
        mh_warn("curl_easy_setopt of TIMEOUT failed. (%d)", curl_res);
        goto return_cleanup;
    }

    curl_res = curl_easy_perform(curl);
    if (curl_res != CURLE_OK) {
        mh_warn("curl request for URI '%s' failed. (%d)", URI, curl_res);
        goto return_cleanup;
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response);
    if (curl_res != CURLE_OK) {
        mh_warn("curl_easy_getinfo for RESPONSE_CODE failed. (%d)", curl_res);
        goto return_cleanup;
    }
    if (response < 200 || response > 299) {
        mh_warn("curl request for URI '%s' got response %ld", URI, response);
    }

return_cleanup:
    curl_easy_cleanup(curl);

    return mh_strlen_zero(buf.buf) ? NULL : strdup(buf.buf);
}

char *
host_os_custom_uuid(void)
{
    return mh_file_first_line("/etc/custom-machine-id");
}

char *
host_os_reboot_uuid(void)
{
    /* Relies on /var/run being erased at boot-time as is common on most modern distros */
    static const char file[] = "/var/run/matahari-reboot-id";
    uuid_t buffer;
    GError* error = NULL;
    char *uuid = mh_file_first_line(file);

    if (uuid) {
        return uuid;
    }

    uuid = malloc(UUID_STR_BUF_LEN);
    if (!uuid) {
        return NULL;
    }

    uuid_generate(buffer);
    uuid_unparse(buffer, uuid);

    if (g_file_set_contents(file, uuid, strlen(uuid), &error) == FALSE) {
        mh_info("%s", error->message);
        free(uuid);
        uuid = strdup(error->message);
    }

    if (error) {
        g_error_free(error);
    }

    return uuid;
}

char *
host_os_agent_uuid(void)
{
    static char agent_uuid[UUID_STR_BUF_LEN] = "";
    uuid_t buffer;

    if (!mh_strlen_zero(agent_uuid)) {
        return strdup(agent_uuid);
    }

    uuid_generate(buffer);
    uuid_unparse(buffer, agent_uuid);

    return strdup(agent_uuid);
}

int
host_os_set_custom_uuid(const char *uuid)
{
    int rc = 0;
    GError *error = NULL;

    if (!uuid) {
        uuid = "";
    }

    if (g_file_set_contents("/etc/custom-machine-id", uuid, strlen(uuid), &error) == FALSE) {
        mh_info("%s", error->message);
        rc = error->code;
    }

    if (error) {
        g_error_free(error);
    }

    return rc;
}

static enum mh_result
exec_command(const char *apath, char *args[], int atimeout, char **stdoutbuf,
             char **stderrbuf)
{
    enum mh_result res = MH_RES_SUCCESS;
    GPid pid = 0;
    GError *gerr = NULL;
    int status = 0;
    int timeout = atimeout;
    gint stdout_fd;
    gint stderr_fd;

    if (!args || !args[0])
      return MH_RES_OTHER_ERROR;

    mh_trace("Spawning '%s'\n", args[0]);
    if (!g_spawn_async_with_pipes(apath, args, NULL, G_SPAWN_DO_NOT_REAP_CHILD,
                                  NULL, NULL, &pid, NULL, &stdout_fd,
                                  &stderr_fd, &gerr)) {
        mh_perror(LOG_ERR, "Spawn_process failed with code %d, message: %s\n",
                  gerr->code, gerr->message);
        return MH_RES_OTHER_ERROR;
    }

    mh_trace("Waiting for %d", pid);
    while (timeout > 0 && waitpid(pid, &status, WNOHANG) <= 0) {
        sleep(1);
        timeout--;
    }

    if (timeout == 0) {
        int killrc = sigar_proc_kill(pid, SIGKILL);

        res = MH_RES_BACKEND_ERROR;
        mh_warn("%d - timed out after %dms", pid, atimeout);

        if (killrc != SIGAR_OK && killrc != ESRCH) {
            mh_err("kill(%d, KILL) failed: %d", pid, killrc);
        }

    } else if (WIFEXITED(status)) {
        if (WEXITSTATUS(status) > 0)
            res = MH_RES_BACKEND_ERROR;
        mh_err("Managed process %d exited with rc=%d", pid, WEXITSTATUS(status));

    } else if (WIFSIGNALED(status)) {
        int signo = WTERMSIG(status);
        res = MH_RES_BACKEND_ERROR;
        mh_err("Managed process %d exited with signal=%d", pid, signo);
    }

    mh_trace("Child done: %d", pid);

    if (stdoutbuf) {
        if (mh_read_from_fd(stdout_fd, stdoutbuf) < 0) {
            mh_err("Unable to read standard output from command %s.", apath);
            stdoutbuf = NULL;
        } else {
            mh_debug("stdout: %s", *stdoutbuf);
        }
    }

    if (stderrbuf) {
        if (mh_read_from_fd(stderr_fd, stderrbuf) < 0) {
            mh_err("Unable to read standard error output from command %s.", apath);
            stderrbuf = NULL;
        } else {
            mh_debug("stderr: %s", *stderrbuf);
        }
    }

    close(stdout_fd);
    close(stderr_fd);

    g_spawn_close_pid(pid);

    return res;
}

GList *
host_os_list_power_profiles(void)
{
    char *stdoutbuf = NULL;
    char *c1, *c2;
    char *args[3] = {0};
    GList *list = NULL;
    enum mh_result res;
    int len;

    args[0] = TUNEDADM;
    args[1] = TA_LISTPROFILES;
    args[2] = NULL;

    res = exec_command(NULL, args, TIMEOUT, &stdoutbuf, NULL);
    len = strlen(stdoutbuf);

    if (res == MH_RES_SUCCESS && stdoutbuf) {
        c1 = stdoutbuf;
        do {
            // Each line with profile in "tuned-adm list" starts with "-"
            if (*c1 == '-' && c1 - stdoutbuf + 2 < len) {
                // Skip the dash and the space after it
                c1 += 2;
                // Profile name is the rest of the line
                c2 = strchr(c1, '\n');
                if (c2 && c2 > c1) {
                    // Replace \n with \0 and append it to output
                    *c2 = '\0';
                    list = g_list_append(list, strdup(c1));
                    // Replace it back with \n, so free() will free whole string
                    *c2 = '\n';
                }
            } else {
                // Line doesn't contain the profile -> skip the line
                c2 = strchr(c1, '\n');
            }
            // Move c1 to beggining of the next line
            if (c2) {
                c1 = c2 + 1;
            }
        } while (c2 && c1 - stdoutbuf < len);

        if (g_list_length(list) == 0) {
            // Return at least "off" profile, if no other profile found
            list = g_list_append(list, strdup(TA_OFF));
        }
    }
    free(stdoutbuf);

    return list;
}

static gboolean
check_profile(const char *profile)
{
    GList *list = NULL;
    GList *llist;
    gboolean rc = FALSE;

    if (!(list = host_os_list_power_profiles()))
        return FALSE;

    if (!g_list_length(list))
        return FALSE;

    for (llist = g_list_first(list); llist && !rc; llist = g_list_next(llist)) {
        mh_trace("comparing '%s' with '%s'", (char *) llist->data, profile);
        if (!strcmp(profile, (char *) llist->data)) {
            rc = TRUE;
            break;
        }
    }
    g_list_free_full(list, free);

    return rc;
}

enum mh_result
host_os_set_power_profile(const char *profile)
{
    char *args[4] = {0};

    args[0] = TUNEDADM;
    if (!profile)
        return MH_RES_INVALID_ARGS;

    if (!strcmp(profile, TA_OFF)) {
        args[1] = TA_OFF;
        mh_trace("switching tuning off");
    } else {
        if (!check_profile(profile)) {
            mh_err("invalid profile: %s", profile);
            return MH_RES_INVALID_ARGS;
        }
        args[1] = TA_SETPROFILE;
        mh_trace("setting profile: %s", profile);
    }

    args[2] = (char *) profile;
    args[3] = NULL;

    return exec_command(NULL, args, TIMEOUT, NULL, NULL);
}

enum mh_result
host_os_get_power_profile(char **profile)
{
    char *stdoutbuf = NULL;
    char *c1, *c2 = NULL;
    char *args[3] = {0};
    enum mh_result res;

    args[0] = TUNEDADM;
    args[1] = TA_GETPROFILE;
    args[2] = NULL;

    res = exec_command(NULL, args, TIMEOUT, &stdoutbuf, NULL);
    if (res == MH_RES_SUCCESS) {
        // Parse first line of "tuned-adm active", that is something like
        // "Current active profile: profile_name", so take what is after
        // semicolon and space to the end of the line
        if (stdoutbuf && (c1 = strchr(stdoutbuf, ':')) &&
                (c1 - stdoutbuf + 2 < strlen(stdoutbuf))) {
            if ((c2 = strchr(c1, '\n'))) {
                *c2 = '\0';
            }
            // + 2 because we need to skip semicolon and space
            *profile = strdup(c1 + 2);
            if (c2)
                *c2 = '\n';
        } else {
            *profile = strdup(STR_UNK);
        }
    } else {
        res = MH_RES_BACKEND_ERROR;
    }
    free(stdoutbuf);

    return res;
}
