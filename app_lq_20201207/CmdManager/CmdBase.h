#ifndef CMD_BASE_H
#define CMD_BASE_H

class CCmdBase
{
public:
    CCmdBase() {};
    virtual ~CCmdBase() {};

public:
    // 作为所有命令执行函数的基类，每个继承于该类都必须实现   subId为参数部分首字节 pucCmdParam参数部分地址  nCmdParamLen参数长度
    virtual int cmdFunction(unsigned char ucCmdSubId, const unsigned char *pucCmdParam, unsigned int nCmdParamLen) = 0;
};

#endif