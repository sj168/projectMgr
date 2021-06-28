
#include "CmdBuild.h"
#include "CmdDef.h"
#include "ErrorCode.h"

CCmdBuild::CCmdBuild()
{
    m_pucCmdData = nullptr;
    m_nCmdDataLen = 0;
}

CCmdBuild::~CCmdBuild()
{
    if (nullptr != m_pucCmdData)
    {
        delete [] m_pucCmdData;
        m_pucCmdData = nullptr;
        m_nCmdDataLen = 0;
    }
}

int CCmdBuild::buildCmd(unsigned char ucCmdFc, unsigned int nCmdParamLen, unsigned char *pucCmdParam, unsigned int * pnCmdDataLen)
{
    unsigned int nCalCmdDataLen = eCPCmdHeadSize + eCPCmdSubHeadSize + eCPCmdFcSize +
                                    eCPCmdParamSize + nCmdParamLen + eCPCmdChecksumSize + eCPCmdTailSize;

    // unsigned int img_param_off = 1 + sizeof(unsigned long long) + 2 * sizeof(unsigned int);
    
    if (nCalCmdDataLen > m_nCmdDataLen)
    {
        if (nullptr != m_pucCmdData)
        {
            delete [] m_pucCmdData;
            m_nCmdDataLen = 0;
        }
        
        m_pucCmdData = new unsigned char[nCalCmdDataLen];
        if (nullptr == m_pucCmdData)
        {
            return eECMem;
        }

        m_nCmdDataLen = nCalCmdDataLen;
    }

    m_pucCmdData[eCPCmdHeadPos] = CMD_HEAD;                             // 头1
    m_pucCmdData[eCPCmdSubHeadPos] = CMD_SUB_HEAD;                      // 头2
    m_pucCmdData[eCPCmdFcPos] = ucCmdFc;                                // FC
    memcpy(m_pucCmdData + eCPCmdParamLenPos, &nCmdParamLen, sizeof(nCmdParamLen));                     // 参数长度
    memcpy(m_pucCmdData + eCPCmdParamPos, pucCmdParam, nCmdParamLen);
    // memcpy(m_pucCmdData + eCPCmdParamPos + img_param_off, pucCmdParam, nCmdParamLen - img_param_off); // 参数
    m_pucCmdData[eCPCmdParamPos + nCmdParamLen] = 0x00;                 //校验和，不校验，填0
    m_pucCmdData[eCPCmdParamPos + nCmdParamLen + 1] = CMD_TAIL;         // 尾

    if (nullptr != pnCmdDataLen)
    {
        *pnCmdDataLen = nCalCmdDataLen;
    }
    
    return eECOK;
}
