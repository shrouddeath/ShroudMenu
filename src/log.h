#pragma once
#include <string>

namespace logfile {

void open(const std::string& path);
void setLevel(int level);          // 0 = quiet, 1 = normal, 2 = debug
int level();
void write(const char* fmt, ...);  // level >= 1
void debug(const char* fmt, ...);  // level >= 2

} // namespace logfile
