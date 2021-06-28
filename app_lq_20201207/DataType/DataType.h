/* 该类用于获取上位机设置数据传输类型以及数据传输速率 
 * 数据传输类型：
 *          0x00：仅传输提取的坐标值
 *          0x01：采到图像后，提取光条结果，将图像和提取的坐标值一起上传
 *          0x02：仅上传图像数据 (例如在标定时，由上位机获得图像并提取光斑中心，保存并计算标定结果)
 * 数据传输速率：
 *          数据传输间隔，数值1，2，3...表示 单位：帧 代表n帧一发
 * 存放于结构体DataTypeParam中
*/

#ifndef DATATYPE_H
#define DATATYPE_H

#include "CmdBase.h"
#include "CmdDef.h"
#include "CmdManager.h"

typedef struct DataTypeParam
{
    unsigned int nDataType; //传输数据类型
    unsigned int nDataRate;
} T_DataTypeParam, *PT_DataTypeParam;

class CDataType : public CCmdBase
{
public:
    static CDataType *getInstance();
    unsigned int getDataType();
    unsigned int getDataRate();

    virtual int cmdFunction(unsigned char ucCmdSubId, const unsigned char *pucCmdParam, unsigned int nCmdParamLen);

    CDataType(const CDataType &) = delete;
    CDataType & operator=(const CDataType &) = delete;

private:
    CDataType();

    static T_DataTypeParam m_tTDataTypeParam;
    static CCmdManager * m_pclsCmdMgr;
};

#endif // DATATYPE_H
