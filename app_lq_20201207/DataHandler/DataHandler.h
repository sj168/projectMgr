
#ifndef DATAHANDLER_H
#define DATAHANDLER_H

#include <pthread.h>

#include "CirQueue.h"
#include "CmdManager.h"
#include "CameraManager.h"
#include "DataPkg.h"
#include "DataType.h"
#include "EncodeManager.h"
#include "Inifile.h"

typedef int (* PFuncDataTransHandler)(const unsigned char *pucData, unsigned int nDataLen, unsigned int nDataTransMethod);

class CDataHandler
{
public:
    CDataHandler();
    ~CDataHandler();
    void exitImageHandle();
    void enableImageHandling(bool bEnable);
    CCirQueue<T_DataPkg> *getCirQueue();
    void setEncoder(CEncodeMgr * pclsEncodeMgr);

public:
    static unsigned char ucData_t;

private:
    static void *imageDataHandlerThread(void * pvArg);
    static const unsigned char *addCmdDataFrame(T_DataPkg & tDataPkg, const unsigned char * pucData,
                                                unsigned int nDataLen, bool bCopyHead = true);
    static void clearCurrentDataFrameLen();
    static const unsigned char *generateTaskPathName();
    static unsigned char *m_pucTaskNameStr;
    static unsigned char *m_pucCmdDataFrame;
    static unsigned int m_nCmdDataFrameCurLen;
    static unsigned int m_nCmdDataFrameTotalLen;
    static CEncodeMgr *m_pclsEncodeMgr;

    CCirQueue<T_DataPkg> m_cirQueue;
    pthread_t m_thread;
    bool m_bHandle;
    bool m_bRunning;
    static bool wr_flag;
    static int perPrint;
    CCmdManager *m_pclsCmdMgr;
};

#endif // DATAHANDLER_H
