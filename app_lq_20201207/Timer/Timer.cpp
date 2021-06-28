#include "Timer.h"

T_DevInfo CTimer::dev_info_ = {0};

CTimer::CTimer()
{
    m_thread = 0;
    m_bRunning = false;
    m_bHandle = false;
    memset(&dev_info_, 0x00, sizeof(T_DevInfo));
    m_pclsDevMgr = CDeviceManager::getInstance();
    timerInit(this);
}

CTimer::~CTimer()
{
    timerUninit();
}

void CTimer::timerInit(CTimer *m_clsTimer)
{
    int ret = 0;
    m_clsTimer->m_bRunning = true;
    m_clsTimer->m_bHandle = true;
    ret = pthread_create(&m_thread, NULL, timerFunc, m_clsTimer);
    if(ret < 0)
    {
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Thread create failed.");
    }
}

void CTimer::timerUninit()
{
    m_thread = 0;
    m_bRunning = false;
    m_bHandle = false;
    pthread_join(m_thread, nullptr);
}

void *CTimer::timerFunc(void *param)
{
    set_cpu(3);
    CTimer *m_clsTmpTimer = (CTimer *)param;
    while(m_clsTmpTimer->m_bRunning)
    {
        if(m_clsTmpTimer->m_bHandle)
        {
            sleep(10);
#if 0
            m_clsTmpTimer->dev_info_.tCamStat.Width = read_profile_int("setting", "WidthVal", 2048, FILE_INI_NAME);
            m_clsTmpTimer->dev_info_.tCamStat.Height = read_profile_int("setting", "HeightVal", 1024, FILE_INI_NAME);
            m_clsTmpTimer->dev_info_.tCamStat.Exposure = read_profile_double("setting", "ExpTime", 50.00, FILE_INI_NAME);
            m_clsTmpTimer->dev_info_.tCamStat.LineFreq = read_profile_double("setting", "LineFreq", 10000, FILE_INI_NAME);
            m_clsTmpTimer->dev_info_.tCamStat.Gain = read_profile_double("setting", "GainVal", 1.00, FILE_INI_NAME);
            m_clsTmpTimer->dev_info_.tCamStat.Offset = read_profile_int("setting", "Offset_X", 0, FILE_INI_NAME);
#else
            CCameraManager::getImageSize(&m_clsTmpTimer->dev_info_.tCamStat.Width, &m_clsTmpTimer->dev_info_.tCamStat.Height);
            CCameraManager::getExposureTime(&m_clsTmpTimer->dev_info_.tCamStat.Exposure);
            CCameraManager::getAcquisitionFrameRate(&m_clsTmpTimer->dev_info_.tCamStat.LineFreq);
            CCameraManager::getGain(&m_clsTmpTimer->dev_info_.tCamStat.Gain);
            CCameraManager::getImageOffset(&m_clsTmpTimer->dev_info_.tCamStat.Offset, nullptr);
            CCameraManager::getCameraTemperature(&dev_info_.tCamStat.cam_temp);
            // printf("width:%u height:%u exp:%.2f lineFreq:%u gain:%.2f offset:%u temp:%.2f\n", m_clsTmpTimer->dev_info_.tCamStat.Width, m_clsTmpTimer->dev_info_.tCamStat.Height,
            //             m_clsTmpTimer->dev_info_.tCamStat.Exposure, m_clsTmpTimer->dev_info_.tCamStat.LineFreq, m_clsTmpTimer->dev_info_.tCamStat.Gain,
            //             m_clsTmpTimer->dev_info_.tCamStat.Offset, dev_info_.tCamStat.cam_temp);
#endif
            m_clsTmpTimer->m_pclsDevMgr->getDeviceStat(&dev_info_.tDevStat);
            m_clsTmpTimer->m_pclsCmdMgr->makeCmdAndSend(eCIManagerLQDevStat, (const unsigned char *)&m_clsTmpTimer->dev_info_, sizeof(T_DevInfo), eDTMUdp);
        }
        else
        {
            usleep(1000);
        }
    }
    
    return NULL;
}
