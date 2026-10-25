#include "log.h"

#include <cstdarg>
#include <cstdio>
#include <ctime>

namespace logfile {

static FILE* g_file = nullptr;
static int g_level = 1;

void open(const std::string& path) {
    if (g_file) fclose(g_file);
    g_file = fopen(path.c_str(), "w");
}

void setLevel(int level) { g_level = level; }
int level() { return g_level; }

static void writeLine(const char* fmt, va_list args) {
    if (!g_file) return;
    time_t now = time(nullptr);
    tm local{};
    localtime_s(&local, &now);
    fprintf(g_file, "[%02d:%02d:%02d] ", local.tm_hour, local.tm_min, local.tm_sec);
    vfprintf(g_file, fmt, args);
    fputc('\n', g_file);
    fflush(g_file);
}

void write(const char* fmt, ...) {
    if (g_level < 1) return;
    va_list args;
    va_start(args, fmt);
    writeLine(fmt, args);
    va_end(args);
}

void debug(const char* fmt, ...) {
    if (g_level < 2) return;
    va_list args;
    va_start(args, fmt);
    writeLine(fmt, args);
    va_end(args);
}

} // namespace logfile
