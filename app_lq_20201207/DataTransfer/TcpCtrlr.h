
#ifndef TCP_CTRLR_H
#define TCP_CTRLR_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "Inifile.h"

typedef int (*PFuncDataHandler)(unsigned char *pucData, unsigned int nDataLen, void * pvUsrArg);

typedef struct TcpClientMgr
{
    // int iClientId;
    int iClientSockFd;
    struct sockaddr_in tRemoteClientAddr;
    bool bClientRunning;
    pthread_t tClientThreadId;
    PFuncDataHandler *ppFuncDataHandler;
    void **ppvUsrArg;

    struct TcpClientMgr * ptNext;
} T_TcpClientMgr, * PT_TcpClientMgr;

typedef struct TcpServerMgr
{
    int iSockFd;
    unsigned short wPort;
    struct sockaddr_in tLocalServerAddr;
    bool bListening;
    int iConnectedClients;
    int iMaxConnections;
    pthread_t tListenThread;
    T_TcpClientMgr *ptTcpClientMgrHead;
} T_TcpServerMgr, *PT_TcpServerMgr;

class CTcpCtrlr
{
public:
    CTcpCtrlr();
    ~CTcpCtrlr();

    static int setKeepAlive(int iSockFd, bool bKeepAlive, int iKeepIdle, int iKeepInterval, int iKeepCount);
    static int checkTcpLink(int iSockFd);
    int sendData(const unsigned char *pucData, unsigned int nLen);
    void closeSocket();
    void setDataHandler(PFuncDataHandler pFuncDataHandler, void *pvUsrArg);
    const T_TcpClientMgr *getTcpClientMgrHead() const;

private:
    static int prepareSocket(T_TcpServerMgr *ptTcpServerMgr);
    static void *listeningThread(void *pvArg);
    static void *clientManageThread(void *pvArg);

    void clearClientLinks();

    T_TcpServerMgr m_tTcpServerMgr;
    PFuncDataHandler m_pFuncDataHandler;
    void *m_pvUsrArg;
    pthread_mutex_t m_tMutex;
};

#endif // TCP_CTRLR_H
