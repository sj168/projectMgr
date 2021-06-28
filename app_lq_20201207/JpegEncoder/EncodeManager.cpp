
#include "ErrorCode.h"

#include "EncodeManager.h"
#include <unistd.h>

CEncodeMgr::CEncodeMgr()
{
    width_val = read_profile_int("setting", "WidthVal", 2048, FILE_INI_NAME);
    height_val = read_profile_int("setting", "HeightVal", 1024, FILE_INI_NAME);
    m_pclsHardwareJpegEncoder = nullptr;
    m_tEncodeParam.nCompMode = eJpegComp;
    m_tEncodeParam.fScaleFactor = 0.5f;
    m_tEncodeParam.nCompType = eCTNone;
    m_tCameraParam.img_h = height_val;
    m_tCameraParam.img_w = width_val;
    m_tCameraParam.img_size = height_val * width_val;
    m_pclsHardwareJpegEncoder = new JpegEncoder;
    m_pclsHardwareJpegEncoder->initEncoder(width_val, height_val);
    m_pclsCmdMgr = CCmdManager::getInstance();
}

CEncodeMgr::~CEncodeMgr()
{
    if (nullptr != m_pclsHardwareJpegEncoder)
    {
        delete m_pclsHardwareJpegEncoder;
        m_pclsHardwareJpegEncoder = nullptr;
    }
}
//unsigned char ucCmdFc, const unsigned char *pucCmdParam, unsigned int nCmdParamLen
int CEncodeMgr::cmdFunction(unsigned char ucCmdSubId, const unsigned char *pucCmdParam, unsigned int nCmdParamLen)
{
    int iRet = eECOK;
    int iRetLen = 2;

    switch (ucCmdSubId)
    {
        case eCompMode:         // 压缩模式 0x00不压缩  0x01 JPEG压缩
        {
            if (nCmdParamLen < 1)
            {
                iRet = eECParam;
                break;
            }
            // printf("compress mode:%02x\n", *(pucCmdParam + 1));
            
            m_tEncodeParam.nCompMode = static_cast<unsigned char>(*(pucCmdParam + 1));
            break;
        }

        case eScaleFactor:  // 压缩系数
        {
            if (nCmdParamLen < 1)
            {
                iRet = eECParam;
                break;
            }

            float fScaleFactor;
            unsigned int *pVal = reinterpret_cast<unsigned int *>(&fScaleFactor);
            *pVal = 0;
            *pVal = *(unsigned int *)(pucCmdParam + 1);
            m_tEncodeParam.fScaleFactor = fScaleFactor;
            break;
        }

        case eCompType: // 压缩类型  0x00无压缩方法  0x01 硬件压缩   0x02GPU压缩
        {
            if (nCmdParamLen < 1)
            {
                iRet = eECParam;
                break;
            }
            m_tEncodeParam.nCompType = static_cast<unsigned char>(*(pucCmdParam + 1));
            break;
        }

        default:
        {
            iRet = eECParam;
            break;
        }
    }

    unsigned char * pucRetBuf = new unsigned char[iRetLen];
    if (nullptr != pucRetBuf)
    {
        pucRetBuf[0] = (iRet == eECOK) ? eAckOk : eAckFailed;
        m_pclsCmdMgr->makeCmdAndSend(eCIAck, pucRetBuf, iRetLen, eDTMUdp);

        delete [] pucRetBuf;
    }

    return iRet;
}

int CEncodeMgr::handleSystemMessage(T_MsgParam * ptMsgParam)
{
    if (nullptr == ptMsgParam)
        return eECParam;

    if (ptMsgParam->tMsgId.ucMsgSubId == eMSISCSetImageSize)
    {
        if (ptMsgParam->vctParamBuf.size() < (2 * sizeof(unsigned int)))
        {
            return eECParam;
        }
        unsigned int nWidth = 0;
        unsigned int nHeight = 0;
        unsigned char *pucBuf = ptMsgParam->vctParamBuf.data();
        memcpy(&nWidth, pucBuf, sizeof(unsigned int));
        memcpy(&nHeight, pucBuf + sizeof(unsigned int), sizeof(unsigned int));
        m_tCameraParam.img_h = nHeight;
        m_tCameraParam.img_w = nWidth;
        m_tCameraParam.img_size = nHeight * nWidth;
        reinitEncoder(nWidth, nHeight);
    }

    return eECOK;
}

const unsigned char * CEncodeMgr::encodeImage(const unsigned char * pucImageData, unsigned long &ulEncodedLen,
                                            char *pcFileName, unsigned int nEncodeType)
{
    unsigned int nEncodeTypeSel = (eCTNone == nEncodeType) ? m_tEncodeParam.nCompType : nEncodeType;
    switch (nEncodeTypeSel)
    {
        case eCTHardware:
        {
            if (nullptr != m_pclsHardwareJpegEncoder)
            {
                return m_pclsHardwareJpegEncoder->jpegEncode(pucImageData, ulEncodedLen, pcFileName);
            }
            break;
        }

        case eCTGPU:
        {
            printf("Only hardware encode type supported.\n");
            return pucImageData;
            break;

        }

        default:
            break;
    }
        
    return nullptr;
}

bool CEncodeMgr::reinitEncoder(unsigned int nWidth, unsigned int nHeight)
{
    if (nullptr != m_pclsHardwareJpegEncoder)
    {
        delete m_pclsHardwareJpegEncoder;
        m_pclsHardwareJpegEncoder = nullptr;
    }

    m_pclsHardwareJpegEncoder = new JpegEncoder;
    if (nullptr == m_pclsHardwareJpegEncoder)
        return false;

    printf("initEncoder nWidth:%u nHeight：%u\n", nWidth, nHeight);
    return m_pclsHardwareJpegEncoder->initEncoder(nWidth, nHeight);
}

void CEncodeMgr::getEncodeParam(T_EncodeParam *ptEncodeParam)
{
    ptEncodeParam->nCompMode = m_tEncodeParam.nCompMode;
    ptEncodeParam->fScaleFactor = m_tEncodeParam.fScaleFactor;
    ptEncodeParam->nCompType = m_tEncodeParam.nCompType;

    return;
}

void CEncodeMgr::getCameraParam(T_CameraParam *ptCameraParam)
{
    ptCameraParam->img_h = m_tCameraParam.img_h;
    ptCameraParam->img_w = m_tCameraParam.img_w;
    ptCameraParam->img_size = m_tCameraParam.img_size;

    return;
}


