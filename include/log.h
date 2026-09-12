#ifndef LOG_H
#define LOG_H

#include <map>
#include <string>

enum class LogLevel { kInfo, kWarning, kError };

extern std::map<std::string, LogLevel> output_log;

#endif  // LOG_H
