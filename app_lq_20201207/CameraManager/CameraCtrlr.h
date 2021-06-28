
#ifndef CAMERA_CTRLR_H
#define CAMERA_CTRLR_H

#include "Camera.h"
#include "Log.h"

class CameraCtrlr
{
public:
    CameraCtrlr(unsigned int camera_type = CT_DALSA_LINEAR);
    ~CameraCtrlr();

    /*
     * user_handler_ptr：图像数据处理函数
     * usr_ptr：用户传递给 user_handler_ptr 图像数据处理函数 的参数
     */
    virtual void registerImageHandler(ImageHandler user_handler_ptr, void * usr_ptr); // 注册图像数据处理函数
    virtual void startImageGrab(); // 开始图像采集，连续采集
    virtual void stopImageGrab(); // 停止图像采集
	virtual void abortImageGrab();
	virtual void saveConvertedImage();
	virtual void startImageSnap(int index);
	virtual void toggleTurboMode();
	virtual void quitImageGrab();
	virtual void jpegEncoderImage(unsigned int image_width, unsigned int image_height, char *outputFileName);
    virtual bool setImageSize(unsigned int width, unsigned int height); // 设置相机图像大小
	virtual void printCameraIP(int index);
    virtual bool forceSetCameraIP(unsigned int *ip_addr, unsigned int *sub_mask);
    virtual bool autoSetCameraIp();
    // virtual bool forceSetDeviceIP(char *nic_name, unsigned int *ip_addr, unsigned int *sub_mask);
    // virtual bool autoSetDeviceIp();
    /* pixel_format 可选参数
     * Mono8, // 8 位灰度图
     * Mono10Packed, // 10 位灰度图
     */
    virtual bool setPixelFormat(unsigned int pixel_format); // 设置像素格式
    virtual bool setExposureTime(float exposure_time); // 设置曝光时间
    virtual bool setGain(float gain); // 设置增益
    virtual bool setImageOffset(unsigned int offset_x, unsigned int offset_y); // 设置图像的偏移量
    virtual bool setAcquisitionFrameRate(unsigned int frame_rate); // 设置帧率，非触发模式下才有效
    virtual bool enableTriggerMode(bool enable_trigger); // 使能或者关闭触发功能
    virtual bool setDefaultSettings();  //恢复相机默认配置

    /* trigger_mode 可选参数
     * TS_LINE0, // 外触发线0触发
     * TS_LINE1, // 外触发线1触发
     * TS_LINE2, // 外触发线2触发
     * TS_LINE3, // 外触发线3触发
     * TS_SOFT,  // 软件触发
     */
    virtual bool setTriggerSource(unsigned int trigger_source); // 设置触发源
	virtual bool setTriggerSelector(unsigned int trigger_selector);
    /*
     * TA_RISING_EDGE      = 0, // 上升沿触发
     * TA_FALLING_EDGE     = 1, // 下降沿触发
     * TA_LEVEL_HIGH       = 2, // 高电平沿触发
     * TA_LEVEL_LOW        = 3, // 低电平沿触发
     * TA_ANY_EDGE         = 4, // 任何沿触发
     * Dalsa相机目前只用到TA_RISING_EDGE、TA_FALLING_EDGE以及TA_ANY_EDGE
     */
    virtual bool setTriggerActivation(unsigned int trigger_act);    // 设置触发沿特性
    virtual bool setTriggerDelay(float trigger_delay);
    virtual bool setTriggerDelaySrc(unsigned int trigger_delay_src);
    virtual bool setTriggerLineCnt(unsigned int trigger_line_cnt);
    virtual bool setTriggerFrameCnt(unsigned int trigger_frame_cnt);
    virtual bool setLineSelector(unsigned int line_selector);
    virtual bool setLineFormat(unsigned int line_format);
    virtual bool setLineDetectionLevel(unsigned int input_lv);
    virtual bool setLineDebouncingPeriod(unsigned long long deb_period);
    virtual bool softTrigger(); // 对相机进行一次软触发，必须使能触发，且触发源是 软件触发

    virtual bool setAcquisitionFrameRateEnable(bool bEnable); // 使能采集帧率设置

    /*
     * 在设置参数前，最好调用 isCameraReady 进行判断
     */
    virtual bool isCameraReady(); // 返回相机已经是否已经准备好
    virtual bool waitCameraRead(); // 等待相机就绪
    virtual bool isCameraGrabbingImages(); // 返回相机是否正在采集图像
    virtual bool getImageSize(unsigned int * width_ptr, unsigned int * height_ptr); // 获取相机图像大小
//    virtual bool getFrameSize(unsigned int *frame_size);    // 获取一帧图像的大小
    virtual bool getImageOffset(unsigned int * offset_x_ptr, unsigned int * offset_y_ptr); // 获得图像的偏移量
    virtual bool getTriggerSource(unsigned int * trigger_src_ptr); // 获得触发源
    virtual bool getTriggerMode(unsigned int * trigger_mode_ptr); // 获得触发是否开启
    virtual bool getCameraTemperature(float *temp_ptr); // 获得相机温度
    virtual bool getAcquisitionFrameRate(unsigned int *frame_rate);
    virtual bool getExposureTime(float *exposure_time); // 设置曝光时间
    virtual bool getGain(float *gain); // 设置增益
    virtual bool reinitBuffer();

private:
    Camera *camera_;
    unsigned int camera_type_;
};

#endif // CAMERA_CTRLR_H
