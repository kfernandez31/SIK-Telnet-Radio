#include "log.hh"

#include <sys/stat.h>
#include <cstddef>
#include <cstdarg>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <ctime>

#define TIMESTAMP_LEN   21
#define LOGS_DIR       "logs/"
#define LOGFILE_PREFIX "radio-"
#define LOGFILE_SUFFIX ".log"
#define LOGFILE_LEN    (sizeof(LOGS_DIR LOGFILE_PREFIX LOGFILE_SUFFIX) + TIMESTAMP_LEN - 1)

struct {
    FILE* logfile;
} logger; // a singleton instance

struct log_level_pretty_info_t {
    const char * const name;
    const char * const color;
};

static const log_level_pretty_info_t pretty[] = {
  {"TRACE", "\x1b[94m"},
  {"DEBUG", "\x1b[36m"},
  {"INFO",  "\x1b[32m"},
  {"WARN",  "\x1b[33m"},
  {"ERROR", "\x1b[31m"},
  {"FATAL", "\x1b[35m"},
};

LogMsg* new_log_msg(const log_level_t level, const char * const file, const size_t line, const char * const fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int length = vsnprintf(NULL, 0, fmt, args); // a hack to get the length of the format string, if it were printed
    if (length < 0) {
        perror("vsnprintf");
        exit(1);
    }
    va_end(args);
    length++;

    LogMsg* msg = (LogMsg*)malloc(sizeof(LogMsg) + length * sizeof(char));
    if (!msg) {
        perror("malloc");
        exit(1);
    }

    time_t now  = time(NULL);
    msg->time   = localtime(&now);
    if (!msg->time) {
        perror("localtime");
        exit(1);
    }
    msg->level = level;
    msg->file  = file; // this has a static storage duration, no need for strdup
    msg->line  = line;

    va_start(args, fmt);
    if (vsnprintf(msg->contents, length, fmt, args) < 0) {
        perror("vsnprintf");
        exit(1);
    }
    va_end(args);

    return msg; // don't forget to free!
}

void free_log_msg(LogMsg* msg) {
    free(msg);
}

void print_log_msg(LogMsg* msg) {
    char time_buf[TIMESTAMP_LEN + 1] = {'\0'};
    strftime(time_buf, sizeof(time_buf), "[%Y-%m-%d %H:%M:%S]", msg->time);
    if (logger.logfile == stderr)
        fprintf(logger.logfile, "%s %s%-5s\x1b[0m \x1b[90m%s:%zu:\x1b[0m %s\n",
            time_buf, pretty[msg->level].color, pretty[msg->level].name, msg->file, msg->line, msg->contents);
    else
        fprintf(logger.logfile, "%s %-5s %s:%zu: %s\n",
            time_buf,                           pretty[msg->level].name, msg->file, msg->line, msg->contents);
    fflush(logger.logfile);
    free_log_msg(msg);
}

void logger_init(const bool log_to_file) {
    if (!log_to_file)
        logger.logfile = stderr;
    else {
        struct stat st = {0};
        if (stat(LOGS_DIR, &st) < 0)
            mkdir(LOGS_DIR, 0777);
        time_t now          = time(NULL);
        struct tm* loc_time = localtime(&now);
        char filename[LOGFILE_LEN] = {'\0'};
        strftime(filename, LOGFILE_LEN, LOGS_DIR LOGFILE_PREFIX "%Y-%m-%d-%H-%M-%S" LOGFILE_SUFFIX, loc_time);
        if (!(logger.logfile = fopen(filename, "a"))) {
            perror("fopen");
            exit(1);
        }
        else
            fprintf(stderr, "opened new log file: %s\n", filename);
    }
}

void logger_destroy() {
    if (logger.logfile != stderr && fclose(logger.logfile) < 0) {
        perror("fclose");
        exit(1);
    }
}
