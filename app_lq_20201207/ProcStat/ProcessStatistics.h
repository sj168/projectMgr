#ifndef PROCESSSTATISTICS_H
#define PROCESSSTATISTICS_H

#include <pthread.h>
#include <mutex>

#include "CmdBase.h"
#include "CmdDef.h"
#include "CmdManager.h"
#include "SystemMessageBase.h"
#include "Inifile.h"

typedef struct StatParam
{
    float fRate;
    unsigned int nRateCnt;
    unsigned int nRateCalCnt;
    float fTotalTimeElapsed;
    bool bCalResult;

} T_StatParam, * PT_StatParam;

typedef struct ProcParam
{
    bool bEnableProcStat;
    uint64_t frame_n;           // 統計幀數
    T_StatParam tGrabRate;
    T_StatParam tProcRate;
    T_StatParam tAvgProcRate;

} T_ProcParam, * PT_ProcParam;

class CProcStat : public CCmdBase, public CSysMsgBase
{
public:
    static CProcStat * getInstance();
    void calGrabRate(uint64_t frame_n);
    void resetGrabRate();
    void calProcRate();
    void calAvgProcTime();
    virtual int cmdFunction(unsigned char ucCmdSubId, const unsigned char * pucCmdDataPkg, unsigned int nDataLen);
    virtual int handleSystemMessage(T_MsgParam * ptMsgParam);

    ~CProcStat();

private:
    CProcStat();
    void initSigAction();
    void startTimer();
    void stopTimer();
    // static void * handleProcStat(void * pvArg);
    static void timeout(int iSigNum);

    static T_ProcParam m_tProcParam;
    static pthread_t m_tProcStatThread;
    static bool m_bRunning;
    static std::mutex m_Mutex;
    CCmdManager *m_pclsCmdMgr;
};

#endif // PROCESSSTATISTICS_H
