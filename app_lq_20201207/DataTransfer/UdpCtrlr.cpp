
#include <fcntl.h>
#include <cstring>
#include <cstdio>
#include <unistd.h>

#include "ErrorCode.h"

#include "UdpCtrlr.h"

CUdpCtrlr::CUdpCtrlr()
{
    bzero(&(m_tUdpMgr.tLocalAddr), sizeof(m_tUdpMgr.tLocalAddr));
    bzero(&(m_tUdpMgr.tRemoteAddr), sizeof(m_tUdpMgr.tRemoteAddr));
    m_tUdpMgr.wRemotePort = (unsigned short)read_profile_int("setting", "PrPort", 8080, FILE_INI_NAME);
    m_tUdpMgr.wLocalPort = (unsigned short)read_profile_int("setting", "LoPort", 8080, FILE_INI_NAME);
    read_profile_string("setting", "LoAddr", m_tUdpMgr.wLocalIP, sizeof (m_tUdpMgr.wLocalIP), "", FILE_INI_NAME);
    read_profile_string("setting", "PrAddr", m_tUdpMgr.wRemoteIP, sizeof (m_tUdpMgr.wRemoteIP), "", FILE_INI_NAME);

    initUdpCtrlr();
    setUdpSocket(&m_tUdpMgr);
    m_tUdpMgr.tRemoteAddr.sin_family = AF_INET;
    m_tUdpMgr.tRemoteAddr.sin_port = htons(m_tUdpMgr.wRemotePort);
    m_tUdpMgr.bThreadRunning = true;
    int iRet = pthread_create(&m_tUdpMgr.tRecvThread, NULL, recvCmdThread, (void *)(this));
    if(iRet < 0)
    {
        printf("recvCmdThread create failed\n");
        return;
    }
}

CUdpCtrlr::~CUdpCtrlr()
{
    if (m_tUdpMgr.iSockFd > 0)
    {
        close(m_tUdpMgr.iSockFd);
        m_tUdpMgr.iSockFd = -1;
    }
    m_tUdpMgr.bThreadRunning = false;

    pthread_join(m_tUdpMgr.tRecvThread, NULL);
}

int CUdpCtrlr::setUdpSocket(T_UdpMgr * ptUdpMgr)
{
    int iFlags;
    int iRet;
    struct sockaddr_in local;

    // in case of 'address already in use' error
    iFlags = 1;
    iRet = setsockopt(ptUdpMgr->iSockFd, SOL_SOCKET, SO_REUSEADDR, &iFlags, sizeof(iFlags));
    if (iRet != 0)
    {
        close(ptUdpMgr->iSockFd);
        ptUdpMgr->iSockFd = -1;
        return eECSetSocketFlag;
    }

    memset(&local, 0x00, sizeof (local));
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = inet_addr(m_tUdpMgr.wLocalIP);
    local.sin_port = htons(ptUdpMgr->wLocalPort);

    if (bind(ptUdpMgr->iSockFd, (struct sockaddr*) &local, sizeof (local)) == -1) {
        perror("bind");
        return eECBindSocket;
    }

    int sockBufLen = 0;
    sockBufLen = 1024 * 1024 * 20; //20MB
    iRet = setsockopt(ptUdpMgr->iSockFd, SOL_SOCKET, SO_SNDBUFFORCE, &sockBufLen, sizeof (int));
    if (iRet < 0) 
    {
        printf("setsockopt SO_RCVBUFFORCE error. %d\n", iRet);
        return eECSetSocketFlag;
    }
    
    int len = sizeof(int);
    iRet = getsockopt(ptUdpMgr->iSockFd, SOL_SOCKET, SO_SNDBUF, &sockBufLen, (socklen_t *) &len);
    if (iRet < 0) 
    {
        printf("getsockopt SO_RCVBUF errot. %d\n", iRet);
        return eECSetSocketFlag;
    }
    // printf("sock send buf is %d\n", len);

    return eECOK;
}

int CUdpCtrlr::initUdpCtrlr()
{
    m_tUdpMgr.iSockFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_tUdpMgr.iSockFd < 0)
    {
        return eECSocket;
    }

    return eECOK;
}

int CUdpCtrlr::sendData(const unsigned char * pucData, unsigned int nDataLen)
{
    const T_TcpClientMgr *ptClientMgr = m_ptClientMgrHead->ptNext;
    
    int iBytesSent = 0;
    int iRet = eECOK;
    unsigned int nSentLen = 0;
    unsigned int nCurSendLen;

    while (ptClientMgr != nullptr)
    {
        if (ptClientMgr->bClientRunning)
        {
            memcpy(&m_tUdpMgr.tRemoteAddr.sin_addr, &ptClientMgr->tRemoteClientAddr.sin_addr,
                    sizeof(struct in_addr));
            // printf("Remote addr: %X, %X\n", *((unsigned long *)(&(m_tUdpMgr.tRemoteAddr.sin_addr))),
            //         *((unsigned long *)(&(ptClientMgr->tRemoteClientAddr.sin_addr))));

            /*
             * 用 UDP 协议发送时，用 sendto 函数最大能发送数据的长度为：65535- IP头(20) - UDP头(8) ＝ 65507 字节。
             * 用 sendto 函数发送数据时，如果发送数据长度大于该值，则函数会返回错误
             */
            while (nSentLen < nDataLen)
            {
                nCurSendLen = (nDataLen - nSentLen);
                if (nCurSendLen > MAX_UDP_SENDTO_PKG_LEN)
                {
                    nCurSendLen = MAX_UDP_SENDTO_PKG_LEN;
                }

                iBytesSent = sendto(m_tUdpMgr.iSockFd, pucData + nSentLen, nCurSendLen, 0,
                                (struct sockaddr *)(&m_tUdpMgr.tRemoteAddr), sizeof(struct sockaddr));
                if (iBytesSent <= 0)
                {
                    iRet = eECDataSendError;
                    break;
                }

                // printf("Send : %d bytes.\n", iBytesSent);
                nSentLen += iBytesSent;
            }
            // printf("nSendLen:%d\n", nSentLen);
        }
        
        nSentLen = 0;

        ptClientMgr = ptClientMgr->ptNext;
    }

    return iRet;
}

void CUdpCtrlr::setClientMgrHead(const T_TcpClientMgr * ptClientMgrHead)
{
    m_ptClientMgrHead = ptClientMgrHead;
}

void CUdpCtrlr::setExtDestPort(unsigned short wPort)
{
    m_tUdpMgr.wRemotePort = wPort;
    m_tUdpMgr.tRemoteAddr.sin_port = htons(m_tUdpMgr.wRemotePort);
}

void * CUdpCtrlr::recvCmdThread(void * pvArg)
{
    CUdpCtrlr *pUdpCtrlr = (CUdpCtrlr *)pvArg;
    int iRet;
    unsigned char aucBuf[256];

    while(pUdpCtrlr->m_tUdpMgr.bThreadRunning)
    {
        iRet = recv(pUdpCtrlr->m_tUdpMgr.iSockFd, aucBuf, 256, 0);
        if(iRet < 0)
        {
            perror("udp->recv");
            goto exit;
        }
        else if(iRet == 0)
        {
            continue;
        }
        else
        {
            printf("Cmd: \n");
            for(int cnt = 0; cnt < iRet; cnt++)
            {
                printf(" %02x", aucBuf[cnt]);
                if((cnt + 1) % 16 == 0)
                    printf("\n");
            }
            printf("\n");

            if (iRet > 0)
            {
                if ((pUdpCtrlr->m_pFuncDataHandler) != nullptr)
                {
                    // 处理数据，在main函数里注册handleCmdData，然后由CDataTransfer调用setDataHandler，将handleCmdData指针赋给m_pFuncDataHandler，即可调用handleCmdData
                    
                    (pUdpCtrlr->m_pFuncDataHandler)(aucBuf, iRet, pUdpCtrlr->m_pvUsrArg);
                }
            }
        }
    }

exit:
    close(pUdpCtrlr->m_tUdpMgr.iSockFd);
    pUdpCtrlr->m_tUdpMgr.iSockFd = -1;
    pUdpCtrlr->m_tUdpMgr.bThreadRunning = false;

    return nullptr;
}

void CUdpCtrlr::setDataHandler(PFuncDataHandler pFuncDataHandler, void * pvUsrArg)
{
    m_pFuncDataHandler = pFuncDataHandler;
    m_pvUsrArg = pvUsrArg;
}