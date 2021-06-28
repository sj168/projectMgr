
#ifndef CMD_DEF_H
#define CMD_DEF_H

#define CMD_HEAD		0xFE
#define CMD_SUB_HEAD	0x68
#define CMD_TAIL		0x55

#define TRANS_DATA_PARAM_ID_LEN		1

typedef struct CmdInfo
{
	unsigned char ucCmdHead;
	unsigned char ucCmdSubHead;
	unsigned char ucCmdFc;
	unsigned int nDataCmdParamLen;
	unsigned char *pucCmdParam;
	unsigned char *pucCmdChecksum;
	unsigned char ucCmdTail;
	unsigned int nDataTransMethod;
} T_CmdInfo, *PT_CmdInfo;

typedef union CommandId
{
	struct
	{
		unsigned char ucCmdFc;
		unsigned char ucCmdSubId;
	};
	unsigned short wCmdId;
} T_CommandId, *PT_CommandId;

enum CmdProperty
{
	eCPCmdHeadSize = 1,
	eCPCmdSubHeadSize = 1,
	eCPCmdFcSize = 1,
	eCPCmdParamSize = 4,	
    eCPCmdSubCmdSize = 1,
	eCPCmdChecksumSize = 1,
	eCPCmdTailSize = 1,

	eCPCmdHeadPos = 0,
	eCPCmdSubHeadPos = 1,
	eCPCmdFcPos = 2,
	eCPCmdParamLenPos = 3,
	eCPCmdParamPos = 7,

	eMinCmdSize = 9,
};

enum CmdId
{
    eCIAck = 0x80,
    eCILQDataType = 0x81, // 设置设备数据上传形式
    eCISetCamera = 0x82, // 设置设备相机
    eCISetImgCompMode, // 设置图像数据压缩模式
    eCIManagerLQDevStat, // 获取设备数据（CPU MEM 利用率，温度）
    eCITransData, // 传输数据
    eCIResetFrameNum, // 复位帧号
    eCIResetLQ, // 复位设备
    eCISetNetParam, // 设置网络参数
    eCILQDevProcStat, // 设备自动返回设备处理状态

    eCIHeartBeat = 0xFF, // 心跳
};

enum CmdAck
{
    eCmdAck = 0x00,
};

enum AckFlag
{
    eAckOk = 0x00, // 外部命令执行成功
    eAckFailed = 0x01, // 外部命令执行失败

    eAckParamSize = 1,
};

enum CtrlSwitch
{
    eDisable,
    eEnable,
};

enum CmdSubIdOfDataType
{
    eDTDataType = 0x00,
    eDTDataRate = 0x01, // 数据发送速率
};

enum CmdSubIdOfSetCamera
{
    eStartOrStopImageGrab = 0x01,
    eSetImageSize = 0x02,
    eSetExposureTime = 0x03,
    eSetGain = 0x04,
    eSetOffset = 0x05,
    eSetFrameRate = 0x06,
    eEnableOrDisableTrigger = 0x07,
    eSetTriggerSrc = 0x08,
    eSoftTrigger = 0x09,
    eSelectIO = 0x0A,
    eSetIOMode = 0x0B,
    eSetIOOutputSrc = 0x0C,
    eSetIOOutputLevel = 0x0D,
    eSetIOOutputTime = 0x0E,
    eSetHardTriggerMode = 0x0F,
    eSetPixelFormat = 0x10,
    eSetTriggerSelector = 0x11,
    eSetDefaultSettings = 0x12,
    eSetTriggerActivation = 0x13,   // 设置触发沿
    eSetTriggerDelay = 0x14,        // 触发延时
    eSetTriggerDelaySrc = 0x15,     // 延时源
    eSetLineSelector = 0x16,        // 触发信号设置
    eSetLineFormat = 0x17,          // 触发信号类型：差分/方波
    eSetLineDetectionLevel = 0x18,      // 
    eSetLineDebouncingPeriod = 0x19,
    eSetTriggerLineCnt = 0x1A,
    eSetTriggerFrameCnt = 0x1B,
    eStartImageSnap = 0x1C,
    eSetCameraIPAddr = 0x1D,        // 设置相机IP和掩码
};

typedef struct _cam_stat
{
    unsigned int Width;
    unsigned int Height;
    float Exposure;
    unsigned int LineFreq;
    float Gain;
    unsigned int Offset;
    float cam_temp;
} T_CamStat, *PT_CamStat;

typedef struct _dev_stat
{
    float cpu_temp;         // cpu温度
    float cpu_util;         // cpu使用率
    float mem_util;         // 内存使用率
} T_DevStat, *PT_DevStat;

typedef struct _dev_info
{
    T_CamStat tCamStat;
    T_DevStat tDevStat;
} T_DevInfo, *PT_DevInfo;

enum CmdSubIdOfImgCompMode
{
    eCompMode,
    eScaleFactor,
    eCompType,
};

enum CompMode
{
    eJpegComp,
    eNoComp,
};

enum CompType
{
    eCTNone = 0x00,
    eCTHardware = 0x01,
    eCTGPU,
};

enum CmdSubIdOfGetLQDevStat
{
    eEnableAutoUploadLQDevStat = 0x00,
    eCurLQDevStat,
};

enum EnableOrDisableTrigger
{
    eDisableTrigger, // 禁止触发模式
    eEnableTrigger, // 使能触发模式
};

enum StartOrStopImageGrab
{
    eStopImageGrab,
    eStartImageGrab,
};

enum TransDataParamId
{
    eTDIImageData, // 传输 图像数据
    eTDIAlgoHandle, // 算法识别结果
    eTDSaveToDevice, // 不传输
    eTDSaveToLocal, // 存本地
};

enum CmdSubIdOfLQDevProcStat
{
    eEnableLQDevProcStat = 0x00,
    eCurProcStat,

    eSetStatisticsBaseVal,
};

enum PersistentOrDHCPIP
{
    eSetPersistentIP,
    eSetDHCPIP,
};

enum CmdSubIdOfDevStat
{
    eGetDeviceStat,         // 获取设备状态
    eSetDeviceIPAddr,        // 设置板子IP和掩码
    eSyncSystemTime,        // 同步系统时间
};

#endif // COMMAND_DEF_H
