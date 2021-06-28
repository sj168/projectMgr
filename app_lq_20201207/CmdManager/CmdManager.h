
#ifndef CMD_MANAGER_H
#define CMD_MANAGER_H

#include <mutex>

#include "BaseDef.h"
#include "CmdBase.h"
#include "CmdDef.h"
#include "CmdRegister.h"
#include "CmdParse.h"
#include "CmdBuild.h"

typedef int (* PFuncDataTransHandler)(const unsigned char * pucData, unsigned int nDataLen, unsigned int nDataTransMethod);

class CCmdManager
{
public:
    static CCmdManager *getInstance();
    static int handleCmdData(unsigned char * pucCmdData, unsigned int nCmdDataLen,
                            void * pvUsrArg);
    // // for single command Id
    // static int dispatchCmd(unsigned char ucCmdId, const unsigned char * pucCmdDataPkg,
    //                         unsigned int nDataPkgLen);
    static int dispatchCmd(T_CommandId * ptCommandId, const unsigned char * pucCmdDataPkg,
                            unsigned int nDataPkgLen);
    static void registDataTransHandler(PFuncDataTransHandler pFuncDataTransHandler);
    // // for single command Id
    // static int makeCmdAndSend(unsigned char ucCmdId, const unsigned char * pucCmdDataPkg,
    //                             unsigned int nDataPkgLen, unsigned int nDataTransMethod = eDTMTcp);

    static int makeCmdAndSend(unsigned char ucCmdFc, const unsigned char * pucCmdParam,
                                unsigned int nCmdParamLen, unsigned int nDataTransMethod = eDTMUdp);

    CCmdManager(const CCmdManager &) = delete;
    CCmdManager & operator=(const CCmdManager &) = delete;

private:
    CCmdManager()
    {}

    static std::mutex m_MutexForDispatch;
    static std::mutex m_MutexForSend;
    static CCmdRegister *m_pclsCmdRegister;
    static CCmdParser m_clsCmdParser;
    static CCmdBuild m_clsCmdBuild;
    static PFuncDataTransHandler m_pFuncDataTransHandler;
};

#endif // COMMAND_MANAGER_H
