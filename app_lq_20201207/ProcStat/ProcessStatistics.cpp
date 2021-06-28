
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>

#include "ErrorCode.h"
#include "ProcessStatistics.h"

T_ProcParam CProcStat::m_tProcParam = {0};
pthread_t CProcStat::m_tProcStatThread;
bool CProcStat::m_bRunning = true;
std::mutex CProcStat::m_Mutex;

CProcStat * CProcStat::getInstance()
{
    static CProcStat clsProcStat;
    return &clsProcStat;
}

void CProcStat::calGrabRate(uint64_t frame_n)
{
    static struct timeval tv1;
    static struct timeval tv2;

    if (!m_tProcParam.bEnableProcStat)
    {
        return;
    }
    m_tProcParam.frame_n = frame_n;

    if (!m_tProcParam.tGrabRate.bCalResult)
    {
        gettimeofday(&tv1, nullptr);
        m_tProcParam.tGrabRate.bCalResult = true;
    }
    else
    {
        ++m_tProcParam.tGrabRate.nRateCnt;

        // 統計採集一定數量的幀所花費的時間
        if (m_tProcParam.tGrabRate.nRateCnt >= m_tProcParam.tGrabRate.nRateCalCnt)  
        {
            gettimeofday(&tv2, nullptr);
            m_tProcParam.tGrabRate.bCalResult = false;
            unsigned long long ullTimeElapsed = 1000000 * (tv2.tv_sec - tv1.tv_sec) + (tv2.tv_usec - tv1.tv_usec);

            m_tProcParam.tGrabRate.fTotalTimeElapsed = (static_cast<float>(ullTimeElapsed) / 1000000.0f);

            printf("fTotalTimeElapsed:%.2f\n", m_tProcParam.tGrabRate.fTotalTimeElapsed);
            std::lock_guard<std::mutex> lock(m_Mutex);
            m_tProcParam.tGrabRate.fRate = m_tProcParam.tGrabRate.nRateCnt / m_tProcParam.tGrabRate.fTotalTimeElapsed;
            m_tProcParam.tGrabRate.nRateCnt = 0;
        }
    }
    // 通过TCP上传到上位机
    m_pclsCmdMgr->makeCmdAndSend(eCILQDevProcStat, (unsigned char *)&m_tProcParam, sizeof(T_ProcParam), eDTMUdp);

    return;
}

void CProcStat::resetGrabRate()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_tProcParam.tGrabRate.bCalResult = false;
    m_tProcParam.tGrabRate.nRateCnt = 0;
}

int CProcStat::cmdFunction(unsigned char ucCmdSubId, const unsigned char * pucCmdDataPkg, unsigned int nDataLen)
{
    int iRet = eECOK;
    int iRetLen = 1;

    switch (ucCmdSubId)
    {
        case eSetStatisticsBaseVal:
        {
            if (nDataLen >= sizeof(unsigned int) && nullptr != pucCmdDataPkg)
            {
                unsigned int nBaseVal = 0;
                nBaseVal = *(unsigned int *)(pucCmdDataPkg + 1);

                std::lock_guard<std::mutex> lock(m_Mutex);
                m_tProcParam.tGrabRate.nRateCalCnt = nBaseVal;
            }
            else
                iRet = eECParam;

            break;
        }

        case eEnableLQDevProcStat:
        {
            if (nDataLen >= sizeof(unsigned char) && nullptr != pucCmdDataPkg)
            {
                m_tProcParam.bEnableProcStat = (pucCmdDataPkg[1] > 0);
                if (pucCmdDataPkg[1] == 0)
                {
                    stopTimer();
                }
                else
                {
                    startTimer();
                }

                resetGrabRate();
            }
            else
                iRet = eECParam;

            break;
        }

        default:
            iRet = eECParam;
    }

    unsigned char * pucRetBuf = new unsigned char[iRetLen];
    if (nullptr != pucRetBuf)
    {
        pucRetBuf[0] = (iRet == eECOK) ? eAckOk : eAckFailed;
        m_pclsCmdMgr->makeCmdAndSend(eCIAck, pucRetBuf, iRetLen, eDTMUdp);

        delete [] pucRetBuf;
    }

    return iRet;
}

int CProcStat::handleSystemMessage(T_MsgParam * ptMsgParam)
{
    if (nullptr == ptMsgParam)
        return eECParam;

    if (ptMsgParam->tMsgId.ucMsgSubId == eMSISCStartOrStopImageGrab)
    {
        resetGrabRate();
    }

    return eECOK;
}

CProcStat::~CProcStat()
{}

CProcStat::CProcStat()
{
    m_tProcParam.tGrabRate.nRateCalCnt = read_profile_int("setting", "CalcGrabPerFrame", 1000, FILE_INI_NAME);  // 每多少帧一统计采集帧率
    m_tProcParam.tProcRate.nRateCalCnt = read_profile_int("setting", "CalcProcPerFrame", 1000, FILE_INI_NAME);
    m_tProcParam.tAvgProcRate.nRateCalCnt = 1000;
    m_pclsCmdMgr = CCmdManager::getInstance();
    initSigAction();
}

void CProcStat::initSigAction()
{
    struct sigaction tSigAct;
    struct sigaction tSigActOld;

    tSigAct.sa_handler = timeout;
    tSigAct.sa_flags   = 0;
    sigemptyset(&tSigAct.sa_mask);
    sigaction(SIGALRM, &tSigAct, &tSigActOld);
}

void CProcStat::startTimer()
{
    // printf("start timer\n");
    struct itimerval itv;
    struct itimerval itvOld;

    itv.it_interval.tv_sec = 2;
    itv.it_interval.tv_usec = 0;
    itv.it_value.tv_sec = 2;
    itv.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &itv, &itvOld);
}

void CProcStat::stopTimer()
{
    // printf("stop timer\n");
    struct itimerval itv;
    struct itimerval itvOld;
    itv.it_value.tv_sec = 0; // 将启动参数设置为 0, 表示定时器不启动
    itv.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &itv, &itvOld);
}

void CProcStat::timeout(int iSigNum)
{
    // printf("Acquisition frame rate: %f\n", m_tProcParam.tGrabRate.fRate);
}
