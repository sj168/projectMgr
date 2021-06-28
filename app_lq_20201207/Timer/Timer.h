#ifndef TIMER_H
#define TIMER_H

#include "CameraManager.h"
#include "CmdManager.h"
#include "Inifile.h"
#include "CmdDef.h"
#include "BaseDef.h"
#include <pthread.h>
#include <unistd.h>
#include "DeviceManager.h"


class CTimer 
{
public:
    CTimer();
    ~CTimer();

    void timerInit(CTimer *);
    void timerUninit();

private:
    static void *timerFunc(void *param);
    pthread_t m_thread;
    CCmdManager *m_pclsCmdMgr;
    CDeviceManager *m_pclsDevMgr;
    static T_DevInfo dev_info_;
    bool m_bRunning;
    bool m_bHandle;
};

#endif
