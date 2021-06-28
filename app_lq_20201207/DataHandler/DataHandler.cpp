
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "DataHandler.h"
#include "CmdDef.h"
#include "Camera.h"
#include "EncodeManager.h"
//#include "EmbeddedLQ.h"
//#include "LQDataType.h"

unsigned char *CDataHandler::m_pucCmdDataFrame = nullptr;
unsigned char *CDataHandler::m_pucTaskNameStr = nullptr;
unsigned int CDataHandler::m_nCmdDataFrameCurLen = eCPCmdSubCmdSize + sizeof(unsigned long long) + 2 * sizeof(unsigned int);
unsigned int CDataHandler::m_nCmdDataFrameTotalLen = 0;
CEncodeMgr *CDataHandler::m_pclsEncodeMgr = nullptr;
unsigned char CDataHandler::ucData_t = eTDIImageData;
bool CDataHandler::wr_flag = true;
int CDataHandler::perPrint = 10;

CDataHandler::CDataHandler() : m_cirQueue(128)
{
    m_pclsCmdMgr = CCmdManager::getInstance();
    m_bHandle = true;
    m_bRunning = true;
//    lq_global_env_init();
//    int init_alg[] = {LQ_GX};
    char path[64] = {0};
    read_profile_string("setting", "AlgorithmModelPath", path, sizeof (path), "", FILE_INI_NAME);
//    lq_set_init_alg(init_alg, 1, path);
 //   lq_set_type(LQ_GX);
    pthread_create(&(m_thread), nullptr, imageDataHandlerThread, this);
}

CDataHandler::~CDataHandler()
{
    exitImageHandle();
}

void CDataHandler::exitImageHandle()
{
    m_bRunning = false;
 //   lq_free();
    pthread_join(m_thread, nullptr);
}

void CDataHandler::enableImageHandling(bool bEnable)
{
    m_bHandle = bEnable;
}

CCirQueue<T_DataPkg> * CDataHandler::getCirQueue()
{
    return &m_cirQueue;
}

void CDataHandler::setEncoder(CEncodeMgr * pclsEncodeMgr)
{
    m_pclsEncodeMgr = pclsEncodeMgr;
}

void *CDataHandler::imageDataHandlerThread(void * pvArg)
{
    CDataHandler *pDataHandler = (CDataHandler *)pvArg;
    bool bOk = true;
    unsigned int nDataType;
    unsigned int nDataRate;
    int iLen;
    unsigned long ulDataLen;
    const unsigned char *pucDataBuf;
    CDataType *pDataType = CDataType::getInstance();    // 获取上位机设置的数据传输类型和数据传输速率
    T_EncodeParam pt_encodeParam = {0};
    T_CameraParam pt_cameraParam = {0};
    T_MsgParam pt_msgParam;
    char imgFile[64] = {0};
    int fd = 0, ret = 0;
    perPrint = read_profile_int("setting", "PrintPerFrame", 100, FILE_INI_NAME);
   // LQ_RESULT pt_LQ_Result;
   // LQ_DATA pt_LQ_Data;
    FILE *fp = fopen("/home/tye/Desktop/test.txt", "w+");

    // double t1, t2;
    while (pDataHandler->m_bRunning)
    {
        if (pDataHandler->m_bHandle)
        {
            iLen = pDataHandler->m_cirQueue.length();
            if (iLen > 0)
            {
                m_pclsEncodeMgr->getEncodeParam(&pt_encodeParam);
                m_pclsEncodeMgr->getCameraParam(&pt_cameraParam);

                T_DataPkg & tDataPkg = pDataHandler->m_cirQueue.cirQueueOut(&bOk);
                if (!bOk)
                    continue;
                
                if (tDataPkg.pucImageData == nullptr)
                    continue;

                if(tDataPkg.nImageDataCurrentLen < pt_cameraParam.img_size)
                {
                    printf("Image size is unexpected. real resolution:%u * %u\n", tDataPkg.nWidth, tDataPkg.nHeight);
                    continue;
                }

                nDataType = pDataType->getDataType();
                switch (nDataType)
                {
                    case eTDSaveToLocal:
                    {}
                    case eTDIImageData:
                    {
                        nDataRate = pDataType->getDataRate();
                        if (0 != nDataRate && tDataPkg.ullFrameNum % nDataRate != 0)
                            continue;
                        else
                        {
//                            ucData_t = eTDIImageData;
                            ucData_t = nDataType;
                            if(pt_encodeParam.nCompMode == eNoComp)
                            {
                                pucDataBuf = tDataPkg.pucImageData;
                                ulDataLen = tDataPkg.nImageDataCurrentLen;;
                            }
                            else if(pt_encodeParam.nCompMode == eJpegComp)
                            {
                                // t1 = GET_MS();
                                ulDataLen = tDataPkg.nImageDataCurrentLen;
//                                printf("dataHandler addr:%x  ulDataLen:%lu\n", tDataPkg.pucImageData, ulDataLen);
                                pucDataBuf = m_pclsEncodeMgr->encodeImage(tDataPkg.pucImageData, ulDataLen, nullptr, eCTHardware);
                                // t2 = GET_MS();
                                // printf("time=====%.2f\n", t2-t1);
                            }
                            // printf("pucDataBuf: %08x    pucImageData: %08x  ulDataLen:%ld\n", pucDataBuf, tDataPkg.pucImageData, ulDataLen);
                            if(pucDataBuf != nullptr)
                            {
                                addCmdDataFrame(tDataPkg, pucDataBuf, ulDataLen);
                                pDataHandler->m_pclsCmdMgr->makeCmdAndSend(eCITransData, m_pucCmdDataFrame, m_nCmdDataFrameCurLen, eDTMTcp);
                            }
                            else
                            {
                                fprintf(fp, "pucDataBuf is nullptr.\n");
                                tDataPkg.bDataHandled = true;
                                continue;
                            }

                            if(tDataPkg.ullFrameNum % perPrint == 0)
                                printf("frame_n: %llu  width: %u  height:%u  ulDataLen:%lu  send frame len:%u\n", tDataPkg.ullFrameNum, tDataPkg.nWidth, tDataPkg.nHeight, ulDataLen, m_nCmdDataFrameCurLen);

                            clearCurrentDataFrameLen();
                        }

                        break;
                    }
                    case eTDIAlgoHandle:
                    {
			    /*
                        ucData_t = eTDIAlgoHandle;

                        pt_LQ_Data.data = tDataPkg.pucImageData;
                        pt_LQ_Data.image_w = tDataPkg.nWidth;
                        pt_LQ_Data.image_h = tDataPkg.nHeight;
                        pt_LQ_Data.image_c = 1;
                        lq_recognize(&pt_LQ_Data, pt_LQ_Result);
                        if(pt_LQ_Result.is_lq)
                        {
                            printf("is lq\n");
                        }
                        else
                        {
                            printf("is not lq\n");
                        }
*/
                        break;
                    }
                    case eTDSaveToDevice:
                    {
                    printf("save to device.\n");

                        if(tDataPkg.ullFrameNum == 1 || wr_flag)
                        {
                            generateTaskPathName();
                            wr_flag = false;
                        }
                        if(pt_encodeParam.nCompMode == eNoComp)
                            sprintf(imgFile, "%s/%llu", m_pucTaskNameStr, tDataPkg.ullFrameNum);
                        else if(pt_encodeParam.nCompMode == eJpegComp)
                            sprintf(imgFile, "%s/%llu.jpeg", m_pucTaskNameStr, tDataPkg.ullFrameNum);
                        // printf("imgFile:%s\n", imgFile);

                        fd = open(imgFile, O_RDWR | O_CREAT /*| O_DIRECT*/, 0666);
                        if(fd < 0)
                        {
                            perror("open");
                        }
                        if(pt_encodeParam.nCompMode == eNoComp)
                        {
                            pucDataBuf = tDataPkg.pucImageData;
                            ulDataLen = tDataPkg.nWidth * tDataPkg.nHeight;
                        }
                        else if(pt_encodeParam.nCompMode == eJpegComp)
                        {
                            pucDataBuf = m_pclsEncodeMgr->encodeImage(tDataPkg.pucImageData, ulDataLen, nullptr, eCTHardware);
                        }

                        ret = write(fd, pucDataBuf, ulDataLen);
                        if(ret < 0)
                        {
                            printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "write error.");
                        }

                        close(fd);
                        break;
                    }
//                    case eTDSaveToLocal:
//                    {
//                    printf("save to local.\n");
//                        nDataRate = pDataType->getDataRate();
//                        if (0 != nDataRate && tDataPkg.ullFrameNum % nDataRate != 0)
//                            continue;
//                        else
//                        {
//                            ucData_t = eTDSaveToLocal;
//    //                            ucData_t = nDataType;
//                            if(pt_encodeParam.nCompMode == eNoComp)
//                            {
//                                pucDataBuf = tDataPkg.pucImageData;
//                                ulDataLen = tDataPkg.nImageDataCurrentLen;;
//                            }
//                            else if(pt_encodeParam.nCompMode == eJpegComp)
//                            {
//                                // t1 = GET_MS();
//                                ulDataLen = tDataPkg.nImageDataCurrentLen;
//    //                                printf("dataHandler addr:%x  ulDataLen:%lu\n", tDataPkg.pucImageData, ulDataLen);
//                                pucDataBuf = m_pclsEncodeMgr->encodeImage(tDataPkg.pucImageData, ulDataLen, nullptr, eCTHardware);
//                                // t2 = GET_MS();
//                                // printf("time=====%.2f\n", t2-t1);
//                            }
//                            // printf("pucDataBuf: %08x    pucImageData: %08x  ulDataLen:%ld\n", pucDataBuf, tDataPkg.pucImageData, ulDataLen);
//                            if(pucDataBuf != nullptr)
//                            {
//                                addCmdDataFrame(tDataPkg, pucDataBuf, ulDataLen);
//                                pDataHandler->m_pclsCmdMgr->makeCmdAndSend(eCITransData, m_pucCmdDataFrame, m_nCmdDataFrameCurLen, eDTMTcp);
//                            }
//                            else
//                            {
//                                fprintf(fp, "pucDataBuf is nullptr.\n");
//                                tDataPkg.bDataHandled = true;
//                                continue;
//                            }

//                            if(tDataPkg.ullFrameNum % perPrint == 0)
//                                printf("frame_n: %llu  width: %u  height:%u  ulDataLen:%lu  send frame len:%u\n", tDataPkg.ullFrameNum, tDataPkg.nWidth, tDataPkg.nHeight, ulDataLen, m_nCmdDataFrameCurLen);

//                            clearCurrentDataFrameLen();
//                        }

//                        break;
//                    }
                    default:
                        break;
                }
            }
            else
                usleep(1000);
        }
        else
            usleep(1000);
    }

    return nullptr;
}

// 图像数据打包，增加协议包头包尾
const unsigned char *CDataHandler::addCmdDataFrame(T_DataPkg & tDataPkg, const unsigned char *pucData,
             unsigned int nDataLen, bool bCopyHead)
{
    unsigned int nLen = m_nCmdDataFrameCurLen + nDataLen;
        
    if (nullptr == m_pucCmdDataFrame)
    {
        m_pucCmdDataFrame = new unsigned char[nLen];
        if (nullptr == m_pucCmdDataFrame)
            return nullptr;

        m_nCmdDataFrameTotalLen = nLen;
    }
    else if (m_nCmdDataFrameTotalLen < nLen)
    {
        unsigned char * pucTmp = new unsigned char[nLen];
        if (nullptr == pucTmp)
            return nullptr;

        memcpy(pucTmp, m_pucCmdDataFrame, m_nCmdDataFrameCurLen);

        delete [] m_pucCmdDataFrame;
        m_pucCmdDataFrame = pucTmp;

        m_nCmdDataFrameTotalLen = nLen;
    }

    if (bCopyHead)
    {
        m_pucCmdDataFrame[0] = ucData_t;
        memcpy(m_pucCmdDataFrame + eCPCmdSubCmdSize, &tDataPkg.ullFrameNum, sizeof(unsigned long long));
        memcpy(m_pucCmdDataFrame + eCPCmdSubCmdSize + sizeof(unsigned long long), &tDataPkg.nWidth, sizeof(unsigned int));
        memcpy(m_pucCmdDataFrame + eCPCmdSubCmdSize + sizeof(unsigned long long) + sizeof(unsigned int), &tDataPkg.nHeight, sizeof(unsigned int));
    }

    memcpy(m_pucCmdDataFrame + m_nCmdDataFrameCurLen, pucData, nDataLen);

    m_nCmdDataFrameCurLen += nDataLen;
    return m_pucCmdDataFrame;
}

void CDataHandler::clearCurrentDataFrameLen()
{
    m_nCmdDataFrameCurLen = eCPCmdSubCmdSize + sizeof(unsigned long long) + 2 * sizeof(unsigned int);
}

const unsigned char *CDataHandler::generateTaskPathName()
{
    if(m_pucTaskNameStr == nullptr)
    {
        m_pucTaskNameStr = new unsigned char[32];
        if(m_pucTaskNameStr == nullptr)
            return nullptr;
    }
    char path[16] = {0};
    read_profile_string("setting", "StoragePath", path, sizeof (path), "", FILE_INI_NAME);
    time_t t;
    time(&t);
    struct tm *p = localtime(&t);
    sprintf((char *)m_pucTaskNameStr, "%s%04u%02u%02u%02u%02u%02u", path, p->tm_year + 1900, p->tm_mon + 1, p->tm_mday,
            p->tm_hour, p->tm_min, p->tm_sec);

    if (access((const char *)m_pucTaskNameStr, F_OK) != 0)
    {
        if (mkdir((const char *)m_pucTaskNameStr, 0777) != 0) {
            printLog(CLog::FILE_AND_TERMINAL, CLog::ERROR, "mkdir taskName failed.");
        }
    }

    return m_pucTaskNameStr;
}
