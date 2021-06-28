
#ifndef UDP_CTRLR_H
#define UDP_CTRLR_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "TcpCtrlr.h"
#include "Inifile.h"

#define MAX_UDP_SENDTO_PKG_LEN              65507

// typedef int (*PFuncDataHandler)(unsigned char *pucData, unsigned int nDataLen, void * pvUsrArg);

typedef struct UdpMgr
{
    int iSockFd;
    struct sockaddr_in tLocalAddr;
    struct sockaddr_in tRemoteAddr;
    char wLocalIP[32];
    unsigned short wLocalPort;
    char wRemoteIP[32];
    unsigned short wRemotePort;
    PFuncDataHandler *ppFuncDataHandler;
    void **ppvUsrArg;
    bool bThreadRunning;
    pthread_t tRecvThread;

} T_UdpMgr, *PT_UdpMgr;

class CUdpCtrlr
{
public:
    CUdpCtrlr();
    ~CUdpCtrlr();

    int setUdpSocket(T_UdpMgr *ptUdpMgr);
    int initUdpCtrlr();
    int sendData(const unsigned char *pucData, unsigned int nDataLen);
    void setClientMgrHead(const T_TcpClientMgr *ptClientMgrHead);
    void setDataHandler(PFuncDataHandler pFuncDataHandler, void *pvUsrArg);
    void setExtDestPort(unsigned short wPort);
private:
    T_UdpMgr m_tUdpMgr;
    const T_TcpClientMgr *m_ptClientMgrHead;
    static void *recvCmdThread(void *pvArg);
    PFuncDataHandler m_pFuncDataHandler;
    void * m_pvUsrArg;
};

#endif // UDP_CTRLR_H
