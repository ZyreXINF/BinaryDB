#ifndef LOGGER_H
#define LOGGER_H

typedef enum
{
    LOG_DATABASE,
    LOG_ERROR
} LogCategory;

void log_write(LogCategory category, const char *format, ...);

#endif