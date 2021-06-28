#include "CmdParse.h"
#include "ErrorCode.h"
#include "CmdDef.h"

CCmdParser::CCmdParser()
{

}

CCmdParser::~CCmdParser()
{

}

int CCmdParser::parseCmd(unsigned char * pucCmdData, unsigned int nCmdDataLen, unsigned char *pucCmdFc, \
		unsigned int *nCmdParamLen, unsigned char **ppucCmdParam)
{
    // printf("Command parse function called\n");
    if (nullptr == pucCmdData || 0 == nCmdDataLen)
    {
		printf("1\n");
        return eECParam;
    }

	if(pucCmdData[eCPCmdHeadPos] != CMD_HEAD || pucCmdData[eCPCmdSubHeadPos] != CMD_SUB_HEAD)
	{
		printf("2\n");
		return eECCmdHead;
	}

	if(pucCmdData[nCmdDataLen - 1] != CMD_TAIL)
	{
		printf("3\n");
		return eECCmdTail;
	}

	if(pucCmdFc != nullptr)
	{
		*pucCmdFc = pucCmdData[eCPCmdFcPos];
	}

	memcpy(nCmdParamLen, pucCmdData + eCPCmdParamLenPos, sizeof(unsigned int));

	*ppucCmdParam = pucCmdData + eCPCmdParamPos;

    return eECOK;
}
