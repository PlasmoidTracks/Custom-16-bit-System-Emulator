//#include <bits/pthreadtypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <pthread.h>

#include "Log.h"
#include "StackTracer.h"

int _stacktrace_depth_ = 0;
char _stacktrace_descriptor_[STACKTRACER_MAX_DEPTH][STACKTRACER_MAX_DESCRIPTOR];  // Stores the full formatted trace messages

int _stacktrace_mute_recursion_ = 1;
int _stacktrace_mute_mark_ = 1;
int _stacktrace_mute_all_ = 0;
static int _stacktrace_mute_ = 0;

pthread_mutex_t _stacktrace_mutex_lock_ = PTHREAD_MUTEX_INITIALIZER;

void stacktrace_mark(const char* format, ...) {
    // Check if the stack depth is within the allowed maximum
    pthread_mutex_lock(&_stacktrace_mutex_lock_);
    if (_stacktrace_depth_ < STACKTRACER_MAX_DEPTH) {
        va_list args;

        // If format is NULL, provide a default message
        if (format == NULL) {
            snprintf(_stacktrace_descriptor_[_stacktrace_depth_], sizeof(_stacktrace_descriptor_[_stacktrace_depth_]), "Trace %d", _stacktrace_depth_);
        } else {
            // Initialize a variable argument list to format the description
            va_start(args, format);

            // Perform formatted string writing
            int written = vsnprintf(_stacktrace_descriptor_[_stacktrace_depth_], sizeof(_stacktrace_descriptor_[_stacktrace_depth_]), format, args);
            va_end(args);

            // Check if the string was truncated
            if ((written < 0 || written >= (int) sizeof(_stacktrace_descriptor_[_stacktrace_depth_])) && !_stacktrace_mute_all_) {
                log_msg(LP_WARNING, "Trace description at depth %d was truncated. Maximum length is %zu characters", _stacktrace_depth_, sizeof(_stacktrace_descriptor_[_stacktrace_depth_]) - 1);

                // Ensure the string is null-terminated
                _stacktrace_descriptor_[_stacktrace_depth_][sizeof(_stacktrace_descriptor_[_stacktrace_depth_]) - 1] = '\0';
            }
        }
        if (!_stacktrace_mute_mark_ && !_stacktrace_mute_all_) {
            log_msg(LP_VERBOSE, _stacktrace_descriptor_[_stacktrace_depth_]);
        }
    } else {
        stacktrace_print();
        exit(1);
        log_msg(LP_WARNING, "traces beyond a depth of %d are not traced. Current depth: %d", STACKTRACER_MAX_DEPTH, _stacktrace_depth_);
    }
    _stacktrace_depth_++;
    pthread_mutex_unlock(&_stacktrace_mutex_lock_);
}

void stacktrace_free(void) {
    pthread_mutex_lock(&_stacktrace_mutex_lock_);
    if (_stacktrace_depth_ <= 0 && !_stacktrace_mute_all_) {
        log_msg(LP_ERROR, "trace depth is negative. There must be a mismatch between 'stacktrace_mark' and 'stacktrace_free'");
        stacktrace_print();
        exit(-1);
    }
    _stacktrace_depth_--;
    if (_stacktrace_depth_ == 0)
        _stacktrace_mute_ = 0;
    pthread_mutex_unlock(&_stacktrace_mutex_lock_);
}

void stacktrace_print(void) {
    // Print the stack trace in reverse order (most recent first)
    int start_index = (_stacktrace_depth_ < STACKTRACER_MAX_DEPTH) ? _stacktrace_depth_ - 1 : STACKTRACER_MAX_DEPTH - 1;
    
    if (start_index < 0 && !_stacktrace_mute_all_) {
        log_msg(LP_STACKTRACE, ": No Trace found");
    }

    /*for (int i = start_index; i >= 0; i--) {
        // Print each stored trace message
        log_msg(LP_STACKTRACE, "[%d]: %s", i, _stacktrace_descriptor_[i]);
    }*/
    if (!_stacktrace_mute_all_) {
        if (!_stacktrace_mute_) {
            for (int i = 0; i <= start_index; i++) {
                // Print each stored trace message
                log_msg_inline(LP_STACKTRACE, "[%d]: ", i);
                for (int _ = 1; _ < i; _++) 
                    printf("    ");
                if (i > 0) 
                    printf("+-- ");
                printf("%s\n", _stacktrace_descriptor_[i]);
            }
            if (_stacktrace_mute_recursion_)
                _stacktrace_mute_ = 1;
        } else {
            log_msg(LP_VERBOSE, "Stacktrace muted. This usually occures when the calling functions recursively call stacktrace_print. To view the muted stacktrace calls, use the function stacktrace_unmute_recursion. ");
        }
    }
}

void stacktrace_panic(void) {
    //printf("\n\t!!!PANIC!!!\n");
    stacktrace_print();
    //exit(-1);
}

void stacktrace_mute_recursion(void) {
    _stacktrace_mute_recursion_ = 1;
}
void stacktrace_unmute_recursion(void) {
    _stacktrace_mute_recursion_ = 0;
}

void stacktrace_mute_mark(void) {
    _stacktrace_mute_mark_ = 1;
}
void stacktrace_unmute_mark(void) {
    _stacktrace_mute_mark_ = 0;
}

void stacktrace_mute(void) {
    _stacktrace_mute_all_ = 1;
}
void stacktrace_unmute(void) {
    _stacktrace_mute_all_ = 0;
}
