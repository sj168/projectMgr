
//#include "haikangcamera.h"
//#include "dahuacamera.h"
#include "DalsaLinearCamera.h"

#include "CameraCtrlr.h"

CameraCtrlr::CameraCtrlr(unsigned int camera_type)
{
    camera_ = nullptr;
    camera_type_ = camera_type;
    switch (camera_type)
    {
        case CT_HAI_KANG:
        {
 //           camera_ = new HaiKangCamera();
            break;
        }

        case CT_DA_HUA:
        {
//            camera_ = new DaHuaCamera();
            break;
        }

        case CT_DALSA_LINEAR:
        {
            camera_ = new DalsaLinearCamera();
            break;
        }

        default:
            return;
    }
}

CameraCtrlr::~CameraCtrlr()
{
    if (camera_ != nullptr)
    {
        delete camera_;
        camera_ = nullptr;
    }
}

void CameraCtrlr::registerImageHandler(ImageHandler user_handler_ptr, void *usr_ptr)
{
    if (nullptr != camera_)
        camera_->registerImageHandler(user_handler_ptr, usr_ptr);
}

void CameraCtrlr::startImageGrab()
{
    if (nullptr == camera_)
        return;
    
    if (!camera_->waitCameraRead())
    {
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
        return;
    }

    return camera_->startImageGrab();
}

void CameraCtrlr::abortImageGrab()
{
	if(nullptr == camera_)
		return;

	if(!camera_->waitCameraRead())
	{
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
		return;
	}

	return camera_->abortImageGrab();
}

void CameraCtrlr::saveConvertedImage() 
{
	if(nullptr == camera_)
		return;

	if(!camera_->waitCameraRead())
	{
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
		return;
	}

	return camera_->saveConvertedImage();
}

void CameraCtrlr::startImageSnap(int index)
{
	if(nullptr == camera_)
		return;

	if(!camera_->waitCameraRead())
	{
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
		return;
	}

	return camera_->startImageSnap(index);
}

void CameraCtrlr::printCameraIP(int index)
{
	if(nullptr == camera_)
		return;

	if(!camera_->waitCameraRead())
	{
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
		return;
	}

	return camera_->printCameraIP(index);
}

bool CameraCtrlr::forceSetCameraIP(unsigned int *ip_addr, unsigned int *sub_mask)
{
	if(nullptr ==  camera_)
		return false;

	if(!camera_->waitCameraRead())
	{
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
		return false;
	}

	return camera_->forceSetCameraIP(ip_addr, sub_mask);
}

bool CameraCtrlr::autoSetCameraIp()
{
    if(nullptr == camera_)
		return false;

	if(!camera_->waitCameraRead())
	{
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
		return false;
	}

	return camera_->autoSetCameraIp();
}

// bool CameraCtrlr::forceSetDeviceIP(char *nic_name, unsigned int *ip_addr, unsigned int *sub_mask)
// {
//     if(nullptr == camera_)
// 		return false;

// 	if(!camera_->waitCameraRead())
// 	{
// 		printf("Wait camera ready timeout\n");
// 		return false;
// 	}

// 	return camera_->forceSetDeviceIP(nic_name, ip_addr, sub_mask);
// }

// bool CameraCtrlr::autoSetDeviceIp()
// {
//     if(nullptr == camera_)
// 		return false;

// 	if(!camera_->waitCameraRead())
// 	{
// 		printf("Wait camera ready timeout\n");
// 		return false;
// 	}

// 	return camera_->autoSetDeviceIp();
// }

void CameraCtrlr::stopImageGrab()
{
    if (nullptr == camera_)
        return ;

    if (!camera_->waitCameraRead())
    {
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
        return ;
    }

    return camera_->stopImageGrab();
}

void CameraCtrlr::toggleTurboMode()
{
    if (nullptr == camera_)
        return;

    if (!camera_->waitCameraRead())
    {
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
        return;
    }

    return camera_->toggleTurboMode();
}

void CameraCtrlr::quitImageGrab()
{	
    if (nullptr == camera_)
        return;

    if (!camera_->waitCameraRead())
    {
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
        return;
    }

    return camera_->quitImageGrab();
}

void CameraCtrlr::jpegEncoderImage(unsigned int image_width, unsigned int image_height, char *outputFileName)
{
    if (nullptr == camera_)
        return;

    if (!camera_->waitCameraRead())
    {
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
        return;
    }

    return camera_->jpegEncoderImage(image_width, image_height, outputFileName);
}

bool CameraCtrlr::setImageSize(unsigned int width, unsigned int height)
{
    if (nullptr == camera_)
        return false;

    if (!camera_->waitCameraRead())
    {
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
        return false;
    }

    return camera_->setImageSize(width, height);
}

bool CameraCtrlr::setPixelFormat(unsigned int pixel_format)
{
    if (nullptr == camera_)
        return false;

    if (!camera_->waitCameraRead())
    {
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
        return false;
    }

    return camera_->setPixelFormat(pixel_format);
}

bool CameraCtrlr::setExposureTime(float exposure_time)
{
    if (nullptr == camera_)
        return false;

    if (!camera_->waitCameraRead())
    {
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
        return false;
    }

    return camera_->setExposureTime(exposure_time);
}

bool CameraCtrlr::setGain(float gain)
{
    if (nullptr == camera_)
        return false;

    if (!camera_->waitCameraRead())
    {
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
        return false;
    }

    return camera_->setGain(gain);
}

bool CameraCtrlr::setImageOffset(unsigned int offset_x, unsigned int offset_y)
{
    if (nullptr == camera_)
        return false;

    if (!camera_->waitCameraRead())
    {
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
        return false;
    }

    return camera_->setImageOffset(offset_x, offset_y);
}

bool CameraCtrlr::setAcquisitionFrameRate(unsigned int frame_rate)
{
    if (nullptr == camera_)
        return false;

    if (!camera_->waitCameraRead())
    {
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
        return false;
    }

    return camera_->setAcquisitionFrameRate(frame_rate);
}

bool CameraCtrlr::enableTriggerMode(bool enable_trigger)
{
    if (nullptr == camera_)
        return false;

    if (!camera_->waitCameraRead())
    {
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
        return false;
    }

    return camera_->enableTriggerMode(enable_trigger);
}

bool CameraCtrlr::setDefaultSettings()
{
    if (nullptr == camera_)
        return false;

    if (!camera_->waitCameraRead())
    {
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
        return false;
    }

    return camera_->setDefaultSettings();
}

bool CameraCtrlr::setTriggerSource(unsigned int trigger_source)
{
    if (nullptr == camera_)
        return false;

    if (!camera_->waitCameraRead())
    {
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
        return false;
    }

    return camera_->setTriggerSource(trigger_source);
}

bool CameraCtrlr::setTriggerActivation(unsigned int trigger_act)
{
    if (nullptr == camera_)
		return false;

	if(!camera_->waitCameraRead())
	{
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
		return false;
	}

	return camera_->setTriggerActivation(trigger_act);
}

bool CameraCtrlr::setTriggerDelay(float trigger_delay)
{
    if (nullptr == camera_)
		return false;

	if(!camera_->waitCameraRead())
	{
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
		return false;
	}

	return camera_->setTriggerDelay(trigger_delay);
}

bool CameraCtrlr::setTriggerDelaySrc(unsigned int trigger_delay_src)
{
    if (nullptr == camera_)
		return false;

	if(!camera_->waitCameraRead())
	{
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
		return false;
	}

	return camera_->setTriggerDelaySrc(trigger_delay_src);
}

bool CameraCtrlr::setTriggerLineCnt(unsigned int trigger_line_cnt)
{
    if (nullptr == camera_)
		return false;

	if(!camera_->waitCameraRead())
	{
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
		return false;
	}

	return camera_->setTriggerLineCnt(trigger_line_cnt);
}

bool CameraCtrlr::setTriggerFrameCnt(unsigned int trigger_frame_cnt)
{
    if (nullptr == camera_)
		return false;

	if(!camera_->waitCameraRead())
	{
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
		return false;
	}

	return camera_->setTriggerFrameCnt(trigger_frame_cnt);
}

bool CameraCtrlr::setLineSelector(unsigned int line_selector)
{
    if (nullptr == camera_)
		return false;

	if(!camera_->waitCameraRead())
	{
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
		return false;
	}

	return camera_->setLineSelector(line_selector);
}

bool CameraCtrlr::setLineFormat(unsigned int line_format)
{
    if (nullptr == camera_)
		return false;

	if(!camera_->waitCameraRead())
	{
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
		return false;
	}

	return camera_->setLineFormat(line_format);
}

bool CameraCtrlr::setLineDetectionLevel(unsigned int input_lv)
{
    if (nullptr == camera_)
		return false;

	if(!camera_->waitCameraRead())
	{
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
		return false;
	}

	return camera_->setLineDetectionLevel(input_lv);
}

bool CameraCtrlr::setLineDebouncingPeriod(unsigned long long deb_period)
{
    if (nullptr == camera_)
		return false;

	if(!camera_->waitCameraRead())
	{
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
		return false;
	}

	return camera_->setLineDebouncingPeriod(deb_period);
}

bool CameraCtrlr::setTriggerSelector(unsigned int trigger_selector)
{
    if (nullptr == camera_)
		return false;

	if(!camera_->waitCameraRead())
	{
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
		return false;
	}

	return camera_->setTriggerSelector(trigger_selector);
}

bool CameraCtrlr::softTrigger()
{
    if (nullptr == camera_)
        return false;

    if (!camera_->waitCameraRead())
    {
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
        return false;
    }

    return camera_->softTrigger();
}

bool CameraCtrlr::setAcquisitionFrameRateEnable(bool bEnable)
{
    if (nullptr == camera_)
        return false;

    if (!camera_->waitCameraRead())
    {
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
        return false;
    }

    return camera_->setAcquisitionFrameRateEnable(bEnable);
}

bool CameraCtrlr::isCameraReady()
{
    if (nullptr == camera_)
        return false;

    if (!camera_->waitCameraRead())
    {
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
        return false;
    }

    return camera_->isCameraReady();
}

bool CameraCtrlr::waitCameraRead()
{
    if (nullptr != camera_)
        return camera_->waitCameraRead();

    return false;
}

bool CameraCtrlr::isCameraGrabbingImages()
{
    if (nullptr == camera_)
        return false;

    if (!camera_->waitCameraRead())
    {
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
        return false;
    }

    return camera_->isCameraGrabbingImages();
}

bool CameraCtrlr::getImageSize(unsigned int * width_ptr, unsigned int * height_ptr)
{
    if (nullptr == camera_)
        return false;

    if (!camera_->waitCameraRead())
    {
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
        return false;
    }

    return camera_->getImageSize(width_ptr, height_ptr);
}

#if 0
bool CameraCtrlr::getFrameSize(unsigned int *frame_size)
{
    if (nullptr == camera_)
        return false;

    if (!camera_->waitCameraRead())
    {
        printf("Wait camera ready timeout\n");
        return false;
    }

    return camera_->getFrameSize(frame_size);
}
#endif

bool CameraCtrlr::getImageOffset(unsigned int *offset_x_ptr, unsigned int *offset_y_ptr)
{
    if (nullptr == camera_)
        return false;

    if (!camera_->waitCameraRead())
    {
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
        return false;
    }

    return camera_->getImageOffset(offset_x_ptr, offset_y_ptr);
}

bool CameraCtrlr::getTriggerSource(unsigned int * trigger_src_ptr)
{
    if (nullptr == camera_)
        return false;

    if (!camera_->waitCameraRead())
    {
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
        return false;
    }

    return camera_->getTriggerSource(trigger_src_ptr);
}

bool CameraCtrlr::getTriggerMode(unsigned int * trigger_mode_ptr)
{
    if (nullptr == camera_)
        return false;

    if (!camera_->waitCameraRead())
    {
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
        return false;
    }

    return camera_->getTriggerMode(trigger_mode_ptr);
}

bool CameraCtrlr::getCameraTemperature(float *temp_ptr)
{
    if (nullptr == camera_)
        return false;

    if (!camera_->waitCameraRead())
    {
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
        return false;
    }

    return camera_->getCameraTemperature(temp_ptr);
}

bool CameraCtrlr::getAcquisitionFrameRate(unsigned int *frame_rate)
{
    if (nullptr == camera_)
        return false;

    if (!camera_->waitCameraRead())
    {
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
        return false;
    }

    return camera_->getAcquisitionFrameRate(frame_rate);
}

bool CameraCtrlr::getExposureTime(float *exposure_time)
{
    if (nullptr == camera_)
        return false;

    if (!camera_->waitCameraRead())
    {
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
        return false;
    }

    return camera_->getExposureTime(exposure_time);
}

bool CameraCtrlr::getGain(float *gain)
{
    if (nullptr == camera_)
        return false;

    if (!camera_->waitCameraRead())
    {
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
        return false;
    }

    return camera_->getGain(gain);
}

bool CameraCtrlr::reinitBuffer()
{
    if (nullptr == camera_)
        return false;

    if (!camera_->waitCameraRead())
    {
        printLog(CLog::FILE_AND_TERMINAL, CLog::WARNING, "Wait camera ready timeout.");
        return false;
    }

    return camera_->reinitBuffer();
}
