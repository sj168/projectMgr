
#ifndef CMD_REGISTER_H
#define CMD_REGISTER_H

#include <map>
#include <mutex>

#include "CmdBase.h"

class CCmdRegister
{
public:
    static CCmdRegister * getInstance();
    static void registCmdHandler(unsigned char ucCmdSubId, CCmdBase * pclsCmdBase);
    static CCmdBase *findCmdHandler(unsigned char ucCmdSubId);

    CCmdRegister(const CCmdRegister &) = delete;
    CCmdRegister & operator=(const CCmdRegister &) = delete;

private:
    CCmdRegister()
    {}

    static std::mutex m_Mutex;
    static std::map<unsigned char, CCmdBase *> m_mpCmdIdCmdHdlr;
};

#endif // COMMAND_REGISTER_H
