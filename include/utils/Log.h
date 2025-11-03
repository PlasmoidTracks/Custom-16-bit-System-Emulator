#ifndef _LOG_H_
#define _LOG_H_

#include <time.h>

typedef enum {
    LP_MINPRIO,     // Minimum priority level (placeholder)
    LP_DEBUG,       // Detailed information, typically of interest only when diagnosing problems.
    LP_DEPRECATED,  // For deprecated functions
    LP_VERBOSE,     // Usually more detailed information, that may be useless
    LP_INFO,        // Informational messages that highlight the progress of the application at a coarse-grained level.
    LP_SUCCESS, 
    LP_NOTICE,      // Normal but significant events, a level between INFO and WARNING.
    LP_TODO, 
    LP_WARNING,     // Potentially harmful situations.
    LP_ERROR,       // Error events that might still allow the application to continue running.
    LP_CRITICAL,    // Critical conditions, often implying a severe error that will prevent parts of the application from functioning.
    LP_EMERGENCY,   // A severe error condition indicating the system is unusable.
    LP_STACKTRACE,  // A stacktrace info
    LP_MAXPRIO      // Maximum priority level (placeholder)
} LOG_PRIORITY;

extern clock_t _LOG_CLOCK;

extern int _USE_ANSI;

extern LOG_PRIORITY LOG_LEVEL;

extern const char* priority_names[];

extern const int priority_color[][3];

extern int priority_occurrence[];

extern void log_mute(void);

extern void log_unmute(void);

extern void log_enable_ansi(void);

extern void log_disable_ansi(void);

extern void log_msg(LOG_PRIORITY priority, const char* format, ...);

extern void log_msg_inline(LOG_PRIORITY priority, const char* format, ...);

extern void log_msg_sparse(LOG_PRIORITY priority, double time, const char* format, ...);

extern void log_count(void);

#endif

/*
USECASE:

log_msg(LP_ERROR, "Null pointer :: \"%s\" at line %d", __FILE__, __LINE__);
*/
