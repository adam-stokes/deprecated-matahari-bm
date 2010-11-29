#ifndef __MH_LOGGING__
#define __MH_LOGGING__

#ifdef __linux__
#include <syslog.h>
#define mh_log_fn syslog
#else
 /* Broken but will compile
  * TODO - Use sigar logging wrappers
  */
#define mh_log_fn fprintf
#define LOG_CRIT stderr
#define LOG_ERR stderr
#define LOG_WARNING stderr
#define LOG_NOTICE stderr
#define LOG_INFO stderr
#define LOG_DEBUG stderr
#endif

#include <stdio.h>
#include "matahari/utilities.h"

#define LOG_TRACE    12

extern int mh_log_level;
extern void mh_log_init(const char *ident, int level);

#if SUPPORT_TRACING
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

#  define MH_TRACE_INIT_DATA(name)					\
    void name(void);							\
    void name(void) { MH_ASSERT(__start___verbose != __stop___verbose); } \
    void __attribute__ ((constructor)) name(void);

#define MH_CHECK(expr, failure_action) do {				\
	static struct _mh_ddebug descriptor				\
	    __attribute__((section("__verbose"), aligned(8))) =		\
	    { __func__, __FILE__, #expr, __LINE__, LOG_TRACE};		\
									\
	if(__unlikely((expr) == FALSE)) {				\
	    mh_abort(__FILE__, __PRETTY_FUNCTION__, __LINE__, #expr,	\
		     descriptor.bump != LOG_TRACE, TRUE);		\
	    failure_action;						\
	}								\
    } while(0)

/*
 * Throughout the macros below, note the leading, pre-comma, space in the
 * various ' , ##args' occurences to aid portability across versions of 'gcc'.
 *	http://gcc.gnu.org/onlinedocs/cpp/Variadic-Macros.html#Variadic-Macros
 */
#  define mh_log(level, fmt, args...) do {				\
	static struct _mh_ddebug descriptor				\
	    __attribute__((section("__verbose"), aligned(8))) =		\
	    { __func__, __FILE__, fmt, __LINE__, LOG_TRACE};		\
	    								\
	if(__likely((level) <= mh_log_level)) {				\
	    mh_log_fn((level), "%s: " fmt, __PRETTY_FUNCTION__ , ##args); \
	    								\
	} else if(__unlikely(descriptor.bump != LOG_TRACE)) {		\
	    mh_log_fn(descriptor.bump, "TRACE: %s: %s:%d " fmt, __PRETTY_FUNCTION__ , __FILE__, __LINE__, ##args); \
	}								\
    } while(0)

#  define mh_log_unlikely(level, fmt, args...) do {			\
	static struct _mh_ddebug descriptor				\
	    __attribute__((section("__verbose"), aligned(8))) =		\
	    { __func__, __FILE__, fmt, __LINE__, LOG_TRACE };		\
	    								\
	if(__unlikely((level) <= mh_log_level)) {			\
	    mh_log_fn((level), "%s: " fmt, __PRETTY_FUNCTION__ , ##args); \
	    								\
	} else if(__unlikely(descriptor.bump != LOG_TRACE)) {		\
	    mh_log_fn(descriptor.bump, "TRACE: %s: %s:%d " fmt, __PRETTY_FUNCTION__ , __FILE__, __LINE__, ##args); \
	}								\
    } while(0)

#  define mh_log_alias(level, file, function, line, fmt, args...) do { \
	if(line) {							\
	    mh_log_fn(level, "TRACE: %s %s:%d "fmt, function, file, line, ##args); \
	} else {							\
	    mh_log_fn(level, "%s "fmt, function, ##args);		\
	}								\
    } while(0)

#else

#  define MH_TRACE_INIT_DATA(name)

#define MH_CHECK(expr, failure_action) do {				\
	if(__unlikely((expr) == FALSE)) {				\
	    mh_abort(__FILE__,__PRETTY_FUNCTION__,__LINE__, #expr, FALSE, TRUE); \
	    failure_action;						\
	}								\
    } while(0)

#  define mh_log(level, fmt, args...) do {				\
	if(__likely((level) <= mh_log_level)) {				\
	    mh_log_fn((level), "%s: " fmt, __PRETTY_FUNCTION__ , ##args); \
	}								\
    } while(0)

#  define mh_log_unlikely(level, fmt, args...) do {			\
	if(__unlikely((level) <= mh_log_level)) {			\
	    mh_log_fn((level), "%s: " fmt, __PRETTY_FUNCTION__ , ##args); \
	}								\
    } while(0)

#  define mh_log_alias(level, file, function, line, fmt, args...) do { \
	mh_log_fn(level, "%s"fmt, function, ##args);			\
    } while(0)

#endif

#define mh_log_always(level, fmt, args...) mh_log_fn(level, "%s: " fmt, __PRETTY_FUNCTION__ , ##args)
#define mh_crit(fmt, args...)    mh_log_always(LOG_CRIT,    fmt , ##args)
#define mh_err(fmt, args...)     mh_log(LOG_ERR,     fmt , ##args)
#define mh_warn(fmt, args...)    mh_log(LOG_WARNING, fmt , ##args)
#define mh_notice(fmt, args...)  mh_log(LOG_NOTICE,  fmt , ##args)
#define mh_info(fmt, args...)    mh_log(LOG_INFO,    fmt , ##args)
#define mh_debug(fmt, args...)   mh_log_unlikely(LOG_DEBUG, fmt , ##args)
#define mh_trace(fmt, args...)   mh_log_unlikely(LOG_TRACE, fmt , ##args)

#define mh_perror(level, fmt, args...) do {				\
	const char *err = strerror(errno);				\
	fprintf(stderr, fmt ": %s (%d)\n", ##args, err, errno);		\
	mh_log(level, fmt ": %s (%d)", ##args, err, errno);		\
    } while(0)
#endif
