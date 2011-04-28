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
#include <sys/reboot.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>

#include <linux/reboot.h>

#include <pcre.h>

#include "matahari/logging.h"
#include "matahari/host.h"
#include "host_private.h"

const char *host_os_get_cpu_flags(void)
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

        if (match != expected || strncmp(cur + found[2], "flags", 5)) {
            continue;
        }

        len = 1 + found[5] - found[4];
        if (!(flags = malloc(len))) {
            goto done;
        }
        strncpy(flags, cur + found[4], len);
        flags[len - 1] = '\0';
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
    reboot(LINUX_REBOOT_CMD_RESTART);
}

void
host_os_shutdown(void)
{
    reboot(LINUX_REBOOT_CMD_HALT);
}
