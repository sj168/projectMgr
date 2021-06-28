
#include <cstring>
#include <cstdio>
#include <unistd.h>

#include "ErrorCode.h"
#include "CmdDef.h"
#include "SystemMessageManager.h"
#include "CameraManager.h"

CCirQueue<T_DataPkg> * CCameraManager::m_pCirQueue = nullptr;
CProcStat * CCameraManager::m_pclsProcStat = CProcStat::getInstance();
CameraCtrlr CCameraManager::m_clsCameraCtrlr(CT_DALSA_LINEAR);

CCameraManager::CCameraManager()/* : m_clsCameraCtrlr(CT_DALSA_LINEAR)*/
{
    m_clsCameraCtrlr.registerImageHandler(handleImageData, &m_pCirQueue);

    m_pclsCmdMgr = CCmdManager::getInstance();
    while (!m_clsCameraCtrlr.isCameraReady())
    {
        sleep(1);
    }
}

CCameraManager::~CCameraManager()
{
}

void CCameraManager::handleImageData(unsigned char *pucImageData, unsigned int nImageDataSize, 
                                        ImageInfo *ptFrameInfo, void *pvUsrArg)
{
    uint64_t frame_n = ptFrameInfo->frame_num;
    m_pclsProcStat->calGrabRate(frame_n);

    if (nullptr != m_pCirQueue)
    {
        static T_DataPkg tDataPkg;

        tDataPkg.ullFrameNum = ptFrameInfo->frame_num;
        tDataPkg.nWidth = ptFrameInfo->image_width;
        tDataPkg.nHeight = ptFrameInfo->image_height;
        tDataPkg.pucImageData = pucImageData;
        tDataPkg.nImageDataCurrentLen = nImageDataSize;
        
        if (m_pCirQueue->cirQueueIn(tDataPkg) == 0)
            printf("cirQueue full\n");
        // printf("frame_num:%lld width:%d height:%d addr:%x  tDataPkg.pucImageData:%x\n", ptFrameInfo->frame_num, ptFrameInfo->image_width, ptFrameInfo->image_height, pucImageData, tDataPkg.pucImageData);
        tDataPkg.pucImageData = nullptr;
    }
}

int CCameraManager::cmdFunction(unsigned char ucCmdSubId, const unsigned char *pucCmdParam, unsigned int nCmdParamLen)
{
    static T_MsgParam tMsgParam, tMsgEncMgr, tMsgIniBuf;
    int iRet = eECOK;
    int iRetLen = 2;    // 子命令+参数，子命令放在参数部分首字节
    bool bIgnore = false;
    static bool startFlag = true;
    static bool stopFlag = false;

    unsigned int nCameraParamId = (unsigned int)ucCmdSubId;

    // if (m_clsCameraCtrlr.isCameraGrabbingImages() && nCameraParamId != eSoftTrigger)
    if(0)
    {
        iRet = eECCameraIsGrabbing;
    }
    else
    {
        if (!bIgnore)
        {
            switch (nCameraParamId)
            {
                // 开始/停止采集
                case eStartOrStopImageGrab: 
                {
                    if (pucCmdParam[1] == eStopImageGrab)
                    {
                        if(stopFlag)
                        {
                            m_clsCameraCtrlr.stopImageGrab();
                            startFlag = true;
                            stopFlag = false;
                        }
                        else
                            iRet = eECSetCameraParam;
                    }
                    else if (pucCmdParam[1] == eStartImageGrab)
                    {
                        if(startFlag)
                        {
                            m_clsCameraCtrlr.startImageGrab();
                            startFlag = false;
                            stopFlag = true;
                        }
                        else
                            iRet = eECSetCameraParam;
                    }
                        
                    CSysMsgMgr::queueMsg(&tMsgParam);

                    break;
                }
                // 设置图像大小
                case eSetImageSize:
                {
                    unsigned int nWidth = 0;
                    unsigned int nHeight = 0;

                    nWidth = *(unsigned int *)(pucCmdParam + 1);
                    nHeight = *(unsigned int *)(pucCmdParam + 5);
                    printf("Width = %d  Height = %d\n", nWidth, nHeight);
                    if (!m_clsCameraCtrlr.setImageSize(nWidth, nHeight))
                    {
                        iRet = eECSetCameraParam;
                        break;
                    }

                    tMsgIniBuf.tMsgId.ucMsgFc = eCISetCamera;
                    tMsgIniBuf.tMsgId.ucMsgSubId = eSetImageSize;
                    CSysMsgMgr::queueMsg(&tMsgIniBuf);
                    usleep(1000);
                    tMsgEncMgr.tMsgId.ucMsgFc = eCISetImgCompMode;
                    tMsgEncMgr.tMsgId.ucMsgSubId = eMSISCSetImageSize;
                    unsigned char cParamBuf[8] = {0};
                    memcpy(cParamBuf, &nWidth, sizeof(unsigned int));
                    memcpy(cParamBuf + sizeof(unsigned int), &nHeight, sizeof(unsigned int));
                    tMsgEncMgr.copyParamToBuf((const unsigned char *)cParamBuf, 2 * sizeof(unsigned int), false);
                    CSysMsgMgr::queueMsg(&tMsgEncMgr);

                    write_profile_int("setting", "WidthVal", nWidth, FILE_INI_NAME);
                    write_profile_int("setting", "HeightVal", nHeight, FILE_INI_NAME);
                    break;
                }
                // 设置曝光时间
                case eSetExposureTime:
                {
                    float nExposureTime = 0;
                    memcpy(&nExposureTime, (pucCmdParam + 1), sizeof(nExposureTime));
                    if (!m_clsCameraCtrlr.setExposureTime(nExposureTime))
                    {
                        iRet = eECSetCameraParam;
                    }

                    write_profile_double("setting", "ExpTime", nExposureTime, FILE_INI_NAME);

                    break;
                }
                // 设置增益
                case eSetGain:
                {
                    float fGain;
                    memcpy(&fGain, (pucCmdParam + 1), sizeof(fGain));
                    if (!m_clsCameraCtrlr.setGain(fGain))
                    {
                        iRet = eECSetCameraParam;
                    }

                    write_profile_double("setting", "GainVal", fGain, FILE_INI_NAME);

                    break;
                }
                // 设置采集帧率
                case eSetFrameRate:
                {
                    unsigned int frame_rate;
                    memcpy(&frame_rate, (pucCmdParam + 1), sizeof(frame_rate));

                    if (!m_clsCameraCtrlr.setAcquisitionFrameRate(frame_rate))
                    {
                        iRet = eECSetCameraParam;
                    }

                    write_profile_double("setting", "LineFreq", frame_rate, FILE_INI_NAME);

                    break;
                }
                // 设置图像偏移
                case eSetOffset:
                {
                    unsigned int off_x = 0, off_y = 0, nWidth = 0, nHeight = 0;
                    memcpy(&off_x, (pucCmdParam + 1), sizeof(off_x));

                    if (!m_clsCameraCtrlr.getImageSize(&nWidth, &nHeight))
                    {
                        printf("Get image size failed.\n");
                        iRet = eECSetCameraParam;
                        break;
                    }

                    if(off_x + nWidth > 2048)
                    {
                        printf("Offset value is invalid, set offset failed.\n");
                        iRet = eECSetCameraParam;
                        break;
                    }

                    // 设置偏移量必须为4的整数倍
                    if(off_x)
                        off_x = (off_x > 4) ? (off_x / 4 * 4) : 4;
                    printf("set offset:%d\n", off_x);

                    if (!m_clsCameraCtrlr.setImageOffset(off_x, off_y))
                    {
                        iRet = eECSetCameraParam;
                    }

                    write_profile_int("setting", "Offset_X", off_x, FILE_INI_NAME);

                    break;
                }
                // 使能触发
                case eEnableOrDisableTrigger:
                {
                    bool bEnable = (pucCmdParam[1] == eEnableTrigger);
                    if (!m_clsCameraCtrlr.enableTriggerMode(bEnable))
                        iRet = eECSetCameraParam;

                    break;
                }
                // 设置触发源
                case eSetTriggerSrc:
                {
                    unsigned char nTr_src = 0;
                    nTr_src = *(unsigned char *)(pucCmdParam + 1);
                    if (!m_clsCameraCtrlr.setTriggerSource(nTr_src))
                        iRet = eECSetCameraParam;
                    break;
                }
                // 软触发
                case eSoftTrigger:
                {
                    if (!m_clsCameraCtrlr.softTrigger())
                        iRet = eECSetCameraParam;
                    break;
                }
                // 设置触发条件
                case eSetTriggerSelector:
                {
                    unsigned char nTr_slr = 0;
                    nTr_slr = *(unsigned char *)(pucCmdParam + 1);
                    if(!m_clsCameraCtrlr.setTriggerSelector(nTr_slr))
                        iRet = eECSetCameraParam;
                    break;
                }
                // 恢复默认设置
                case eSetDefaultSettings:
                {
                    unsigned int nWidth = 2048, nHeight = 1024, Offset_x = 0, Offset_y = 0, lineFreq = 10000;
                    float expr_t = 50, gain = 1.00;

                    if(!m_clsCameraCtrlr.setDefaultSettings())
                    {
                        iRet = eECSetCameraParam;
                        break;
                    }
                    usleep(1000);

                    if (!m_clsCameraCtrlr.getImageSize(&nWidth, &nHeight))
                    {
                        printf("Get image size failed.\n");
                        iRet = eECGetParam;
                    }

                    if(!m_clsCameraCtrlr.getAcquisitionFrameRate(&lineFreq))
                    {
                        printf("Get acquisition line rate failed.\n");
                        iRet = eECGetParam;
                    }

                    if(!m_clsCameraCtrlr.getExposureTime(&expr_t))
                    {
                        printf("Get exposure time failed.\n");
                        iRet = eECGetParam;
                    }

                    if(!m_clsCameraCtrlr.getGain(&gain))
                    {
                        printf("Get gain failed.\n");
                        iRet = eECGetParam;
                    }

                    if (!m_clsCameraCtrlr.getImageOffset(&Offset_x, &Offset_y))
                    {
                        printf("Get image offset failed.\n");
                        iRet = eECGetParam;
                    }

                    tMsgIniBuf.tMsgId.ucMsgFc = eCISetCamera;
                    tMsgIniBuf.tMsgId.ucMsgSubId = eSetImageSize;
                    CSysMsgMgr::queueMsg(&tMsgIniBuf);
                    usleep(1000);
                    tMsgEncMgr.tMsgId.ucMsgFc = eCISetImgCompMode;
                    tMsgEncMgr.tMsgId.ucMsgSubId = eMSISCSetImageSize;
                    unsigned char cParamBuf[8] = {0};
                    memcpy(cParamBuf, &nWidth, sizeof(unsigned int));
                    memcpy(cParamBuf + sizeof(unsigned int), &nHeight, sizeof(unsigned int));
                    tMsgEncMgr.copyParamToBuf((const unsigned char *)cParamBuf, 2 * sizeof(unsigned int), false);
                    CSysMsgMgr::queueMsg(&tMsgEncMgr);

                    write_profile_double("setting", "ExpTime", expr_t, FILE_INI_NAME);
                    write_profile_double("setting", "LineFreq", lineFreq, FILE_INI_NAME);
                    write_profile_int("setting", "WidthVal", nWidth, FILE_INI_NAME);
                    write_profile_int("setting", "HeightVal", nHeight, FILE_INI_NAME);
                    write_profile_double("setting", "GainVal", gain, FILE_INI_NAME);
                    write_profile_int("setting", "Offset_X", Offset_x, FILE_INI_NAME);
                    break;
                }
                case eSetTriggerActivation:
                {
                    unsigned char nTr_act = 0;
                    nTr_act = *(unsigned char *)(pucCmdParam + 1);
                    if(!m_clsCameraCtrlr.setTriggerActivation(nTr_act))
                        iRet = eECSetCameraParam;
                    break;
                }
                case eSetTriggerDelay:
                {
                    unsigned int nTr_delay = 0;
                    nTr_delay = *(unsigned int *)(pucCmdParam + 1);
                    if(!m_clsCameraCtrlr.setTriggerDelay((float)nTr_delay))
                        iRet = eECSetCameraParam;
                    break;
                }
                case eSetTriggerDelaySrc:
                {
                    unsigned char nTr_delay_src = 0;
                    nTr_delay_src = *(unsigned int *)(pucCmdParam + 1);
                    if(!m_clsCameraCtrlr.setTriggerDelaySrc(nTr_delay_src))
                        iRet = eECSetCameraParam;
                    break;
                }
                case eSetLineSelector:
                {
                    unsigned char nLn_selector = 0;
                    nLn_selector = *(unsigned char *)(pucCmdParam + 1);
                    if(!m_clsCameraCtrlr.setLineSelector(nLn_selector))
                        iRet = eECSetCameraParam;
                    break;
                }
                case eSetLineFormat:
                {
                    unsigned char nLn_format = 0;
                    nLn_format = *(unsigned char *)(pucCmdParam + 1);
                    if(!m_clsCameraCtrlr.setLineFormat(nLn_format))
                        iRet = eECSetCameraParam;
                    break;
                }
                case eSetLineDetectionLevel:
                {
                    unsigned char nInput_lv = 0;
                    nInput_lv = *(unsigned char *)(pucCmdParam + 1);
                    if(!m_clsCameraCtrlr.setLineDetectionLevel(nInput_lv))
                        iRet = eECSetCameraParam;
                    break;
                }
                case eSetLineDebouncingPeriod:
                {
                    unsigned long long nDeb_period = 0;
                    nDeb_period = *(unsigned long long *)(pucCmdParam + 1);
                    if(!m_clsCameraCtrlr.setLineDebouncingPeriod(nDeb_period))
                        iRet = eECSetCameraParam;
                    break;
                }
                case eSetTriggerLineCnt:
                {
                    unsigned int nTriggerLineCnt = 0;
                    nTriggerLineCnt = *(unsigned int *)(pucCmdParam + 1);
                    if(!m_clsCameraCtrlr.setTriggerLineCnt(nTriggerLineCnt))
                        iRet = eECSetCameraParam;
                    break;
                }
                case eSetTriggerFrameCnt:
                {
                    unsigned int nTriggerFrameCnt = 0;
                    nTriggerFrameCnt = *(unsigned int *)(pucCmdParam + 1);
                    if(!m_clsCameraCtrlr.setTriggerFrameCnt(nTriggerFrameCnt))
                        iRet = eECSetCameraParam;
                    break;
                }
                case eStartImageSnap:
                {
                    int nSnapCnt = 0;
                    nSnapCnt = *(int *)(pucCmdParam + 1);
                    m_clsCameraCtrlr.startImageSnap(nSnapCnt);
                    break;
                }
                case eSetCameraIPAddr:
                {
                    m_clsCameraCtrlr.printCameraIP(0);
                    if(pucCmdParam[1] == eSetPersistentIP)
                    {
                        uint32_t ip_addr;
                        uint32_t mask_addr;
                        struct in_addr in_addr;

                        ip_addr = *(uint32_t *)(pucCmdParam + 2);
                        mask_addr = *(uint32_t *)(pucCmdParam + 2 + sizeof(uint32_t));
                        memcpy(&in_addr, &ip_addr, sizeof(uint32_t));
                        printf("ipAddr:%s\n", inet_ntoa(in_addr));
                        
                        if(!m_clsCameraCtrlr.forceSetCameraIP(&ip_addr, &mask_addr))
                            iRet = eECSetCameraParam;
                    }
                    else if(pucCmdParam[1] == eSetDHCPIP)
                    {
                        if(!m_clsCameraCtrlr.autoSetCameraIp())
                            iRet = eECSetCameraParam;
                    }
                    break;
                }
                
                default:
                {
                    iRet = eECSetCameraParam;
                    break;
                }
            }
        }
    }

    unsigned char *pucRetBuf = new unsigned char[iRetLen];
    if (nullptr != pucRetBuf)
    {
        pucRetBuf[0] = eCmdAck;
        pucRetBuf[1] = (iRet == eECOK) ? eAckOk : eAckFailed;
        m_pclsCmdMgr->makeCmdAndSend(eCIAck, pucRetBuf, iRetLen, eDTMUdp);
        delete [] pucRetBuf;
    }

    return iRet;
}

void CCameraManager::setCirQueue(CCirQueue<T_DataPkg> * pCirQueue)
{
    m_pCirQueue = pCirQueue;
}

unsigned int CCameraManager::getTriggerSource()
{
    unsigned int nTriggerSrc = TS_Unknown;
    m_clsCameraCtrlr.getTriggerSource(&nTriggerSrc);
    return nTriggerSrc;
}

unsigned int CCameraManager::getTriggerMode()
{
    unsigned int nTriggerMode = TM_Unknown;
    m_clsCameraCtrlr.getTriggerMode(&nTriggerMode);
    return nTriggerMode;
}

bool CCameraManager::getCameraTemperature(float *pfTemperature)
{
    return m_clsCameraCtrlr.getCameraTemperature(pfTemperature);
}

bool CCameraManager::getImageSize(unsigned int *width_ptr, unsigned int *height_ptr)
{
    return m_clsCameraCtrlr.getImageSize(width_ptr, height_ptr);
}

bool CCameraManager::getImageOffset(unsigned int *offset_x_ptr, unsigned int *offset_y_ptr)
{
    return m_clsCameraCtrlr.getImageOffset(offset_x_ptr, offset_y_ptr);
}

bool CCameraManager::getAcquisitionFrameRate(unsigned int *frame_rate)
{
    return m_clsCameraCtrlr.getAcquisitionFrameRate(frame_rate);
}

bool CCameraManager::getExposureTime(float *exposure_time)
{
    return m_clsCameraCtrlr.getExposureTime(exposure_time);
}

bool CCameraManager::getGain(float *gain)
{
    return m_clsCameraCtrlr.getGain(gain);
}

int CCameraManager::handleSystemMessage(T_MsgParam *ptMsgParam)
{
    if (nullptr == ptMsgParam)
        return eECParam;

    if (ptMsgParam->tMsgId.ucMsgSubId == eSetImageSize)
    {
        // 重置接收buff
        m_clsCameraCtrlr.reinitBuffer();
    }

    return eECOK;
}
