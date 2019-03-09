#pragma once
#include "Log.h"

//#define DEBUG_LOG

#undef LOG_INFO
#undef LOG_WARN
#undef LOG_ERROR
#undef LOG_NL

#ifdef DEBUG_LOG
#define LOG_FILE "WinDSP_log.txt"
#define LOG_INFO(str, ...)	printf(str,  ##__VA_ARGS__); printf("\n");	__LOG_INFO__(str,  ##__VA_ARGS__)
#define LOG_WARN(str, ...)	printf(str,  ##__VA_ARGS__); printf("\n");	__LOG_WARN__(str,  ##__VA_ARGS__)
#define LOG_ERROR(str, ...) printf(str,  ##__VA_ARGS__); printf("\n");	__LOG_ERROR__(str,  ##__VA_ARGS__)
#define LOG_NL()			printf("\n");								__LOG_NL__()
#else
#define LOG_INFO(str, ...)	printf(str,  ##__VA_ARGS__); printf("\n")
#define LOG_WARN LOG_INFO
#define LOG_ERROR LOG_INFO
#define LOG_NL()			printf("\n");
#endif

namespace WinDSPLog {

    void configure();

}

