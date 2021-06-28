
#ifndef CAMERA_H
#define CAMERA_H

struct ImageInfo
{
    unsigned int image_width;
    unsigned int image_height;
    unsigned long long frame_num;
    unsigned int multiple_num;
};

enum CameraType
{
    CT_HAI_KANG, // 海康相机
    CT_DA_HUA, // 大华相机
    CT_DALSA_LINEAR,   // DALSA相机 added by sj @ 2020.05.12
};

enum TriggerActivation
{
    TA_RISING_EDGE      = 0, // 上升沿触发
    TA_FALLING_EDGE     = 1, // 下降沿触发
    TA_LEVEL_HIGH       = 2, // 高电平沿触发
    TA_LEVEL_LOW        = 3, // 低电平沿触发
    TA_ANY_EDGE         = 4, // 任何沿触发
};

enum TriggerDelaySrc
{
    TDS_Internal_Clock = 0,
    TDS_Line_Trigger_Signal = 1,
};

enum LineSelector
{
    LS_LINE1 = 0,
    LS_LINE2,
    LS_LINE3,
    LS_LINE4,
    LS_LINE5,
    LS_LINE6,
};

enum LineFormat
{
    LF_RS422 = 0,
    LF_Single_Ended,
};

enum InputLineDetectionLevel
{
    Threshold_for_3V3 = 0,
    Threshold_for_5V,
    Threshold_for_12V,
    Threshold_for_24V,
};

enum TriggerSource
{
    TS_LINE1, // 外触发线1触发
    TS_LINE2, // 外触发线2触发
    TS_LINE3, // 外触发线3触发
    TS_LINE4, // 外触发线4触发
    TS_ROTARY_ENCODER,  // 编码器触发
    TS_SOFT,  // 软件触发
    TS_Unknown, // 未知触发
};

enum TriggerSelector
{
	LineStart,
	FrameStart,
	FrameBurstStart,
	FrameActive,
	FrameBurstActive,
};

enum TriggerMode
{
    TM_OFF, // 触发关闭
    TM_ON, // 触发开启

    TM_Unknown, // 未知
};

enum PixelFormat
{
    Mono8,          // 8 位灰度图
    Mono10Packed,   // 10 位灰度图
	Mono12,
    Mono12Packed,   // 12 位灰度图
};

enum DeviceTemperatureSelector
{
    FPGA_Board,
};

/*
 * image_data_ptr: 图像数据首地址
 * data_size：图像数据总大小
 * image_info_ptr：图像信息，它的 nFrameNum 成员表示当前帧号
 * user_ptr：用户数据的指针
 */
typedef void (*ImageHandler)(unsigned char *image_data_ptr, unsigned int data_size, 
                                ImageInfo *image_info_ptr, void *user_ptr);

class Camera
{
public:
    Camera() {};
    virtual ~Camera() {};

    /*
     * user_handler_ptr：图像数据处理函数
     * usr_ptr：用户传递给 user_handler_ptr 图像数据处理函数 的参数
     */
    virtual void registerImageHandler(ImageHandler user_handler_ptr, void * usr_ptr) = 0; // 注册图像数据处理函数
    virtual void startImageGrab() = 0; // 开始图像采集
    virtual void stopImageGrab() = 0; // 停止图像采集
	virtual void abortImageGrab() = 0;
	virtual void saveConvertedImage() = 0;
	virtual void startImageSnap(int index) = 0;
	virtual void toggleTurboMode() = 0;
	virtual void quitImageGrab() = 0;
	virtual void jpegEncoderImage(unsigned int image_width, unsigned int image_height, char *outputFileName) = 0;
	virtual bool setImageSize(unsigned int width, unsigned int height) = 0; // 设置相机图像大小
	virtual void printCameraIP(int index) = 0;
    virtual bool forceSetCameraIP(unsigned int *ip_addr, unsigned int *sub_mask) = 0;
    virtual bool autoSetCameraIp() = 0;
    // virtual bool forceSetDeviceIP(char *nic_name, unsigned int *ip_addr, unsigned int *sub_mask) = 0;
    // virtual bool autoSetDeviceIp() = 0;

	/* pixel_format 可选参数
     * Mono8, // 8 位灰度图
     * Mono10Packed, // 10 位灰度图
     * Mono12Packed, // 12 位灰度图
     */
    virtual bool setPixelFormat(unsigned int pixel_format) = 0; // 设置像素格式
    virtual bool setExposureTime(float exposure_time) = 0; // 设置曝光时间
    virtual bool setGain(float gain) = 0; // 设置增益
    virtual bool setImageOffset(unsigned int offset_x, unsigned int offset_y) = 0; // 设置图像的偏移量
    virtual bool setAcquisitionFrameRate(unsigned int frame_rate) = 0; // 设置帧率，非触发模式下才有效
    virtual bool enableTriggerMode(bool enable_trigger) = 0; // 使能或者关闭触发功能
    virtual bool setDefaultSettings() = 0;  //恢复相机默认配置

    /* trigger_mode 可选参数
     * TS_LINE0, // 外触发线0触发
     * TS_LINE1, // 外触发线1触发
     * TS_LINE2, // 外触发线2触发
     * TS_LINE3, // 外触发线3触发
     * TS_SOFT,  // 软件触发
     */
    virtual bool setTriggerSource(unsigned int trigger_source) = 0; // 设置触发源
	virtual bool setTriggerSelector(unsigned int trigger_selector) = 0;

    /*
     * TA_RISING_EDGE      = 0, // 上升沿触发
     * TA_FALLING_EDGE     = 1, // 下降沿触发
     * TA_LEVEL_HIGH       = 2, // 高电平沿触发
     * TA_LEVEL_LOW        = 3, // 低电平沿触发
     * TA_ANY_EDGE         = 4, // 任何沿触发
     * Dalsa相机目前只用到TA_RISING_EDGE、TA_FALLING_EDGE以及TA_ANY_EDGE
     */
    virtual bool setTriggerActivation(unsigned int trigger_act) = 0;    // 设置触发沿特性
    virtual bool setTriggerDelay(float trigger_delay) = 0;
    virtual bool setTriggerDelaySrc(unsigned int trigger_delay_src) = 0;
    virtual bool setTriggerLineCnt(unsigned int trigger_line_cnt) = 0;
    virtual bool setTriggerFrameCnt(unsigned int trigger_frame_cnt) = 0;
    virtual bool setLineSelector(unsigned int line_selector) = 0;
    virtual bool setLineFormat(unsigned int line_format) = 0;
    virtual bool setLineDetectionLevel(unsigned int input_lv) = 0;
    virtual bool setLineDebouncingPeriod(unsigned long long deb_period) = 0;
    virtual bool softTrigger() = 0; // 对相机进行一次软触发，必须使能触发，且触发源是 软件触发

    virtual bool setAcquisitionFrameRateEnable(bool bEnable) = 0; // 使能采集帧率设置

    /*
     * 在设置参数前，最好调用 isCameraReady 进行判断
     */
    virtual bool isCameraReady() = 0; // 返回相机已经是否已经准备好
    virtual bool waitCameraRead() = 0; // 等待相机就绪
    virtual bool isCameraGrabbingImages() = 0; // 返回相机是否正在采集图像
    virtual bool getImageSize(unsigned int * width_ptr, unsigned int * height_ptr) = 0; // 获取相机图像大小
    virtual bool getImageOffset(unsigned int * offset_x_ptr, unsigned int * offset_y_ptr) = 0; // 获得图像的偏移量
    virtual bool getTriggerSource(unsigned int * trigger_src_ptr) = 0; // 获得触发源
    virtual bool getTriggerMode(unsigned int * trigger_mode_ptr) = 0; // 获得触发是否开启
    virtual bool getCameraTemperature(float *temp_ptr) = 0; // 获得相机温度
    virtual bool getAcquisitionFrameRate(unsigned int *frame_rate) = 0;
    virtual bool getExposureTime(float *exposure_time) = 0; // 设置曝光时间
    virtual bool getGain(float *gain) = 0; // 设置增益
    virtual bool reinitBuffer() = 0;

private:
    virtual bool initCameraConnection() = 0; // 初始化相机连接
};

#endif // CAMERA_H
