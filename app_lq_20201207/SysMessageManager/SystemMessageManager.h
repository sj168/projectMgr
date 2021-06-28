
#ifndef SYSTEMMESSAGEMANAGER_H
#define SYSTEMMESSAGEMANAGER_H

#include <map>
#include <vector>
// #include <mutex>

using namespace std;

#include "CirQueue.h"
#include "SystemMessageBase.h"
#include "Log.h"

class CSysMsgMgr
{
public:
    CSysMsgMgr();
    ~CSysMsgMgr();

    static int registMsg(unsigned char ucMsgFc, CSysMsgBase * pSysMsgBase);
    static int queueMsg(T_MsgParam * ptMsgParam);
    int handleMsg();

private:
    static vector<CSysMsgBase *> * findMsgHandlerVector(unsigned char ucMsgFc);
    static map<unsigned char, vector<CSysMsgBase *> *> m_mpucIdSysMsgBase;
    static CCirQueue<T_MsgParam *> m_MsgParamCirQueue;
    // static mutex m_Mutex;
};

#endif // SYSTEMMESSAGEMANAGER_H
