#ifndef _STACKTRACER_H_
#define _STACKTRACER_H_

#define STACKTRACE_SAFEGUARD

#define STACKTRACER_MAX_DEPTH 64
#define STACKTRACER_MAX_DESCRIPTOR 1024

extern int _stacktrace_depth_;
extern char _stacktrace_descriptor_[STACKTRACER_MAX_DEPTH][STACKTRACER_MAX_DESCRIPTOR];
extern void* _stacktrace_info_[STACKTRACER_MAX_DEPTH];

extern int _stacktrace_mute_recursion_;
extern int _stacktrace_mute_mark_;
extern int _stacktrace_mute_all_;

/* 
this function sets a mark with a given name and optional information. 
this HAS to have a corresponding call to stacktrace_free at every exit point in a function
*/
extern void stacktrace_mark(const char* format, ...);

// this HAS to be called for every return point in a function
extern void stacktrace_free(void);

// this prints out all the stackframes and information
extern void stacktrace_print(void);

// this ends the process and prints out all the stackframes and information
extern void stacktrace_panic(void);

extern void stacktrace_mute_recursion(void);
extern void stacktrace_unmute_recursion(void);

extern void stacktrace_mute_mark(void);
extern void stacktrace_unmute_mark(void);

extern void stacktrace_mute(void);
extern void stacktrace_unmute(void);

#endif

/*
USAGE:

typedef struct {
    uint8_t signal;
} DATA;

void function(int i) {
    stacktrace_mark("function called with argument (%d) at line %d, in \"%s\"", i, __LINE__, __FILE__);       // implicitely
    if (i > 8) {
        stacktrace_free();
        return;
    } else if (i > 100) {
        stacktrace_panic();
    }
    f(i+1);
    stacktrace_free();
}
*/
