#include "WinDSPLog.h"
#include <cstdarg>
#include "Date.h"
#include <fstream> // ofstream

using std::make_unique;
using std::ofstream;
using std::ios_base;
using std::this_thread::get_id;

#define BUFFER_SIZE 512

unique_ptr<deque<LogLine>> WinDSPLog::_pBuffer;
SpinLock WinDSPLog::_lock;
thread::id WinDSPLog::_mainThreadId;
bool WinDSPLog::_logToFile = false;

void WinDSPLog::init() {
    _pBuffer = make_unique<deque<LogLine>>();
    _mainThreadId = get_id();
    _logToFile = false;
}

void WinDSPLog::destroy() {
    _pBuffer = nullptr;
}

void WinDSPLog::setLogToFile(const bool logToFile) {
    _logToFile = logToFile;
    // Start with empty row.
    if (logToFile) {
        ofstream outfile;
        outfile.open(LOG_FILE, ios_base::app);
        outfile << "\n";
    }
}

void WinDSPLog::log(const LogSeverity severity, const string& fileName, const unsigned int lineNumber, const char* const str, ...) {
    // Apply argument to user string.
    va_list ap;
    char text[BUFFER_SIZE];
    va_start(ap, str);
    vsnprintf(text, BUFFER_SIZE, str, ap);
    va_end(ap);
    if (_mainThreadId == get_id()) {
        logLine(severity, Date::toLocalDateTimeString(), fileName, lineNumber, text);
    }
    else {
        _lock.lock();
        _pBuffer->push_back(LogLine(severity, Date::toLocalDateTimeString(), fileName, lineNumber, text));
        _lock.unlock();
    }
}

void WinDSPLog::flush() {
    for (;;) {
        _lock.lock();
        if (_pBuffer->size()) {
            logLine(_pBuffer->front().severity, _pBuffer->front().timestamp, _pBuffer->front().fileName, _pBuffer->front().lineNumber, _pBuffer->front().text);
            _pBuffer->pop_front();
            _lock.unlock();
        }
        else {
            _lock.unlock();
            return;
        }
    }
}

void WinDSPLog::logLine(const LogSeverity severity, const string& timestamp, const string& fileName, const unsigned int lineNumber, const string& text) {
    switch (severity) {
    case LogSeverity::S_DEBUG:
        printf("%s | %s | %s\n", timestamp.c_str(), getSeverityText(severity).c_str(), text.c_str());
        break;
    case LogSeverity::S_INFO:
        printf("%s\n", text.c_str());
        break;
    case LogSeverity::S_WARN:
    case LogSeverity::S_ERROR:
        printf("%s\n\n", text.c_str());
        break;
    }
    if (_logToFile) {
        ofstream outfile;
        outfile.open(LOG_FILE, ios_base::app);
        outfile << timestamp << " | " << getSeverityText(severity) << " | " << fileName << "(" << lineNumber << ")" << " | " << text << "\n";
        switch (severity) {
        case LogSeverity::S_WARN:
        case LogSeverity::S_ERROR:
            outfile << timestamp << " | " << getSeverityText(severity) << " | " << fileName << "(" << lineNumber << ")" << " | " << "\n";
            break;
        }
    }
}

const string WinDSPLog::getSeverityText(const LogSeverity severity) {
    switch (severity) {
    case LogSeverity::S_DEBUG:
        return "DEBUG";
    case LogSeverity::S_INFO:
        return "INFO ";
    case LogSeverity::S_WARN:
        return "WARN ";
    case LogSeverity::S_ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}