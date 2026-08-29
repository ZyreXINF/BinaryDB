#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include "paths.h"

int get_executable_dir(char *out, size_t out_size)
{
#ifdef _WIN32
    DWORD len = GetModuleFileNameA(NULL, out, (DWORD)out_size);
    if (len == 0 || len == out_size) {return -1;}
#else
    ssize_t len = readlink("/proc/self/exe", out, out_size - 1);
    if (len == -1) {return -1;}
    out[len] = '\0';
#endif

    char *slash = strrchr(out, '/');
    char *bslash = strrchr(out, '\\');
    if (bslash != NULL && (slash == NULL || bslash > slash)) {slash = bslash;}
    if (slash != NULL) {*slash = '\0';}

    return 0;
}