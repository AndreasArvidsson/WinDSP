#pragma once
#include <deque>
#include <thread>
#include <string>
#include "SpinLock.h"

using std::string;
using std::deque;
using std::unique_ptr;
using std::thread;

#define LOG_FILE "WinDSP_log.txt"

enum class LogSeverity {
    S_DEBUG, S_INFO, S_WARN, S_ERROR
};

class LogLine {
public:
    LogSeverity severity;
    string timestamp, fileName, text;
    unsigned int lineNumber;

    LogLine(const LogSeverity severity, const string &timestamp, const string &fileName, const unsigned int lineNumber, const string &text) :
        severity(severity), timestamp(timestamp), fileName(fileName), lineNumber(lineNumber), text(text) {}

};

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__))

#undef LOG_INFO
#undef LOG_WARN
#undef LOG_ERROR
#undef LOG_NL

#define LOG_DEBUG(str, ...) WinDSPLog::log(LogSeverity::S_DEBUG, __FILENAME__, __LINE__, str,  ##__VA_ARGS__)
#define LOG_INFO(str, ...)  WinDSPLog::log(LogSeverity::S_INFO, __FILENAME__, __LINE__, str,  ##__VA_ARGS__)
#define LOG_WARN(str, ...)  WinDSPLog::log(LogSeverity::S_WARN, __FILENAME__, __LINE__, str,  ##__VA_ARGS__)
#define LOG_ERROR(str, ...) WinDSPLog::log(LogSeverity::S_ERROR, __FILENAME__, __LINE__, str,  ##__VA_ARGS__)
#define LOG_NL()            WinDSPLog::log(LogSeverity::S_INFO, __FILENAME__, __LINE__, "")

class WinDSPLog {
public:
    static void init();
    static void destroy();
    static void setLogToFile(const bool logToFile);
    static void log(const LogSeverity severity, const string &fileName, const unsigned int line, const char * const str, ...);
    static void flush();

private:
    static unique_ptr<deque<LogLine>> _pBuffer;
    static SpinLock _lock;
    static thread::id _mainThreadId;
    static bool _logToFile;

    static void logLine(const LogSeverity severity, const string &timestamp, const string &fileName, const unsigned int line, const string &text);
    static const string getSeverityText(const LogSeverity severity);

};

