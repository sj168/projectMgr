#include "Log.h"
#include <sys/timeb.h>
#include <time.h>
#include "Inifile.h"

T_LogInfo CLog::m_tLogInfo = {0};
pthread_mutex_t CLog::m_Mutex;

CLog *CLog::getInstance()
{
    static CLog clsLog;
    return &clsLog;
}

string CLog::getCurSysTime()
{
    char curTime[64];
    time_t pTime;
    time(&pTime);
    strftime(curTime, sizeof(curTime), "%Y-%m-%d %H:%M:%S", localtime(&pTime));

    return curTime;
}

void CLog::initLogFile()
{
    read_profile_string("setting", "LogFilePath", m_tLogInfo.log_path, sizeof (m_tLogInfo.log_path), "/home/tye/autoStart/log", FILE_INI_NAME);

    if(m_tLogInfo.log_fp == nullptr)
    {
        m_tLogInfo.log_fp = fopen(m_tLogInfo.log_path, "a");
        if(m_tLogInfo.log_fp == nullptr)
        {
            printf("Log file created failed.\n");
            return;
        }
    }
}

bool CLog::recordLog(int log_target, int log_type, const char *pcFile, const char *pcFunc,
                    unsigned int nLineNum, string content)
{
    const char *log_ct;
    string space = " ";
    string act_level;
    string output_content;
    string act_file = pcFile;
    string act_func = pcFunc;
    string act_line = inttostr(nLineNum);

    pthread_mutex_lock(&m_Mutex);
    switch(log_type)
    {
        case DEBUG:
        {
            act_level = "[DEBUG  ]";
            break;
        }
        case INFO:
        {
            act_level = "[INFO   ]";
            break;
        }
        case WARNING:
        {
            act_level = "[WARNING]";
            break;
        }
        case ERROR:
        {
            act_level = "[ERROR  ]";
            break;
        }
        default:
        {
            act_level = "[INVALID]";
            break;
        }
    }

    output_content = act_level + space + getCurSysTime() + space + act_file + space + act_func + space + act_line + space + content;
    log_ct = output_content.c_str();
    switch(log_target)
    {
        case TERMINAL:
        {
            printf("%s\n", log_ct);
            break;
        }
        case LOGFILE:
        {
            if(m_tLogInfo.log_fp)
            {
                fprintf(m_tLogInfo.log_fp, "%s\n", log_ct);
                fflush(m_tLogInfo.log_fp);
            }
            break;
        }
        case FILE_AND_TERMINAL:
        {
            printf("%s\n", log_ct);
            if(m_tLogInfo.log_fp)
            {
                fprintf(m_tLogInfo.log_fp, "%s\n", log_ct);
                fflush(m_tLogInfo.log_fp);
            }

            break;
        }
        default:
            cout << "Invalid Target";
            break;
    }

    pthread_mutex_unlock(&m_Mutex);

    return true;
}

/*
bool CLog::recordLog(const char *format, ...)
{
    va_list args;
    va_start(args, logContent);
    char ch;
    while(ch = *format++)
    {
        switch(*format)
        {
            case DEBUG:
            {
                act_level = "[DEBUG  ]";
                break;
            }
            case INFO:
            {
                act_level = "[INFO   ]";
                break;
            }
            case WARNING:
            {
                act_level = "[WARNING]";
                break;
            }
            case ERROR:
            {
                act_level = "[ERROR  ]";
                break;
            }
            default:
            {
                act_level = "[INVALID]";
                break;
            }
        }

    }
    vprintf(logContent, args);
    va_end(args); 

    const char *log_ct;
    const char *space = " ";
    const char *act_level;
    const char *output_content;

    string act_line = inttostr(nLineNum);
    char logContent[1024];

    pthread_mutex_lock(&m_Mutex);
    switch(log_type)
    {
        case DEBUG:
        {
            act_level = "[DEBUG  ]";
            break;
        }
        case INFO:
        {
            act_level = "[INFO   ]";
            break;
        }
        case WARNING:
        {
            act_level = "[WARNING]";
            break;
        }
        case ERROR:
        {
            act_level = "[ERROR  ]";
            break;
        }
        default:
        {
            act_level = "[INVALID]";
            break;
        }
    }

    sprintf(logContent, "%s %s %s %d %s", act_level, pcFile, pcFunc, nLineNum, format);

    va_list args;
    va_start(args, logContent);
    vprintf(logContent, args);
    va_end(args);

    output_content = act_level + space + getCurSysTime() + space + act_file + space + act_func + space + act_line + space + content;
    log_ct = output_content.c_str();
    switch(log_target)
    {
        case TERMINAL:
        {
            printf("%s\n", log_ct);
            break;
        }
        case LOGFILE:
        {
            if(m_tLogInfo.log_fp)
            {
                fprintf(m_tLogInfo.log_fp, "%s \n", log_ct);
                fflush(m_tLogInfo.log_fp);
            }
            break;
        }
        case FILE_AND_TERMINAL:
        {
            printf("%s\n", log_ct);
            if(m_tLogInfo.log_fp)
            {
                fprintf(m_tLogInfo.log_fp, "%s\n", log_ct);
                fflush(m_tLogInfo.log_fp);
            }

            break;
        }
        default:
            cout << "Invalid Target";
            break;
    }

    pthread_mutex_unlock(&m_Mutex);

    return true;
}
*/
void CLog::uninitLogFile()
{
    if(m_tLogInfo.log_fp)
    {
        fclose(m_tLogInfo.log_fp);
        m_tLogInfo.log_fp = nullptr;
    }
}

CLog::CLog()
{
}
