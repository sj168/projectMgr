
#include "ErrorCode.h"

#include "DataType.h"

T_DataTypeParam CDataType::m_tTDataTypeParam = {eTDIImageData, 1};
CCmdManager * CDataType::m_pclsCmdMgr = CCmdManager::getInstance();;

CDataType * CDataType::getInstance()
{
    static CDataType dataType;
    return &dataType;
}

unsigned int CDataType::getDataType()
{
    return m_tTDataTypeParam.nDataType;
}

unsigned int CDataType::getDataRate()
{
    return m_tTDataTypeParam.nDataRate;
}

// 传输数据类型和传输速率
int CDataType::cmdFunction(unsigned char ucCmdSubId, const unsigned char *pucCmdParam, unsigned int nCmdParamLen)
{
    int iRet = eECOK;
    int iRetLen = eCPCmdSubCmdSize + sizeof(unsigned char); // 回传命令执行结果的长度
    switch (ucCmdSubId)
    {
        case eDTDataType:
        {
            if (nCmdParamLen >= eCPCmdSubCmdSize + sizeof(unsigned char) && nullptr != pucCmdParam)
            {
                m_tTDataTypeParam.nDataType = static_cast<unsigned int>(pucCmdParam[1]);
            }
            else
                iRet = eECParam;

            break;
        }

        case eDTDataRate:
        {
            if (nCmdParamLen >= eCPCmdSubCmdSize + sizeof(unsigned int) && nullptr != pucCmdParam)
            {
                unsigned int nDataRate = 0;
                memcpy(&nDataRate, (unsigned char *)pucCmdParam + eCPCmdSubCmdSize, sizeof(nDataRate));
                m_tTDataTypeParam.nDataRate = nDataRate;
            }
            else
                iRet = eECParam;
            
            break;
        }
    }

    unsigned char * pucRetBuf = new unsigned char[iRetLen];
    if (nullptr != pucRetBuf)
    {
        pucRetBuf[0] = eCmdAck;
        pucRetBuf[1] = (iRet == eECOK) ? eAckOk : eAckFailed;
        m_pclsCmdMgr->makeCmdAndSend(eCIAck, pucRetBuf, iRetLen, eDTMUdp);
        delete [] pucRetBuf;
    }

    return iRet;
}

CDataType::CDataType()
{
}
