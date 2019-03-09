#include "WinDSPLog.h"

void WinDSPLog::configure() {
#ifdef DEBUG_LOG
    Log::logToPrint(false);
    Log::logToFile(LOG_FILE);
    Log::clearFile();
#endif
}