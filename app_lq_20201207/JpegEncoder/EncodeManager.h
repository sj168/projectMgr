
#ifndef ENCODEMANAGER_H
#define ENCODEMANAGER_H

#include "CmdBase.h"
#include "CmdDef.h"
#include "CmdManager.h"
#include "SystemMessageBase.h"
#include "JpegEncoder.h"
#include "Inifile.h"

typedef struct EncodeParam
{
    unsigned char nCompMode;
    float fScaleFactor;
    unsigned char nCompType;

} T_EncodeParam, * PT_EncodeParam;

typedef struct _camera_param
{
    unsigned int img_w;
    unsigned int img_h;
    unsigned int img_size;
} T_CameraParam, *PT_CameraParam;

class CEncodeMgr : public CCmdBase, public CSysMsgBase
{
public:
    CEncodeMgr();
    ~CEncodeMgr();

    virtual int cmdFunction(unsigned char ucCmdSubId, const unsigned char *pucCmdParam, unsigned int nCmdParamLen);
    virtual int handleSystemMessage(T_MsgParam * ptMsgParam);

    const unsigned char *encodeImage(const unsigned char * pucImageData, unsigned long & ulEncodedLen,
                                        char * pcFileName = nullptr, unsigned int nEncodeType = eCTNone);

    void getEncodeParam(T_EncodeParam *ptEncodeParam);
    void getCameraParam(T_CameraParam *ptCameraParam);
private:
    bool reinitEncoder(unsigned int nWidth, unsigned int nHeight);

    T_EncodeParam m_tEncodeParam;
    T_CameraParam m_tCameraParam;
    JpegEncoder *m_pclsHardwareJpegEncoder;
    CCmdManager *m_pclsCmdMgr;
    unsigned int width_val;
    unsigned int height_val;
};

#endif // ENCODEMANAGER_H
