
#ifndef DATA_TRANSFER_H
#define DATA_TRANSFER_H

#include "BaseDef.h"
#include "CmdBase.h"
#include "CmdDef.h"
#include "TcpCtrlr.h"
#include "UdpCtrlr.h"
#include "SystemMessageManager.h"

class CDataTransfer : public CCmdBase
{
public:
    CDataTransfer();
    ~CDataTransfer();

    void registDataHandler(PFuncDataHandler pFuncDataHandler, void * pvUsrArg);
    static int transData(const unsigned char *pucData, unsigned int nDataLen, unsigned int nDataTransMethod = eDTMTcp);

    virtual int cmdFunction(unsigned char ucCmdSubId, const unsigned char *pucCmdDataPkg, unsigned int nDataLen);

private:
    static CTcpCtrlr m_clsTcpCtrlr;
    static CUdpCtrlr m_clsUdpCtrlr;

    unsigned int m_nDataTransMethod;
};

#endif // DATA_TRANSFER_H
