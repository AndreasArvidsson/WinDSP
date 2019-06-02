#include "WinDSPLog.h"
#include <cstdarg>
#include "Date.h"
#include <fstream> // std::ofstream

#define LOG_FILE "WinDSP_log.txt"

#define BUFFER_SIZE 512

std::deque<LogLine> WinDSPLog::_buffer;
SpinLock WinDSPLog::_lock;
std::thread::id WinDSPLog::_mainThreadId;
bool WinDSPLog::_logToFile = false;

void WinDSPLog::init() {
    _mainThreadId = std::this_thread::get_id();
    _logToFile = true;
    if (_logToFile) {
        std::ofstream outfile;
        outfile.open(LOG_FILE, std::ios_base::app);
        outfile << "\n";
    }
}

void WinDSPLog::log(const LogSeverity severity, const char * const fileName, const unsigned int lineNumber, const char *const str, ...) {
    //Apply argument to user string.
    va_list ap;
    char text[BUFFER_SIZE];
    va_start(ap, str);
    vsnprintf(text, BUFFER_SIZE, str, ap);
    va_end(ap);

    if (_mainThreadId == std::this_thread::get_id()) {
        logLine(severity, Date::getLocalDateTimeString(), fileName, lineNumber, text);
    }
    else {
        _lock.lock();
        _buffer.push_back(LogLine(severity, Date::getLocalDateTimeString(), fileName, lineNumber, text));
        _lock.unlock();
    }
}

void WinDSPLog::flush() {
    for (;;) {
        _lock.lock();
        if (_buffer.size()) {
            logLine(_buffer.front().severity, _buffer.front().timestamp, _buffer.front().fileName, _buffer.front().lineNumber, _buffer.front().text);
            _buffer.pop_front();
            _lock.unlock();
        }
        else {
            _lock.unlock();
            return;
        }
    }
}

void WinDSPLog::logLine(const LogSeverity severity, const std::string &timestamp, const std::string &fileName, const unsigned int lineNumber, const std::string &text) {
    switch (severity) {
    case LogSeverity::DEBUG:
        printf("%s: %s\n", timestamp.c_str(), text.c_str());
        break;
    case LogSeverity::INFO:
        printf("%s\n", text.c_str());
        break;
    case LogSeverity::WARN:
    case LogSeverity::ERR:
        printf("%s\n\n", text.c_str());
        break;
    }
    if (_logToFile) {
        std::ofstream outfile;
        outfile.open(LOG_FILE, std::ios_base::app);
        outfile << timestamp << " | " << getSeverityText(severity) << " | " << fileName << "(" << lineNumber << ")" << " | " << text << "\n";
        switch (severity) {
        case LogSeverity::WARN:
        case LogSeverity::ERR:
            outfile << timestamp << " | " << getSeverityText(severity) << " | " << fileName << "(" << lineNumber << ")" << " | " << "\n";
            break;
        }
    }
}

const std::string WinDSPLog::getSeverityText(const LogSeverity severity) {
    switch (severity) {
    case LogSeverity::DEBUG:
        return "DEBUG";
    case LogSeverity::INFO:
        return "INFO ";
    case LogSeverity::WARN:
        return "WARN ";
    case LogSeverity::ERR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}