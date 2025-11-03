#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#include <utils/Log.h>

clock_t _LOG_CLOCK = 0;

int _USE_ANSI = 1;
int _LOG_MUTE = 0;
int _LOG_TO_FILE = 0;

LOG_PRIORITY LOG_LEVEL = LP_MINPRIO;

const char* priority_names[] = {
    "MINPRIO",
    "DEBUG",
    "DEPRECATED", 
    "VERBOSE", 
    "INFO",
    "SUCCESS", 
    "NOTICE",
    "TODO", 
    "WARNING",
    "ERROR",
    "CRITICAL",
    "EMERGENCY",
    "STACKTRACE", 
    "MAXPRIO"
};

const int priority_color[][3] = {
    {255, 255, 255}, 
    {128, 128, 128}, 
    {255, 255, 255},    // need a different color 
    {128, 160, 176}, 
    {128, 192, 224}, 
    {64, 224, 64}, 
    {255, 255, 255}, 
    {128, 80, 16}, 
    {255, 255, 0}, 
    {255, 128, 128}, 
    {255, 0, 0}, 
    {160, 0, 255}, 
    {255, 0, 255}, 
    {255, 255, 255}
};

int priority_occurrence[LP_MAXPRIO + 1] = {0};

void log_mute(void) {
    _LOG_MUTE = 1;
}

void log_unmute(void) {
    _LOG_MUTE = 0;
}

void log_enable_ansi(void) {
    _USE_ANSI = 1;
}

void log_disable_ansi(void) {
    _USE_ANSI = 0;
}

void write_to_log_file(const char* timestamp, const char* priority_name, const char* message) {
    FILE* file = fopen("log.txt", "a");
    if (file) {
        fprintf(file, "%s [%s] %s\n", timestamp, priority_name, message);
        fclose(file);
    }
}

void log_msg(LOG_PRIORITY priority, const char* format, ...) {
    if (_LOG_MUTE) return;
    if (priority < LP_MINPRIO || priority > LP_MAXPRIO || format == NULL) {
        fprintf(stderr, "Invalid log priority or NULL format string.\n");
        return;
    }

    priority_occurrence[priority]++;

    if (priority >= LOG_LEVEL) {
        va_list args;
        char buffer[1024];
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        time_t now = time(NULL);
        char time_buffer[20];
        strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", localtime(&now));

        if (_USE_ANSI) {
            printf("%s [\033[0;38;2;%d;%d;%dm%s\033[0m] %s\n", 
                time_buffer, 
                priority_color[priority][0], 
                priority_color[priority][1], 
                priority_color[priority][2], 
                priority_names[priority], 
                buffer);
        } else {
            printf("%s [%s] %s\n", 
                time_buffer, 
                priority_names[priority], 
                buffer);
        }

        if (_LOG_TO_FILE) {
            write_to_log_file(time_buffer, priority_names[priority], buffer);
        }
    }
}

void log_msg_inline(LOG_PRIORITY priority, const char* format, ...) {
    if (_LOG_MUTE) return;
    if (priority < LP_MINPRIO || priority > LP_MAXPRIO || format == NULL) {
        fprintf(stderr, "Invalid log priority or NULL format string.\n");
        return;
    }

    priority_occurrence[priority]++;

    if (priority >= LOG_LEVEL) {
        va_list args;
        char buffer[1024];
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        time_t now = time(NULL);
        char time_buffer[20];
        strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", localtime(&now));

        if (_USE_ANSI) {
            printf("%s [\033[0;38;2;%d;%d;%dm%s\033[0m] %s", 
                time_buffer, 
                priority_color[priority][0], 
                priority_color[priority][1], 
                priority_color[priority][2], 
                priority_names[priority], 
                buffer);
        } else {
            printf("%s [%s] %s", 
                time_buffer, 
                priority_names[priority], 
                buffer);
        }

        if (_LOG_TO_FILE) {
            write_to_log_file(time_buffer, priority_names[priority], buffer);
        }
    }
}

void log_msg_sparse(LOG_PRIORITY priority, double time, const char* format, ...) {
    if (_LOG_MUTE) return;
    if (_LOG_CLOCK == 0) {
        _LOG_CLOCK = clock();
    }

    clock_t t = clock();
    if ((double)(t - _LOG_CLOCK) / CLOCKS_PER_SEC < time) return;

    _LOG_CLOCK = t;

    va_list args;
    char buffer[1024];
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    log_msg(priority, "%s", buffer);
}

void log_count(void) {
    for (int i = 1; i < LP_MAXPRIO; i++) {
        if (!priority_occurrence[i]) continue;
        if (_USE_ANSI) {
            printf("[\033[0;38;2;%d;%d;%dm%s\033[0m] : %d\n", 
                priority_color[i][0], 
                priority_color[i][1], 
                priority_color[i][2], 
                priority_names[i], 
                priority_occurrence[i]);
        } else {
            printf("[%s] : %d\n", 
                priority_names[i], 
                priority_occurrence[i]);
        }
    }
}


/*
setting LOG_LEVEL to one of the Log priorities, the log_msg function will only log messages of higher or equal priority as the log level
*/
