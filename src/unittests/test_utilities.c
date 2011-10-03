/*
 * mh_test_utilities.c:  util functions for testing
 *
 * Copyright (C) 2011 Red Hat Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 * Author: Adam Stokes <astokes@fedoraproject.org>
 */

#include "mh_test_utilities.h"
#include "matahari/utilities.h"
#include <pcre.h>
#include <string.h>
#include <stdio.h>

#define OVECCOUNT 30

int
mh_test_is_match(const char *pattern, const char *subject)
{
    int erroroffset;
    int ovector[OVECCOUNT];
    const char *error;
    pcre *re;
    int rc;

    re = pcre_compile(pattern,
                      0,
                      &error,
                      &erroroffset,
                      NULL);
    if (re == NULL) {
        return PCRE_ERROR_NULL;
    }

    rc = pcre_exec(
        re,
        NULL,
        subject,
        strlen(subject),
        0,
        0,
        ovector,
        DIMOF(ovector));

    /* Since we only care about return code for now free the regex */
    pcre_free(re);
    return rc;
}
