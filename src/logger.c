#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif
#include "paths.h"
#include "logger.h"

#define LOG_FOLDER "logs"

static const char *log_category_filename(LogCategory category)
{
    switch (category)
    {
    case LOG_DATABASE: return "database.log";
    case LOG_ERROR:    return "error.log";
    default:           return "misc.log";
    }
}

void log_write(LogCategory category, const char *format, ...)
{
    char exe_dir[512];
    if (get_executable_dir(exe_dir, sizeof(exe_dir)) != 0) {return;}

    char folder[560];
    snprintf(folder, sizeof(folder), "%s/%s", exe_dir, LOG_FOLDER);

#ifdef _WIN32
    _mkdir(folder);
#else
    mkdir(folder, 0755);
#endif

    char path[620];
    snprintf(path, sizeof(path), "%s/%s", folder, log_category_filename(category));

    FILE *log_file = fopen(path, "a");
    if (log_file == NULL) {return;} /* logging must never crash the program */

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    fprintf(log_file, "[%s] ", timestamp);

    va_list args;
    va_start(args, format);
    vfprintf(log_file, format, args);
    va_end(args);

    fprintf(log_file, "\n");

    fclose(log_file);
}