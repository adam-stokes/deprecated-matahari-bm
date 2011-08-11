/* logging.h - Copyright (C) 2010 Red Hat, Inc.
 * Written by Andrew Beekhof <andrew@beekhof.net>
 *
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

/**
 * \file
 * \brief Logging API
 * \ingroup coreapi
 */

#ifndef __MH_LOGGING__
#define __MH_LOGGING__

#ifdef __linux__
#  include <syslog.h>
#else
#  define LOG_EMERG   0 /* system is unusable */
#  define LOG_ALERT   1 /* action must be taken immediately */
#  define LOG_CRIT    2 /* critical conditions */
#  define LOG_ERR     3 /* error conditions */
#  define LOG_WARNING 4 /* warning conditions */
#  define LOG_NOTICE  5 /* normal but significant condition */
#  define LOG_INFO    6 /* informational */
#  define LOG_DEBUG   7 /* debug-level messages */
#endif

#include <stdio.h>
#include <glib.h>
#include "matahari/utilities.h"

#define LOG_TRACE    8

extern int mh_log_level;
extern void mh_log_fn(int priority, const char * fmt, ...) G_GNUC_PRINTF(2,3);
extern void mh_log_init(const char *ident, int level, gboolean to_stderr);
extern void mh_enable_stderr(gboolean to_stderr);

#if SUPPORT_TRACING

/* Linux with trace logging */

struct _mh_ddebug_query {
        const char *files;
        const char *formats;
        const char *functions;
        unsigned long long total;
        unsigned long long matches;
};

/*
 * An instance of this structure is created in a special
 * ELF section at every dynamic debug callsite.  At runtime,
 * the special section is treated as an array of these.
 */
struct _mh_ddebug {
        /*
         * These fields are used to drive the user interface
         * for selecting and displaying debug callsites.
         */
        const char *function;
        const char *filename;
        const char *format;
        unsigned int lineno:24;
        /*
         * The bump field will add to the level at the callsite.
         * The value here are changed dynamically when the user
         * writes commands to FIXME ;-)
         */
        int bump;
} __attribute__((aligned(8)));

/* will be assigned by ld linker magic */
extern struct _mh_ddebug __start___verbose[];
extern struct _mh_ddebug __stop___verbose[];

#  define MH_TRACE_INIT_DATA(name)                                        \
    void name(void);                                                      \
    void name(void) { MH_ASSERT(__start___verbose != __stop___verbose); } \
    void __attribute__ ((constructor)) name(void);

#  define MH_CHECK(expr, failure_action) do {                             \
        static struct _mh_ddebug descriptor                               \
            __attribute__((section("__verbose"), aligned(8))) =           \
            { __func__, __FILE__, #expr, __LINE__, LOG_TRACE};            \
                                                                          \
        if (__unlikely((expr) == 0)) {                                    \
            mh_abort(__FILE__, __FUNCTION__, __LINE__, #expr,      \
                     descriptor.bump != LOG_TRACE, 1);                    \
            failure_action;                                               \
        }                                                                 \
    } while(0)

/*
 * Throughout the macros below, note the leading, pre-comma, space in the
 * various ' , ##args' occurences to aid portability across versions of 'gcc'.
 * http://gcc.gnu.org/onlinedocs/cpp/Variadic-Macros.html#Variadic-Macros
 */
#  define mh_log(level, fmt, args...) do {                                \
        static struct _mh_ddebug descriptor                               \
            __attribute__((section("__verbose"), aligned(8))) =           \
            { __func__, __FILE__, fmt, __LINE__, LOG_TRACE};              \
                                                                          \
        if (__likely((level) <= mh_log_level)) {                          \
            mh_log_fn((level), "%s: " fmt, __FUNCTION__ , ##args); \
                                                                          \
        } else if (__unlikely(descriptor.bump != LOG_TRACE)) {            \
            mh_log_fn(descriptor.bump, "TRACE: %s: %s:%d " fmt,           \
                      __FUNCTION__ , __FILE__, __LINE__, ##args);  \
        }                                                                 \
    } while(0)

#  define mh_log_unlikely(level, fmt, args...) do {                       \
        static struct _mh_ddebug descriptor                               \
            __attribute__((section("__verbose"), aligned(8))) =           \
            { __func__, __FILE__, fmt, __LINE__, LOG_TRACE };             \
                                                                          \
        if (__unlikely((level) <= mh_log_level)) {                        \
            mh_log_fn((level), "%s: " fmt, __FUNCTION__ , ##args); \
                                                                          \
        } else if (__unlikely(descriptor.bump != LOG_TRACE)) {            \
            mh_log_fn(descriptor.bump, "TRACE: %s: %s:%d " fmt,           \
                      __FUNCTION__ , __FILE__, __LINE__, ##args);  \
        }                                                                 \
    } while(0)

#  define mh_log_alias(level, file, function, line, fmt, args...) do {    \
        if (line) {                                                       \
            mh_log_fn(level, "TRACE: %s %s:%d "fmt, function, file, line, \
                      ##args);                                            \
        } else {                                                          \
            mh_log_fn(level, "%s "fmt, function, ##args);                 \
        }                                                                 \
    } while(0)

#else

#  define MH_TRACE_INIT_DATA(name)

#  define MH_CHECK(expr, failure_action) do {                             \
        if (__unlikely((expr) == 0)) {                                    \
            mh_abort(__FILE__,__FUNCTION__,__LINE__, #expr, 0, 1); \
            failure_action;                                               \
        }                                                                 \
    } while(0)

#  define mh_log(level, fmt, args...) do {                                \
        if (__likely((level) <= mh_log_level)) {                          \
            mh_log_fn((level), "%s: " fmt, __FUNCTION__ , ##args); \
        }                                                                 \
    } while(0)

#  define mh_log_unlikely(level, fmt, args...) do {                       \
        if (__unlikely((level) <= mh_log_level)) {                        \
            mh_log_fn((level), "%s: " fmt, __FUNCTION__ , ##args); \
        }                                                                 \
    } while(0)

#  define mh_log_alias(level, file, function, line, fmt, args...) do {    \
        mh_log_fn(level, "%s"fmt, function, ##args);                      \
    } while(0)

#endif

#define mh_log_always(level, fmt, args...) mh_log_fn(level, "%s: " fmt,   \
                                                     __FUNCTION__ ,\
                                                     ##args)
#define mh_crit(fmt, args...)    mh_log(LOG_CRIT,    fmt , ##args)
#define mh_err(fmt, args...)     mh_log(LOG_ERR,     fmt , ##args)
#define mh_warn(fmt, args...)    mh_log(LOG_WARNING, fmt , ##args)
#define mh_notice(fmt, args...)  mh_log(LOG_NOTICE,  fmt , ##args)
#define mh_info(fmt, args...)    mh_log(LOG_INFO,    fmt , ##args)
#define mh_debug(fmt, args...)   mh_log_unlikely(LOG_DEBUG, fmt , ##args)
#define mh_trace(fmt, args...)   mh_log_unlikely(LOG_TRACE, fmt , ##args)

#define mh_perror(level, fmt, args...) do {                               \
        const char *err = strerror(errno);                                \
        fprintf(stderr, fmt ": %s (%d)\n", ##args, err, errno);           \
        mh_log(level, fmt ": %s (%d)", ##args, err, errno);               \
    } while(0)
#endif
