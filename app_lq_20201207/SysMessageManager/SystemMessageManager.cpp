
#include "ErrorCode.h"

#include "SystemMessageManager.h"

map<unsigned char, vector<CSysMsgBase *> *> CSysMsgMgr::m_mpucIdSysMsgBase;
CCirQueue<T_MsgParam *> CSysMsgMgr::m_MsgParamCirQueue(1024);
// mutex CSysMsgMgr::m_Mutex;

CSysMsgMgr::CSysMsgMgr()
{}

CSysMsgMgr::~CSysMsgMgr()
{
    for (map<unsigned char, vector<CSysMsgBase *> *>::iterator it = m_mpucIdSysMsgBase.begin();
        it != m_mpucIdSysMsgBase.end();
        ++it)
    {
        if (nullptr != it->second)
        {
            delete (it->second);
            it->second = nullptr;
        }
    }
}

int CSysMsgMgr::registMsg(unsigned char ucMsgFc, CSysMsgBase * pSysMsgBase)
{
    vector<CSysMsgBase *> * pvectSysMsgBase = findMsgHandlerVector(ucMsgFc);

    if (nullptr == pvectSysMsgBase)
    {
        pvectSysMsgBase = new vector<CSysMsgBase *>;
        if (nullptr == pvectSysMsgBase)
            return eECMem;
        m_mpucIdSysMsgBase[ucMsgFc] = pvectSysMsgBase;
    }

    for (vector<CSysMsgBase *>::iterator it = pvectSysMsgBase->begin();
         it != pvectSysMsgBase->end();
         ++it)
    {
        if (*it == pSysMsgBase)
            return eECOK;
    }

    pvectSysMsgBase->push_back(pSysMsgBase);
    // printf("regist one message\n");

    return eECOK;
}

int CSysMsgMgr::queueMsg(T_MsgParam *ptMsgParam)
{
    if (m_MsgParamCirQueue.cirQueueIn(ptMsgParam) != 1)
    {
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "MsgQueue is full.");
        return eECQueueFull;
    }
    
    return eECOK;
}

vector<CSysMsgBase *> * CSysMsgMgr::findMsgHandlerVector(unsigned char ucMsgFc)
{
    vector<CSysMsgBase *> * pvectSysMsgBase = nullptr;
    map<unsigned char, vector<CSysMsgBase *> *>::const_iterator cit = m_mpucIdSysMsgBase.find(ucMsgFc);
    if (cit != m_mpucIdSysMsgBase.end())
        pvectSysMsgBase = cit->second;
    
    return pvectSysMsgBase;
}

int CSysMsgMgr::handleMsg()
{
    static T_MsgParam * ptMsgParam;
    static vector<CSysMsgBase *> * pvectSysMsgBase = nullptr;

    if (m_MsgParamCirQueue.length() > 0)
    {
        if (m_MsgParamCirQueue.cirQueueOut(ptMsgParam) == 1)
        {
            pvectSysMsgBase = findMsgHandlerVector(ptMsgParam->tMsgId.ucMsgFc);
            if (nullptr != pvectSysMsgBase)
            {
                for (vector<CSysMsgBase *>::iterator it = pvectSysMsgBase->begin();
                     it != pvectSysMsgBase->end();
                     ++it)
                {
                    ptMsgParam->iLastRetVal = (*it)->handleSystemMessage(ptMsgParam);
                    ptMsgParam->bMsgHandled = true;
                }
            }
        }
    }

    return eECOK;
}


