#ifndef _LOG_H
#define _LOG_H

#include <stdio.h>
#include <string>
#include <iostream>

using namespace std;

#define printLog(target, type, content)     CLog::getInstance()->recordLog(target, type, __FILE__, __func__, __LINE__, content)

typedef struct _log_info
{
    char log_path[64];
    FILE *log_fp;
} T_LogInfo, *PT_LogInfo; 

class CLog
{
public:
    enum _log_type          // 日志类型
    {
        DEBUG = 0,          // 调试
        INFO,               // 正常信息
        WARNING,            // 警告
        ERROR,              // 错误
        INVALID,
    };

    enum _log_target        // 日志打印出处
    {
        TERMINAL = 0,       // 终端
        LOGFILE,            // 日志文件
        FILE_AND_TERMINAL,  // 终端和日志文件
    };
    static CLog *getInstance();
    
    string getCurSysTime();
    static void initLogFile();
    bool recordLog(int log_target, int log_type, const char *pcFile, const char *pcFunc,
                    unsigned int nLineNum, string content);
    void uninitLogFile();

private:
    CLog();
    static pthread_mutex_t m_Mutex;
    static T_LogInfo m_tLogInfo;
    static inline std::string inttostr(int num) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", num);
        return std::string(buf);
    }
};



#endif
