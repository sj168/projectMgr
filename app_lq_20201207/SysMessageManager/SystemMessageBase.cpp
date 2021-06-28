
#include <cstring>

#include "SystemMessageBase.h"

MsgParam::MsgParam()
{
    bMsgHandled = true;
    tMsgId.wMsgId = 0;
    pvArg = nullptr;
    iLastRetVal = 0;
}

MsgParam::~MsgParam()
{
    bMsgHandled = true;
    tMsgId.wMsgId = 0;
    vctParamBuf.clear();
    pvArg = nullptr;
    iLastRetVal = 0;
}

bool MsgParam::copyParamToBuf(const unsigned char * pucData, unsigned int nDataLen, bool bAppend)
{
    if (nullptr == pucData || 0 == nDataLen)
        return false;
    
    if (!bAppend)
        vctParamBuf.clear();
    
    for (unsigned int i = 0; i < nDataLen; ++i)
        vctParamBuf.push_back(pucData[i]);

    return true;
}

MsgParam & MsgParam::operator=(const MsgParam & tMsgParam)
{
    if (this != &tMsgParam)
    {
        vctParamBuf = tMsgParam.vctParamBuf;
        bMsgHandled = tMsgParam.bMsgHandled;
        tMsgId.wMsgId = tMsgParam.tMsgId.wMsgId;
        pvArg = tMsgParam.pvArg;
    }

    return *this;
}
