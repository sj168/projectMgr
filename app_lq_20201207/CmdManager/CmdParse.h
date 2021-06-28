#ifndef CMDPARSE_H
#define CMDPARSE_H

#include <stdio.h>
#include <string.h>

class CCmdParser
{
public:
	CCmdParser();
	~CCmdParser();

	// 传入整个指令包首地址和长度，返回功能码FC、参数长度、参数首地址（子命令ID存放于参数部分首字节）
	int parseCmd(unsigned char *pucCmdData, unsigned int nCmdDataLen, unsigned char *pucCmdFc, unsigned int *pucCmdParamLen, unsigned char **ppucCmdParam);
};

#endif
