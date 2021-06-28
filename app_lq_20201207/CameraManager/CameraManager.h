
#ifndef CAMERA_MANAGER_H
#define CAMERA_MANAGER_H

#include "CmdBase.h"
#include "CmdManager.h"
#include "CameraCtrlr.h"
#include "DataHandler.h"
#include "DataPkg.h"
#include "ProcessStatistics.h"
#include "Inifile.h"

class CCameraManager : public CCmdBase , public CSysMsgBase
{
public:
    CCameraManager();
    ~CCameraManager();

    // // for single command Id
    // virtual int cmdFunction(const unsigned char * pucCmdDataPkg, unsigned int nDataLen);
    virtual int cmdFunction(unsigned char ucCmdSubId, const unsigned char *pucCmdParam, unsigned int nCmdParamLen);
    virtual int handleSystemMessage(T_MsgParam * ptMsgParam);

    void setCirQueue(CCirQueue<T_DataPkg> * pCirQueue);
    unsigned int getTriggerSource();
    unsigned int getTriggerMode();
    static bool getCameraTemperature(float *pfTemperature);
    static bool getImageSize(unsigned int *width_ptr, unsigned int *height_ptr);    // 获取相机图像大小
    static bool getImageOffset(unsigned int *offset_x_ptr, unsigned int *offset_y_ptr); 
    static bool getAcquisitionFrameRate(unsigned int *frame_rate);
    static bool getExposureTime(float *exposure_time);
    static bool getGain(float *gain); 

private:
    /*
     * image_data_ptr: 图像数据首地址
     * data_size：图像数据总大小
     * image_info_ptr：图像信息，它的 nFrameNum 成员表示当前帧号
     * user_ptr：用户数据的指针
     */
    static void handleImageData(unsigned char * pucImageData, unsigned int nImageDataSize, 
                                ImageInfo * ptFrameInfo, void * pvUsrArg);
    static CCirQueue<T_DataPkg> * m_pCirQueue;
    static CProcStat * m_pclsProcStat;

    static CameraCtrlr m_clsCameraCtrlr;
//    CameraCtrlr m_clsCameraCtrlr;
    CCmdManager *m_pclsCmdMgr;
};

#endif // CAMERA_MANAGER_H
