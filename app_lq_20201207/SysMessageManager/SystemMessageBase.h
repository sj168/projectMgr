
#ifndef SYSTEMMESSAGEBASE_H
#define SYSTEMMESSAGEBASE_H

#include <vector>

#include "CmdDef.h"

typedef union MsgId
{
    struct
    {
        unsigned char ucMsgFc; // priority
        unsigned char ucMsgSubId;
    };
    unsigned short wMsgId;
} T_MsgId, * PT_MsgId;

typedef struct MsgParam
{
    MsgParam();
    ~MsgParam();
    bool copyParamToBuf(const unsigned char *pucData, unsigned int nDataLen, bool bAppend = false);

    bool bMsgHandled;
    T_MsgId tMsgId;
    std::vector<unsigned char> vctParamBuf;
    void * pvArg;
    int iLastRetVal;

    MsgParam & operator=(const MsgParam & tMsgParam);
} T_MsgParam, * PT_MsgParam;

enum MsgType
{
    eMTSetCamera = eCISetCamera,
};

enum MsgSubIdOfSetCamera
{
    eMSISCStartOrStopImageGrab = eStartOrStopImageGrab,
    eMSISCSetImageSize = eSetImageSize,
};

class CSysMsgBase
{
public:
    CSysMsgBase() {};
    ~CSysMsgBase() {};

    virtual int handleSystemMessage(T_MsgParam *ptMsgParam) = 0;
};

#endif // SYSTEMMESSAGEBASE_H
