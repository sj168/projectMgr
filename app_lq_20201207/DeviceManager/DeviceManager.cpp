#include "DeviceManager.h"
#include <unistd.h>

T_DevStat CDeviceManager::m_tDevStat = {0};
std::mutex CDeviceManager::m_Mutex;

CDeviceManager *CDeviceManager::getInstance()
{
    static CDeviceManager clsDeviceManager;
    return &clsDeviceManager;
}

int CDeviceManager::cmdFunction(unsigned char ucCmdSubId, const unsigned char *pucCmdParam, unsigned int nCmdParamLen)
{
    int iRet = eECOK;
    int iRetLen = 1;
    T_DevInfo m_tDevInfo = {0};

    switch(ucCmdSubId)
    {
        case eGetDeviceStat:
        {
            getDeviceStat(&m_tDevInfo.tDevStat);
            m_pclsCmdMgr->makeCmdAndSend(eCIManagerLQDevStat, (const unsigned char *)&m_tDevInfo, sizeof(m_tDevInfo), eDTMUdp);
            return 0;
        }
        case eSetDeviceIPAddr:
        {
            if(pucCmdParam[1] == eSetPersistentIP)
            {
                char nic_ptr[5] = {0}; 
                uint32_t ip_addr;
                uint32_t mask_addr;

                memcpy(nic_ptr, pucCmdParam + 2, 4);
                ip_addr = *(uint32_t *)(pucCmdParam + 2 + 4);
                mask_addr = *(uint32_t *)(pucCmdParam + 2 + 4 + sizeof(uint32_t));

                if(!forceSetDeviceIP(nic_ptr, &ip_addr, &mask_addr))
                    iRet = eECSetParam;
            }
            else if(pucCmdParam[1] == eSetDHCPIP)
            {
                if(autoSetDeviceIp())
                    iRet = eECSetParam;
            }
            break;
        }
        case eSyncSystemTime:
        {
            syncSystemTime(&pucCmdParam[1]);
            break;
        }
        default:
        {
            iRet = eECSetParam;
            break;
        }
    }

    unsigned char *pucRetBuf = new unsigned char[iRetLen];
    if (nullptr != pucRetBuf)
    {
        pucRetBuf[0] = eCmdAck;
        pucRetBuf[1] = (iRet == eECOK) ? eAckOk : eAckFailed;
        m_pclsCmdMgr->makeCmdAndSend(eCIAck, pucRetBuf, iRetLen, eDTMUdp);
        delete [] pucRetBuf;
    }

    return iRet;
}

bool CDeviceManager::forceSetDeviceIP(char *nic_name, unsigned int *ip_addr, unsigned int *sub_mask)
{
    char cmd_ptr[128] = {0};
    struct in_addr in_ip_addr = {0};
    struct in_addr in_mask_addr = {0};
    memcpy(&in_ip_addr, ip_addr, sizeof(uint32_t));
    memcpy(&in_mask_addr, sub_mask, sizeof(uint32_t));
    // 注意：inet_ntoa返回的是一个静态地址，如果重复调用，则后一次的转换结果会覆盖前一次，因此要么分开转换了执行指令，要么用将转换结果赋给另外的内存空间
    if(nic_name != nullptr)
    {
        sprintf(cmd_ptr, "ifconfig %s %s up", nic_name, inet_ntoa(in_ip_addr));
        system(cmd_ptr);
        printf("%s\n", cmd_ptr);
        memset(cmd_ptr, 0x00, sizeof(cmd_ptr));
        sprintf(cmd_ptr, "ifconfig %s netmask %s up", nic_name, inet_ntoa(in_mask_addr));
        system(cmd_ptr);
        printf("%s\n", cmd_ptr);        
        return true;
    }
    
    return false;
}

bool CDeviceManager::autoSetDeviceIp()
{
    // TODO

    return false;
}

void CDeviceManager::getDeviceStat(T_DevStat *m_tDevStat)
{
    FILE *stream;
    static int pid = 0;
    float cpu_util, cpu_temp, mem_util;
    char buf[128] = {0};
    char *p;
    char cmd_ptr[128] = {0};

    if(pid == 0)
    {
        /*Get process name*/
        char processName[16] = {0};
        char processPath[128] = {0};
        char *path_end;
        if(readlink("/proc/self/exe", processPath, sizeof(processPath)) <= 0)
        {
            printLog(CLog::FILE_AND_TERMINAL, CLog::ERROR, "Read process path failed.");
            return;
        }
        path_end = strrchr(processPath, '/');
        if(path_end == NULL)
            return;
        ++path_end;
        strcpy(processName, path_end);
        *path_end = '\0';
        sprintf(cmd_ptr, "pidof %s", processName);
        stream = popen(cmd_ptr, "r");
        if(stream == NULL)
            return;
        p = fgets(buf, 16, stream);
        pid = atoi(buf);
        memset(buf, 0x00, sizeof(buf));
        pclose(stream);
        memset(cmd_ptr, 0x00, sizeof(cmd_ptr));
    }

    sprintf(cmd_ptr, "top -d 3 -b -n 1 -p %d | grep  -A 2 %%CPU | awk '{print $9}' | grep -v CPU", pid);
    stream = popen(cmd_ptr, "r");
    if(stream == NULL)
        return;
    p = fgets(buf, 16, stream);
    cpu_util = atof(buf);
    memset(buf, 0x00, sizeof(buf));
    memset(cmd_ptr, 0x00, sizeof(cmd_ptr));
    pclose(stream);
    
    sprintf(cmd_ptr, "top -d 3 -b -n 1 -p %d | grep -A 2 %%MEM | awk '{print $10}' | grep -v MEM", pid);
    stream = popen(cmd_ptr, "r");
    if(stream == NULL)
        return;
    p = fgets(buf, 16, stream);
    mem_util = atof(buf);
    memset(buf, 0x00, sizeof(buf));
    memset(cmd_ptr, 0x00, sizeof(cmd_ptr));
    pclose(stream);

    sprintf(cmd_ptr, "cat /sys/class/thermal/thermal_zone0/temp"); 
    stream = popen(cmd_ptr, "r");
    if(stream == NULL)
        return;
    p = fgets(buf, 16, stream);
    cpu_temp = atof(buf);
    cpu_temp *= 0.001;
    pclose(stream);
    
    m_tDevStat->cpu_temp = cpu_temp;
    m_tDevStat->cpu_util = cpu_util;
    m_tDevStat->mem_util = mem_util;
}

void CDeviceManager::syncSystemTime(const unsigned char *strTime)
{
    char str[512] = {0};
    int year, mon, day, hour, min, sec;
    sscanf((const char *)strTime, "%04d%02d%02d%02d%02d%02d", 
            &year, &mon, &day, &hour, &min, &sec);
    
    sprintf(str, "date -s \"%04d-%02d-%02d %02d:%02d:%02d\">/dev/null",
                year, mon, day, hour, min, sec);
    system(str);
    printf("Current system time is set to: %04d-%02d-%02d %02d:%02d:%02d\r\n",
           year, mon, day, hour, min, sec);
    return ;
}

CDeviceManager::CDeviceManager()
{
    m_pclsCmdMgr = CCmdManager::getInstance();
}

CDeviceManager::~CDeviceManager()
{}
