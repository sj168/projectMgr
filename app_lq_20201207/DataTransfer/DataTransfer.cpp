
#include <stdio.h>

#include "ErrorCode.h"
#include "DataTransfer.h"

CTcpCtrlr CDataTransfer::m_clsTcpCtrlr;
CUdpCtrlr CDataTransfer::m_clsUdpCtrlr;

CDataTransfer::CDataTransfer() : CCmdBase()
{
    m_nDataTransMethod = eDTMTcp;

    m_clsUdpCtrlr.setClientMgrHead(m_clsTcpCtrlr.getTcpClientMgrHead());
}

CDataTransfer::~CDataTransfer()
{

}

void CDataTransfer::registDataHandler(PFuncDataHandler pFuncDataHandler, void * pvUsrArg)
{
    // m_clsTcpCtrlr.setDataHandler(pFuncDataHandler, pvUsrArg);
    m_clsUdpCtrlr.setDataHandler(pFuncDataHandler, pvUsrArg);
}

int CDataTransfer::transData(const unsigned char * pucData, unsigned int nDataLen, unsigned int nDataTransMethod)
{
    int iRet;
    switch (nDataTransMethod)
    {
        case eDTMUdp:
        {
            iRet = m_clsUdpCtrlr.sendData(pucData, nDataLen);
            break;
        }
        case eDTMTcp:
        {
            iRet = m_clsTcpCtrlr.sendData(pucData, nDataLen);
            break;
        }
        default:
        {
            return eECDataTransMethod;
        }
    }

    return iRet;
}

int CDataTransfer::cmdFunction(unsigned char ucCmdSubId, const unsigned char * pucCmdDataPkg, unsigned int nDataLen)
{
    printf("DataTransfer cmdFunction called\n");

    // unsigned char pucDataPkgPtr = nullptr;
    // unsigned int nDataPkgLen = 0;
    unsigned char ucCommandId = ucCmdSubId;
    if(ucCommandId != eCITransData)
    {
        return eECCmdId;
    }

    // 获取图像数据地址


    return eECOK;
}
