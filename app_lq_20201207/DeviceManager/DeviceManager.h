#ifndef _DEVICE_MANAGER_H
#define _DEVICE_MANAGER_H

#include "CmdBase.h"
#include "CmdManager.h"
#include "CmdDef.h"
#include "ErrorCode.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <mutex>
#include <stdio.h>
#include <Log.h>

class CDeviceManager : public CCmdBase 
{
public:
    static CDeviceManager *getInstance();

    virtual int cmdFunction(unsigned char ucCmdSubId, const unsigned char *pucCmdParam, unsigned int nCmdParamLen);  

    bool forceSetDeviceIP(char *nic_name, unsigned int *ip_addr, unsigned int *sub_mask);
    bool autoSetDeviceIp();
    void getDeviceStat(T_DevStat *m_tDevStat);
    void syncSystemTime(const unsigned char *strTime);

    ~CDeviceManager();

private:
    CDeviceManager();

    static T_DevStat m_tDevStat;
    static std::mutex m_Mutex;
    
    CCmdManager *m_pclsCmdMgr;
} ;


#endif
