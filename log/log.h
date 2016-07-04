#ifndef __LOG_H__
#define __LOG_H__

#include <string>
#include <fstream>
#include <iostream>
using namespace std;

namespace b_log
{

	class log
	{
	public:
		typedef enum
		{ 
			LOGCRIT=0, LOGERROR, LOGWARNING, LOGINFO,
		  	LOGDEBUG, LOGDEBUG2, LOGALL
		} log_level;
	public:
		log( const std::string& log_file, log_level level );
		virtual ~log();

		void write( log_level level, const char* format, ... );

	private:
		std::ofstream m_log_file;
		log_level m_log_level;
	};	
}


// typedef enum
// { RTMP_LOGCRIT=0, RTMP_LOGERROR, RTMP_LOGWARNING, RTMP_LOGINFO,
//   RTMP_LOGDEBUG, RTMP_LOGDEBUG2, RTMP_LOGALL
// } RTMP_LogLevel;

// extern RTMP_LogLevel RTMP_debuglevel;

// typedef void (RTMP_LogCallback)(int level, const char *fmt, va_list);
// void RTMP_LogSetCallback(RTMP_LogCallback *cb);
// void RTMP_LogSetOutput(FILE *file);
// #ifdef __GNUC__
// void RTMP_LogPrintf(const char *format, ...) __attribute__ ((__format__ (__printf__, 1, 2)));
// void RTMP_LogStatus(const char *format, ...) __attribute__ ((__format__ (__printf__, 1, 2)));
// void RTMP_Log(int level, const char *format, ...) __attribute__ ((__format__ (__printf__, 2, 3)));
// #else
// void RTMP_LogPrintf(const char *format, ...);
// void RTMP_LogStatus(const char *format, ...);
// void RTMP_Log(int level, const char *format, ...);
// #endif
// void RTMP_LogHex(int level, const uint8_t *data, unsigned long len);
// void RTMP_LogHexString(int level, const uint8_t *data, unsigned long len);
// void RTMP_LogSetLevel(RTMP_LogLevel lvl);
// RTMP_LogLevel RTMP_LogGetLevel(void);

// #ifdef __cplusplus
// }
// #endif

#endif
