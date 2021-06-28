
#ifndef ERROR_CODE_H
#define ERROR_CODE_H

enum ErrorCode
{
    eECOK = 0, // no error

    eECParam,
    eECMem,

    eECSocket,
    eECSetSocketFlag,
    eECBindSocket,
    eECListen,
    eECDataSendError,
    eECDataTransMethod,

    eECCmdHead,
    eECCmdId,
    eECCmdTail,

    eECSetCameraParam,
    eECCameraIsGrabbing,

    eECGetParam,
    eECSetParam,

    eECQueueFull,
};


#endif // ERROR_CODE_H
