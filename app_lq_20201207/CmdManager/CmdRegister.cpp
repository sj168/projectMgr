
#include "CmdRegister.h"

std::mutex CCmdRegister::m_Mutex;
std::map<unsigned char, CCmdBase *> CCmdRegister::m_mpCmdIdCmdHdlr;

// 单例模式
CCmdRegister *CCmdRegister::getInstance()
{
    static CCmdRegister clsCmdRegitser;

    return &clsCmdRegitser;
}

void CCmdRegister::registCmdHandler(unsigned char ucCmdSubId, CCmdBase *pclsCmdBase)
{
    // lock_guard 在构造函数里加锁，在析构函数里解锁
    std::lock_guard<std::mutex> lock(m_Mutex);
    // 将具体的指令对应操作方法插入到map中
    m_mpCmdIdCmdHdlr[ucCmdSubId] = pclsCmdBase;
}

CCmdBase *CCmdRegister::findCmdHandler(unsigned char ucCmdSubId)
{
    std::map<unsigned char, CCmdBase *>::const_iterator cit = m_mpCmdIdCmdHdlr.find(ucCmdSubId);

    if (cit != m_mpCmdIdCmdHdlr.end())
    {
        return cit->second;
    }

    return nullptr;
}
