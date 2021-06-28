
#include "ErrorCode.h"
#include "CmdManager.h"

std::mutex CCmdManager::m_MutexForDispatch;
std::mutex CCmdManager::m_MutexForSend;
CCmdRegister *CCmdManager::m_pclsCmdRegister = CCmdRegister::getInstance();
CCmdParser CCmdManager::m_clsCmdParser;
CCmdBuild CCmdManager::m_clsCmdBuild;
PFuncDataTransHandler CCmdManager::m_pFuncDataTransHandler = nullptr;

CCmdManager * CCmdManager::getInstance()
{
    static CCmdManager clsCmdManager;
    
    return &clsCmdManager;
}

/**
 * pucCmdData:整个指令包
 * nCmdDataLen：整个指令包长
 * pvUsrArg：留作扩展
 */
int CCmdManager::handleCmdData(unsigned char *pucCmdData, unsigned int nCmdDataLen, void *pvUsrArg)
{
    T_CommandId tCmdId;
    unsigned char *pucCmdParam = nullptr;
    unsigned int nCmdParamLen = 0;
    int iRet = m_clsCmdParser.parseCmd(pucCmdData, nCmdDataLen, &(tCmdId.ucCmdFc), &nCmdParamLen, &pucCmdParam);
    // 子命令ID
    tCmdId.ucCmdSubId = pucCmdParam[0];
    printf("ucCmdFc:0x%x ucCmdSubId:0x%x\n", tCmdId.ucCmdFc, tCmdId.ucCmdSubId);

    if (eECOK == iRet)
    {
        return dispatchCmd(&tCmdId, pucCmdParam, nCmdParamLen);
    }

    return iRet;
}

int CCmdManager::dispatchCmd(T_CommandId *ptCommandId, const unsigned char *pucCmdParam, unsigned int nCmdParamLen)
{
    // lock_guard 在构造函数里加锁，在析构函数里解锁
    // printf("dispatchCmd function called\n");
    std::lock_guard<std::mutex> lock(m_MutexForDispatch);

    CCmdBase *pCmdBase = m_pclsCmdRegister->findCmdHandler(ptCommandId->ucCmdFc);
    if (nullptr != pCmdBase)
    {
        return pCmdBase->cmdFunction(ptCommandId->ucCmdSubId, pucCmdParam, nCmdParamLen);
    }

    return eECCmdId;
}

void CCmdManager::registDataTransHandler(PFuncDataTransHandler pFuncDataTransHandler)
{
    m_pFuncDataTransHandler = pFuncDataTransHandler;
}

// 构建并发送命令包
int CCmdManager::makeCmdAndSend(unsigned char ucCmdFc, const unsigned char *pucCmdParam, unsigned int nCmdParamLen, unsigned int nDataTransMethod)
{
    // lock_guard 在构造函数里加锁，在析构函数里解锁
    std::lock_guard<std::mutex> lock(m_MutexForSend);
    unsigned int nCmdDataLen = 0;
    int iRet = m_clsCmdBuild.buildCmd(ucCmdFc, nCmdParamLen, (unsigned char *)pucCmdParam, &nCmdDataLen);
    if (iRet == eECOK && m_pFuncDataTransHandler != nullptr)
    {
        //main函数里注册sendData到m_pFuncDataTransHandler函数指针，即可调用sendData
        iRet = m_pFuncDataTransHandler(m_clsCmdBuild.m_pucCmdData, nCmdDataLen, nDataTransMethod);
        return iRet;
    }

    return eECOK;
}
