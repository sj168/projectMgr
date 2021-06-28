
#ifndef DALSA_LINEAR_CAMERA_H
#define DALSA_LINEAR_CAMERA_H


#include "Camera.h"
#include "CmdParse.h"
#include "CmdBuild.h"

/* added by sj @ 2020.05.14*/
#include "cordef.h"
#include "GenApi/GenApi.h"
#include "gevapi.h"
#include <sched.h>
#include "SapX11Util.h"
#include "X_Display_utils.h"
#include "FileUtil.h"
#include "Inifile.h"
#include "Log.h"

#define INT32 T_INT32_T
#include "JpegEncoder.h"
#undef INT32
#define IPCONFIG_MODE_PERSISTENT    	0x01
#define IPCONFIG_MODE_DHCP         		0x02
#define IPCONFIG_MODE_LLA           	0x04

#define MAX_NETIF                   	8
#define MAX_CAMERAS_PER_NETIF       	32
#define MAX_CAMERAS                 	(MAX_NETIF * MAX_CAMERAS_PER_NETIF)

#define TUNE_STREAMING_THREADS			1
#define ENABLE_BAYER_CONVERSION			1
#define USE_SYSNCHRONOUS_BUFFER_CYCLING	0
#define NUM_BUF							8   // 该值在系统内存允许的情况下可以适当增加，比如50？100？

using namespace std;
using namespace GenICam;
using namespace GenApi;

#define USER_CAMERA_CONFIG  "./xml/Teledyne DALSA/userSettingFile.txt"
#define SAVE_USR_CONFIG		1
#define LOAD_USR_CONFIG		1
#define IS_VISIBLE          true
#define USE_GEV_DISPLAY_WINDOW  0   //是否使用GigE-V窗口，1为使用，0为不使用
#define USE_HUGEPAGE    1

typedef struct tag_CONTEXT
{
	X_VIEW_HANDLE View;
	GEV_CAMERA_HANDLE handle;
    unsigned int nWidth;
    unsigned int nHeight;
    unsigned int pixFormat;
	int depth;
	int format;
	void *convertBuffer;
	void *m_latestBuffer;
	bool convertFormat;
	bool exit;		// exit to grab image
    bool isRun;     // stop to grab image
    ImageHandler user_handler_ptr; // 用户的 图像数据处理函数
    void *usr_ptr; // 用户的 图像数据处理函数 的参数
} CONTEXT, *p_CONTEXT;

struct DalsaLinearCameraCtrlrParam
{
    pthread_t camera_connect_thread_id;
	pthread_t camera_grab_thread_id;

	CONTEXT camera_sptr;
	PUINT8 bufAddress[NUM_BUF];	
	uint64_t size;
    int camera_n;                                           // 连接到系统的相机个数
    GEV_DEVICE_INTERFACE pCamera[MAX_CAMERAS];
	FILE *usr_setting_fp;       // 存放相机参数
    GenApi::CNodeMapRef CameraMapRef;
	bool m_blsLoadXMLFile;  // 控制CameraMapRef与句柄只产生一次关联，多次会出错
    bool flag_bindBuf;
    bool camera_ready; // 标记相机已经准备好
    bool grabbing_image; // 标记是否正在取流
    bool check_camera_connection; // 标记是否初始化了 相机连接
    bool pause_img_grab; // 暂停数据采集，true：相机可以设置宽高（包括恢复默认设置） false：相机不能设置宽高
    bool tri_flag;      // 触发模式标志，上位机点击软触发，自动开启抓流线程，仅第一次点击时执行。
    // bool bIsTriggerModeOn;         // 触发模式 true为打开  false为关闭，用于执行连续采集和抓图时判断是否当前为触发模式，如果为触发模式则返回
};

class DalsaLinearCamera : public Camera
{
public:
    DalsaLinearCamera();
    ~DalsaLinearCamera();

    /*
     * user_handler_ptr：图像数据处理函数
     * usr_ptr：用户传递给 user_handler_ptr 图像数据处理函数 的参数
     */
    void registerImageHandler(ImageHandler user_handler_ptr, void *usr_ptr); // 注册图像数据处理函数
    void startImageGrab(); // 开始图像采集
	void abortImageGrab();
	void saveConvertedImage();
	void startImageSnap(int index);
    void stopImageGrab(); // 停止图像采集
	void toggleTurboMode();
	void quitImageGrab();
	void jpegEncoderImage(unsigned int image_width, unsigned int image_height, char *outputFileName);
   	void printCameraIP(int index);
    bool forceSetCameraIP(unsigned int *ipAddr, unsigned int *sub_mask);
    bool autoSetCameraIp();
    // bool forceSetDeviceIP(char *nic_name, unsigned int *ipAddr, unsigned int *sub_mask);
    // bool autoSetDeviceIp();
    /* pixel_format 可选参数
     * Mono8, // 8 位灰度图
     * Mono10Packed, // 10 位灰度图
     */
    bool setPixelFormat(unsigned int pixel_format); // 设置像素格式
    bool setExposureTime(float exposure_time); // 设置曝光时间
    bool setGain(float gain); // 设置增益
    bool setImageSize(unsigned int width, unsigned int height); // 设置相机图像大小
    bool setImageOffset(unsigned int offset_x, unsigned int offset_y); // 设置图像的偏移量
    bool setAcquisitionFrameRate(unsigned int frame_rate); // 设置帧率，非触发模式下才有效
    bool enableTriggerMode(bool enable_trigger); // 使能或者关闭触发功能
    bool setDefaultSettings();  // 恢复相机默认配置

    /* trigger_mode 可选参数
     * TS_LINE0, // 外触发线0触发
     * TS_LINE1, // 外触发线1触发
     * TS_LINE2, // 外触发线2触发
     * TS_LINE3, // 外触发线3触发
     * TS_SOFT,  // 软件触发
     */
    bool setTriggerSource(unsigned int trigger_source); // 设置触发源
	bool setTriggerSelector(unsigned int trigger_selector);
    /*
     * TA_RISING_EDGE      = 0, // 上升沿触发
     * TA_FALLING_EDGE     = 1, // 下降沿触发
     * TA_LEVEL_HIGH       = 2, // 高电平沿触发
     * TA_LEVEL_LOW        = 3, // 低电平沿触发
     */
    bool setTriggerActivation(unsigned int trigger_act);    // 设置触发沿特性
    bool setTriggerDelay(float trigger_delay);
    bool setTriggerDelaySrc(unsigned int trigger_delay_src);
    bool setTriggerLineCnt(unsigned int trigger_line_cnt);
    bool setTriggerFrameCnt(unsigned int trigger_frame_cnt);
    bool setLineSelector(unsigned int line_selector);
    bool setLineFormat(unsigned int line_format);
    bool setLineDetectionLevel(unsigned int input_lv);
    bool setLineDebouncingPeriod(unsigned long long deb_period);
    bool softTrigger(); // 对相机进行一次软触发，必须使能触发，且触发源是 软件触发

    bool setAcquisitionFrameRateEnable(bool bEnable); // 使能采集帧率设置

    /*
     * 在设置参数前，最好调用 isCameraReady 进行判断
     */
    bool isCameraReady(); // 返回相机已经是否已经准备好
    bool waitCameraRead(); // 等待相机就绪
    bool isCameraGrabbingImages(); // 返回相机是否正在采集图像
    bool getImageSize(unsigned int * width_ptr, unsigned int * height_ptr); // 获取相机图像大小
    bool getImageOffset(unsigned int * offset_x_ptr, unsigned int * offset_y_ptr); // 获得图像的偏移量
    bool getTriggerSource(unsigned int * trigger_src_ptr); // 获得触发源
    bool getTriggerMode(unsigned int * trigger_mode_ptr); // 获得触发是否开启
    bool getCameraTemperature(float *temp_ptr);
    bool getAcquisitionFrameRate(unsigned int *frame_rate);
    bool getExposureTime(float *exposure_time); // 设置曝光时间
    bool getGain(float *gain); // 设置增益
    bool reinitBuffer();  //重置接收buffer

private:
    bool initCameraConnection(); // 初始化相机连接
    static bool saveUserSettings(DalsaLinearCameraCtrlrParam *camera_ctrlr_param_ptr); // 保存参数
    static bool loadUserSettings(DalsaLinearCameraCtrlrParam *camera_ctrlr_param_ptr); // 加载参数
    static void initApiLib();   //初始化GenApi lib
	static void _GetUniqueFilename(char *filename, size_t size, char *basename);
	static int IsTurboDriveAvailable(GEV_CAMERA_HANDLE handle);
	static void OutputFeatureValues(const CNodePtr &ptrFeature, FILE *fp);
	static void OutputFeatureValuePair(const char *feature_name, const char *value_string, FILE *fp);
	static void ValidateFeatureValues(const CNodePtr &ptrFeature);
    static void getFrameSize(DalsaLinearCameraCtrlrParam *camera_ctrlr_param_ptr, unsigned int *frame_size, unsigned int *width_size, unsigned *height_size);    // 获取一帧图像的大小
    static void GevGetParamThresholdInt(DalsaLinearCameraCtrlrParam *camera_ctrlr_param_ptr, const char *key, unsigned int *max, unsigned int *min);
    static void GevGetParamThresholdDouble(DalsaLinearCameraCtrlrParam *camera_ctrlr_param_ptr, const char *key, double *max, double *min);
    static bool initCamera(DalsaLinearCameraCtrlrParam *camera_ctrlr_param_ptr);
	static void *ImageGrabThread(void *);
    static void *cameraReconnectProcess(void *user_ptr);

    DalsaLinearCameraCtrlrParam camera_ctrlr_param_;
};

#endif // DALSA_LINEAR_CAMERA_H
