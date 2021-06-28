
#ifndef CMD_BUILD_H
#define CMD_BUILD_H

#include "CmdParse.h"

class CCmdBuild
{
public:
    CCmdBuild();
    ~CCmdBuild();

    // 构建命令包，传入功能码FC、参数首地址、参数长度，并将构建的命令填入数据包地址m_pucCmdData，以及返回数据包总长度pnCmdDataLen
    int buildCmd(unsigned char ucCmdFc, unsigned int nCmdParamLen, unsigned char *pucCmdParam, unsigned int * pnCmdDataLen);

    unsigned char *m_pucCmdData;    // 数据包地址
    unsigned int m_nCmdDataLen;     // 数据包总大小
};

#endif // CMD_BUILD_H
