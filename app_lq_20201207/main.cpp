
#include <iostream>
#include <cstring>

#include "ErrorCode.h"
#include "DataTransfer.h"
#include "CmdDef.h"
#include "CmdManager.h"
#include "CameraManager.h"
#include "DataType.h"
#include "SystemMessageManager.h"
#include "ProcessStatistics.h"
#include "EncodeManager.h"
#include "Timer.h"
#include "DeviceManager.h"
#include "Log.h"

#include <signal.h>

using namespace std;
/*
static void catch_sigsegv()
{
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_sigaction = sigsegv_handler;
    action.sa_flags = SA_SIGINFO;
    if(sigaction(SIGSEGV, &action, NULL) < 0)
    {
        perror("sugaction");
    }
}
*/
int main(int argc, char * argv[])
{
    printf("Compiled @ %s %s\n", __DATE__, __TIME__);
	printf("main\n");

	CLog *clsLog = CLog::getInstance();
	clsLog->initLogFile();
	clsLog->recordLog(1, 0, __FILE__, __func__, __LINE__, "Start log");

    CSysMsgMgr clsSysMsgMgr;
    CCmdManager *pclsCmdManager = CCmdManager::getInstance();
    CDataTransfer clsDataTransfer;
    CDataHandler clsDataHandler;

    clsDataTransfer.registDataHandler(pclsCmdManager->handleCmdData, nullptr);
    pclsCmdManager->registDataTransHandler(CDataTransfer::transData);
    
    CCmdRegister::registCmdHandler(eCISetNetParam, &clsDataTransfer);
    usleep(2000);       // 等待算法初始化完成
    CCameraManager clsCameraManager;
    clsCameraManager.setCirQueue(clsDataHandler.getCirQueue());
    // clsCameraManager.setCirQueue(clsRawImageTransfer.getCirQueue());
    CCmdRegister::registCmdHandler(eCISetCamera, &clsCameraManager);

    CCmdRegister::registCmdHandler(eCILQDataType, CDataType::getInstance());

    CProcStat *pclsProcStat = CProcStat::getInstance();
    CCmdRegister::registCmdHandler(eCILQDevProcStat, pclsProcStat);
    CSysMsgMgr::registMsg(eMTSetCamera, pclsProcStat);
    
    CEncodeMgr clsEncodeMgr;
    clsDataHandler.setEncoder(&clsEncodeMgr);

    CCmdRegister::registCmdHandler(eCISetImgCompMode, &clsEncodeMgr);
    CSysMsgMgr::registMsg(eCISetImgCompMode, &clsEncodeMgr);
    CSysMsgMgr::registMsg(eCISetCamera, &clsCameraManager); //注册重新分配接收buffer

    CDeviceManager *pclsDeviceManager = CDeviceManager::getInstance();
    CCmdRegister::registCmdHandler(eCIManagerLQDevStat, pclsDeviceManager);
    
    // 定时向上位机发送相机当前设置
    CTimer clsTimer;
    // catch_sigsegv();
    // pressEnterToExit();
    while (1)
    {
        clsSysMsgMgr.handleMsg();
        usleep(1000);
    }

    return eECOK;
}
