
#include "DalsaLinearCamera.h"

#ifdef __unix__
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif


DalsaLinearCamera::DalsaLinearCamera() : Camera()
{
    memset(&(camera_ctrlr_param_.camera_connect_thread_id), 0, sizeof(camera_ctrlr_param_.camera_connect_thread_id));

    camera_ctrlr_param_.camera_ready = false;
	camera_ctrlr_param_.flag_bindBuf = false;
    camera_ctrlr_param_.grabbing_image = false;
    camera_ctrlr_param_.check_camera_connection = true;
	camera_ctrlr_param_.camera_n = 0;

	camera_ctrlr_param_.camera_sptr.convertBuffer = nullptr;
    camera_ctrlr_param_.camera_sptr.exit = true;
	camera_ctrlr_param_.camera_sptr.isRun = false;
	camera_ctrlr_param_.camera_sptr.m_latestBuffer = nullptr;
    camera_ctrlr_param_.camera_sptr.user_handler_ptr = nullptr;
    camera_ctrlr_param_.camera_sptr.usr_ptr = nullptr;
	camera_ctrlr_param_.usr_setting_fp = nullptr;
	camera_ctrlr_param_.m_blsLoadXMLFile = false;
	camera_ctrlr_param_.pause_img_grab = true;
	camera_ctrlr_param_.tri_flag = true;
	// camera_ctrlr_param_.bIsTriggerModeOn = false;
	// camera_ctrlr_param_.CameraMapRef = nullptr;

    initCameraConnection();
}

DalsaLinearCamera::~DalsaLinearCamera()
{
	// stopImageGrab();
    camera_ctrlr_param_.check_camera_connection = false;
	camera_ctrlr_param_.grabbing_image = false;
	camera_ctrlr_param_.m_blsLoadXMLFile = true;
	camera_ctrlr_param_.pause_img_grab = true;
	// camera_ctrlr_param_.CameraMapRef = nullptr;
    pthread_join(camera_ctrlr_param_.camera_connect_thread_id, nullptr);
	camera_ctrlr_param_.camera_sptr.exit = true;
	
	GevFreeTransfer(camera_ctrlr_param_.camera_sptr.handle);
	for(int i = 0; i < NUM_BUF; i++)
	{
#if USE_HUGEPAGE
		munmap(camera_ctrlr_param_.bufAddress[i], camera_ctrlr_param_.size);
#else
		free(camera_ctrlr_param_.bufAddress[i]);
#endif
		camera_ctrlr_param_.bufAddress[i] = nullptr;
	}
	camera_ctrlr_param_.flag_bindBuf = false;
	if(camera_ctrlr_param_.camera_sptr.convertBuffer != nullptr)
	{
		free(camera_ctrlr_param_.camera_sptr.convertBuffer);
		camera_ctrlr_param_.camera_sptr.convertBuffer = nullptr;
	}

	if(camera_ctrlr_param_.usr_setting_fp)
	{
		fclose(camera_ctrlr_param_.usr_setting_fp);
		camera_ctrlr_param_.usr_setting_fp = nullptr;
	}
    // close camera
    GevCloseCamera(&camera_ctrlr_param_.camera_sptr.handle);

	// Close down the API
	GevApiUninitialize();
	// Close socket API
	_CloseSocketAPI();
}

bool DalsaLinearCamera::initCameraConnection()
{
    int ret;

    ret = pthread_create(&(camera_ctrlr_param_.camera_connect_thread_id), nullptr, cameraReconnectProcess, &camera_ctrlr_param_);
    if (ret != 0)
    {
        printf("thread create failed ret = %d\n", ret);
        return false;
    }

    return true;
}

void DalsaLinearCamera::OutputFeatureValuePair(const char *feature_name, const char *value_string, FILE *fp)
{
	if((feature_name != nullptr) && value_string != nullptr)
	{
		// Feature : Value pair output (in one place in to ease changing formats or output method - if desired)
		fprintf(fp, "%s %s\n", feature_name, value_string);
	}
}

void DalsaLinearCamera::OutputFeatureValues(const CNodePtr &ptrFeature, FILE *fp)
{
	CCategoryPtr ptrCategory(ptrFeature);
	if(ptrCategory.IsValid())
	{
		GenApi::FeatureList_t Features;
		ptrCategory->GetFeatures(Features);
		for(GenApi::FeatureList_t::iterator itFeature=Features.begin(); itFeature!=Features.end();itFeature++)
		{
			OutputFeatureValues((*itFeature), fp);
		}
	}
	else
	{
        // Store only "streamable" features (since only they can be restored).
		if(ptrFeature->IsStreamable())
		{
			// Create a selector set (in case this feature is selected)
			bool selectorSettingWasOutput = false;
			CSelectorSet selectorSet(ptrFeature);

			// Loop through all the selectors that select this feature
			// Use the magical CSelectorSet class that handles the
			// "set of selectors that select this feature" and indexes
			// through all possible combinations so we can save all of them
			selectorSet.SetFirst();

			do
			{
				CValuePtr valNode(ptrFeature);
				if(valNode.IsValid() && (GenApi::RW == valNode->GetAccessMode()) && (ptrFeature->IsFeature()))
				{
					// Its a valid streamable feature
					// Get its selectors (if it has any)
					FeatureList_t selectorList;
					selectorSet.GetSelectorList(selectorList, true);

					for(FeatureList_t::iterator itSelector=selectorList.begin(); itSelector!=selectorList.end(); itSelector++)
					{
						// Output selector : selectorValue as a feature : value pair
						selectorSettingWasOutput = true;
						CNodePtr selectedNode(*itSelector);
						CValuePtr selectedValue(*itSelector);
						OutputFeatureValuePair(static_cast<const char *>(selectedNode->GetName()), static_cast<const char *>(selectedValue->ToString()), fp);
					}
					// Output feature : value pair for this selector combination 
					// It just outputs the feature : value pair if there are no selectors
					OutputFeatureValuePair(static_cast<const char *>(ptrFeature->GetName()), static_cast<const char *>(valNode->ToString()), fp);
				}
			} while(selectorSet.SetNext());

			// Reset to original selector/selected value (if any was used)
			selectorSet.Restore();

			// Save the original settings for any selector that was handled (looped over) above
			if(selectorSettingWasOutput)
			{
				FeatureList_t selectingFeatures;
				selectorSet.GetSelectorList(selectingFeatures, true);
				for(FeatureList_t::iterator itSelector = selectingFeatures.begin(); itSelector != selectingFeatures.end(); ++itSelector)
				{
					CNodePtr selectedNode( *itSelector);
					CValuePtr selectedValue( *itSelector);
					OutputFeatureValuePair(static_cast<const char *>(selectedNode->GetName()), static_cast<const char *>(selectedValue->ToString()), fp);
				}
			}
		}
	}
}

bool DalsaLinearCamera::saveUserSettings(DalsaLinearCameraCtrlrParam *camera_ctrlr_param_ptr)
{
#if SAVE_USR_CONFIG
	GEV_STATUS status = 0;
	GEV_CAMERA_HANDLE handle;
	FILE *fp = nullptr;

	handle = camera_ctrlr_param_ptr->camera_sptr.handle;
	fp = fopen(USER_CAMERA_CONFIG, "w");
	if(fp == nullptr)
	{
		// printf("Open %s failed, errno:%d.\n", USER_CAMERA_CONFIG, errno);
		printLog(CLog::FILE_AND_TERMINAL, CLog::ERROR, "Open camera configuration (XML) falied.");
		return false;
	}

	if(!camera_ctrlr_param_ptr->m_blsLoadXMLFile)
	{        
		char xmlFileName[MAX_PATH] = {0};
		// Get the XML file onto disk and use it to make the CNodeMap object
		// status = Gev_RetrieveXMLFile(handle, xmlFileName, sizeof(xmlFileName), false);
		status = GevGetGenICamXML_FileName(handle, sizeof(xmlFileName), xmlFileName);
		if(status != 0)
        {
            return false;
        }

        camera_ctrlr_param_ptr->CameraMapRef._LoadXMLFromFile(xmlFileName);
		// Connect the features in the node map to the camera handle
        status = GevConnectFeatures(handle, static_cast<void *>(&camera_ctrlr_param_ptr->CameraMapRef));
        if(status != 0 )
        {
			printf("Error %d connecting node map to handle\n", status);
            return false;
        }
		camera_ctrlr_param_ptr->m_blsLoadXMLFile = true;
	}
	
	// Put the camera in "streaming feature mode"
	CCommandPtr start = camera_ctrlr_param_ptr->CameraMapRef._GetNode("Std::DeviceFeaturePersistenceStart");
	if(start)
	{
		try
		{
			int done = false;
			int timeout = 5;
			start->Execute();

			while(!done && (timeout-- > 0))
			{
				Sleep(10);
				done = start->IsDone();
			}
		}
		// Catch all possible exceptions from a node access
		CATCH_GENAPI_ERROR(status);
	}

	// Traverse the node map and dump all the { feature value } pairs
	if(status == 0)
	{
		// Find the standard "Root" node and dump the feature
		GenApi::CNodePtr pRoot = camera_ctrlr_param_ptr->CameraMapRef._GetNode("Root");
		OutputFeatureValues(pRoot, fp);
	}

	// End the "streaming feature mode"
	CCommandPtr end = camera_ctrlr_param_ptr->CameraMapRef._GetNode("Std::DeviceFeaturePersistenceEnd");
	if(end)
	{
		try
		{
			int done = false;
			int timeout = 5;
			end->Execute();

			while(!done && (timeout-- > 0))
			{
				Sleep(10);
				done = end->IsDone();
			}
		}
		// Catch all possible exceptions from a node access
		CATCH_GENAPI_ERROR(status);
	}

	fclose(fp);

#endif
	return true;
}

void DalsaLinearCamera::ValidateFeatureValues(const CNodePtr &ptrFeature)
{
	CCategoryPtr ptrCategory(ptrFeature);
	if( ptrCategory.IsValid() )
	{
		GenApi::FeatureList_t Features;
		ptrCategory->GetFeatures(Features);
		for( GenApi::FeatureList_t::iterator itFeature=Features.begin(); itFeature!=Features.end(); itFeature++ )
		{    
			ValidateFeatureValues( (*itFeature) );
		}
	}
	else
	{
		// Issue a "Get" on the feature (with validate set to true).
		CValuePtr valNode(ptrFeature);  
		if ((GenApi::RW == valNode->GetAccessMode()) || (GenApi::RO == valNode->GetAccessMode()) )
		{
			int status = 0;
			try {
				valNode->ToString(true);
			}
			// Catch all possible exceptions from a node access.
			CATCH_GENAPI_ERROR(status);
			if (status != 0)
			{
				printf("load_features : Validation failed for feature %s\n", static_cast<const char *>(ptrFeature->GetName())); 
			}
		}
	}
}


bool DalsaLinearCamera::loadUserSettings(DalsaLinearCameraCtrlrParam *camera_ctrlr_param_ptr)
{
#if LOAD_USR_CONFIG
	GEV_STATUS status = -1;
	GEV_CAMERA_HANDLE handle;
	FILE *fp = nullptr;

	handle = camera_ctrlr_param_ptr->camera_sptr.handle;
	int error_count = 0, feature_count = 0;

	fp = fopen(USER_CAMERA_CONFIG, "r");
	if(fp == nullptr)
	{
		// printf("Error opening file %s : errno = %d\n", USER_CAMERA_CONFIG, errno);
		printLog(CLog::FILE_AND_TERMINAL, CLog::ERROR, "Open camera configuration (XML) falied.");
		return false;
	}

	if(!camera_ctrlr_param_ptr->m_blsLoadXMLFile)
	{        
		char xmlFileName[MAX_PATH] = {0};
		// Get the XML file onto disk and use it to make the CNodeMap object
		status = GevGetGenICamXML_FileName(handle, sizeof(xmlFileName), xmlFileName);
		if(status != 0 )
        {
            return false;
        }
		printf("XML stored as %s\n", xmlFileName);
        camera_ctrlr_param_ptr->CameraMapRef._LoadXMLFromFile(xmlFileName);
		// Connect the features in the node map to the camera handle
        status = GevConnectFeatures(handle, static_cast<void *>(&camera_ctrlr_param_ptr->CameraMapRef));
        if(status != 0 )
        {
			printf("Error %d connecting node map to handle\n", status);
            return false;
        }
		camera_ctrlr_param_ptr->m_blsLoadXMLFile = true;
	}
	// Put the camera in "streaming feature mode".
	CCommandPtr start = camera_ctrlr_param_ptr->CameraMapRef._GetNode("Std::DeviceRegistersStreamingStart");
	if (start)
	{
		try {
			int done = FALSE;
			int timeout = 5;
			start->Execute();
			while(!done && (timeout-- > 0))
			{
				Sleep(10);
				done = start->IsDone();
			}
		}
		// Catch all possible exceptions from a node access.
		CATCH_GENAPI_ERROR(status);
	}
	// Read the file as { feature value } pairs and write them to the camera.
	if (status == 0)
	{
		char feature_name[MAX_GEVSTRING_LENGTH + 1] = {0};
		char value_str[MAX_GEVSTRING_LENGTH + 1] = {0};

		while (2 == fscanf(fp, "%s %s", feature_name, value_str))
		{
			status = 0;
			// Find node and write the feature string (without validation).
			GenApi::CNodePtr pNode = camera_ctrlr_param_ptr->CameraMapRef._GetNode(feature_name);
			if (pNode)
			{
				CValuePtr valNode(pNode);  
				try {
					valNode->FromString(value_str, false);
				}
				// Catch all possible exceptions from a node access.
				CATCH_GENAPI_ERROR(status);
				if (status != 0)
				{
					error_count++;
					printf("Error restoring feature %s : with value %s\n", feature_name, value_str); 
				}
				else
				{
					feature_count++;
				}
			}
			else
			{
				error_count++;                                                                                      
				printf("Error restoring feature %s : with value %s\n", feature_name, value_str);
			}
		}
	}

	// End the "streaming feature mode".
	CCommandPtr end = camera_ctrlr_param_ptr->CameraMapRef._GetNode("Std::DeviceRegistersStreamingEnd");
	if ( end  )
	{
		try {
			int done = FALSE;
			int timeout = 5;
			end->Execute();
			while(!done && (timeout-- > 0))
			{
				Sleep(10);
				done = end->IsDone();
			}
		}
		// Catch all possible exceptions from a node access.
		CATCH_GENAPI_ERROR(status);
	}

	// Validate.
	if (status == 0)
	{
		// Iterate through all of the features calling "Get" with validation enabled.
		// Find the standard "Root" node and dump the features.
		GenApi::CNodePtr pRoot = camera_ctrlr_param_ptr->CameraMapRef._GetNode("Root");
		ValidateFeatureValues( pRoot );
	}

	if (error_count == 0)
	{
		printf("%d Features loaded successfully !\n", feature_count);
	}
	else
	{
		printf("%d Features loaded successfully : %d Features had errors\n", feature_count, error_count);           
	}

	fclose(fp);
#endif
	return true;
}

void DalsaLinearCamera::initApiLib()
{
	// Set default options for the library
    GEVLIB_CONFIG_OPTIONS options = {0};
    GevGetLibraryConfigOptions(&options);
    options.logLevel = GEV_LOG_LEVEL_NORMAL;
    GevSetLibraryConfigOptions(&options);
}

void DalsaLinearCamera::registerImageHandler(ImageHandler user_handler_ptr, void * usr_ptr)
{
	camera_ctrlr_param_.camera_sptr.user_handler_ptr = user_handler_ptr;
	camera_ctrlr_param_.camera_sptr.usr_ptr = usr_ptr;
}

void DalsaLinearCamera::startImageGrab()
{
	// printf("start to grab image.\n");
	printLog(CLog::FILE_AND_TERMINAL, CLog::INFO, "Start to grab image.");

	GEV_STATUS status = 0;
	GEV_CAMERA_HANDLE handle = camera_ctrlr_param_.camera_sptr.handle;

#if USE_GEV_DISPLAY_WINDOW
	unsigned int height = 0;
	unsigned int width = 0;
	unsigned int pixDepth = camera_ctrlr_param_.camera_sptr.depth;
	unsigned int pixFormat = camera_ctrlr_param_.camera_sptr.pixFormat;

	/* Create a window for display first*/
	/*
		param 1:title Window title
		param 2:visible true--visible  false--not visible
		param 3:window height, in pixels
		param 4:window width, in pixels
		param 5:Pixel depth, in bits
		param 6:use_shared_memory
	*/
	//=======================================================================
	// Get the GenICam FeatureNodeMap object and access the camera features
	// GenApi::CNodeMapRef *Camera = static_cast<GenApi::CNodeMapRef*>(GevGetFeatureNodeMap(handle));
	// if(Camera)
	{
		// Access some features using ths bare GenApi interface methods
		try
		{
			// Mandatory features...
			GenApi::CIntegerPtr ptrIntNode = camera_ctrlr_param_.CameraMapRef._GetNode("Width");
			width = (uint32_t)ptrIntNode->GetValue();
			ptrIntNode = camera_ctrlr_param_.CameraMapRef._GetNode("Height");
			height = (uint32_t)ptrIntNode->GetValue();
		}
		// Catch all possible exceptions from a node access
		CATCH_GENAPI_ERROR(status);
	}
	

	double t1, t2;

	X_VIEW_HANDLE View = NULL;
	t1 = GET_MS();
	View = CreateDisplayWindow("DALSA Camera Console", IS_VISIBLE, height, width, pixDepth, pixFormat, false);
	t2 = GET_MS();
	printf("time ====== %.2f ms\n", t2 - t1);
	camera_ctrlr_param_.camera_sptr.View = View;
#endif
    if(camera_ctrlr_param_.camera_sptr.exit == true)
	{
		camera_ctrlr_param_.camera_sptr.exit = false;
	}
        
	if(camera_ctrlr_param_.camera_sptr.isRun == false)
	{
		camera_ctrlr_param_.camera_sptr.isRun = true;
	}

	usleep(1000);	
	status = GevStartTransfer(handle, -1);
	if(status != 0)
	{
		printLog(CLog::FILE_AND_TERMINAL, CLog::ERROR, "Failed to transfer image.");
		camera_ctrlr_param_.camera_ready = false;
		return;
	}

	if(camera_ctrlr_param_.pause_img_grab == true)
	{
		camera_ctrlr_param_.pause_img_grab = false;
	}
	if(camera_ctrlr_param_.grabbing_image == false)
	{
		camera_ctrlr_param_.grabbing_image = true;
	}
}

void DalsaLinearCamera::startImageSnap(int index)
{
    if(camera_ctrlr_param_.camera_sptr.exit == true)
	{
		camera_ctrlr_param_.camera_sptr.exit = false;
	}
        
	if(camera_ctrlr_param_.camera_sptr.isRun == false)
	{
		camera_ctrlr_param_.camera_sptr.isRun = true;
	}

	GEV_STATUS status = GevStartTransfer(camera_ctrlr_param_.camera_sptr.handle, (uint32_t)index);	
	if(status != 0)
	{
		printLog(CLog::FILE_AND_TERMINAL, CLog::ERROR, "Image snap failed.");
		return;
	}

	if(camera_ctrlr_param_.pause_img_grab)
		camera_ctrlr_param_.pause_img_grab = false;
	if(!camera_ctrlr_param_.grabbing_image)
		camera_ctrlr_param_.grabbing_image = true;
	printf("start to snap %u frame image.\n", index);
}

void DalsaLinearCamera::abortImageGrab()
{
	GEV_STATUS status = GevAbortTransfer(camera_ctrlr_param_.camera_sptr.handle);
	if(status != 0)
	{
		printf("Failed to abort transfer image.\n");
		return;
	}
}

int DalsaLinearCamera::IsTurboDriveAvailable(GEV_CAMERA_HANDLE handle)
{
	int type;
	uint32_t val = 0;

	if(0 == GevGetFeatureValue(handle, "transferTurboCurrentlyAbailable", &type, sizeof(uint32_t), &val))
	{
		// Current / Standard method present - this feature indicates if TurboMode is available.
		// (Yes - it is spelled that odd way on purpose)
		return (val != 0);
	}
	else
	{
		// Legacy mode check - standard feature is not there try it manually.
		char pxlfmt_str[64] = {0};
		// Mandatory feature (always present)
		GevGetFeatureValueAsString(handle, "PixelFormat", &type, sizeof(pxlfmt_str), pxlfmt_str);

		// Set the "turbo" capability selector for this format
		if(0 != GevSetFeatureValueAsString(handle, "transferTurboCapabilitySelector", pxlfmt_str))
		{
			// Either the capability selector is not present or the pixel format is not part of 
			// the capability set.
			// Either way - TurboMOde is NOT AVAILABLE....

			return 0;
		}
		else
		{
			// The capability set exists so TurboMode is AVAILABLE
			// It is up to the camera to send TurboMode data if it can - so we let it.
			return 1;
		}
	}
	return 0;
}

void DalsaLinearCamera::toggleTurboMode()
{
	GEV_STATUS status = -1;
	int turboDriveAvailable = 0, type = 0;

	turboDriveAvailable = IsTurboDriveAvailable(camera_ctrlr_param_.camera_sptr.handle);
	if(turboDriveAvailable)
	{
		uint32_t val = 1;
		status = GevGetFeatureValue(camera_ctrlr_param_.camera_sptr.handle, "transferTurboMode", &type, sizeof(uint32_t), &val);
		if(status != 0)
		{
			printf("toggleTurboMode->GevGetFeatureValue error.\n");
			return;
		}
		val = (val == 0) ? 1 : 0;
		status = GevSetFeatureValue(camera_ctrlr_param_.camera_sptr.handle, "transferTurboMode", sizeof(UINT32), &val);
		if(status != 0)
		{
			printf("toggleTurboMode->GevSetFeatureValue error.\n");
			return;
		}
		status = GevGetFeatureValue(camera_ctrlr_param_.camera_sptr.handle, "transferTurboMode", &type, sizeof(UINT32), &val);
		if(status != 0)
		{
			printf("toggleTurboMode->GevGetFeatureValue error.\n");
			return;
		}

		if(val == 1)
		{
			printf("TurboMode enabled.\n");
		}
		else
		{
			printf("TurboMode disabled.\n");
		}
	}
	else
	{
		printf("*** TurboDrive is NOT Available for this device/pixel format combination ***");
	}
}

void DalsaLinearCamera::quitImageGrab()
{
	GEV_STATUS status = -1;
	int numBuffers = NUM_BUF;

	status = GevStopTransfer(camera_ctrlr_param_.camera_sptr.handle);
	status = GevAbortTransfer(camera_ctrlr_param_.camera_sptr.handle);
	status = GevFreeTransfer(camera_ctrlr_param_.camera_sptr.handle);
#if	USE_GEV_DISPLAY_WINDOW
	DestroyDisplayWindow(camera_ctrlr_param_.camera_sptr.View);
#endif
	for(int i = 0; i < numBuffers; i++)
	{
		memset(camera_ctrlr_param_.bufAddress[i], 0x00, camera_ctrlr_param_.size);
	}

	camera_ctrlr_param_.pause_img_grab = true;
}

void DalsaLinearCamera::jpegEncoderImage(unsigned int image_width, unsigned int image_height, char *outputFileName)
{
	struct timeval tv1, tv2;
	void *bufToEncode = camera_ctrlr_param_.camera_sptr.m_latestBuffer;
	char *output_file_name = new char[strlen(outputFileName) + 5];
	strcpy(output_file_name, outputFileName);
	strcat(output_file_name, ".jpg");

	JpegEncoder jpeg_encoder;
	if(!jpeg_encoder.initEncoder(image_width, image_height))
	{
		printf("Init JPEG encoder failed.\n");
		return;
	}

	unsigned long jpeg_enc_size;
	unsigned char *jpeg_data_ptr;

	gettimeofday(&tv1, NULL);
	jpeg_data_ptr = jpeg_encoder.jpegEncode((const unsigned char *)bufToEncode, jpeg_enc_size, output_file_name);
	gettimeofday(&tv2, NULL);

	int diff = tv2.tv_sec - tv1.tv_sec;
	if(diff > 0)
		printf("Time: %.2f s\n", (tv2.tv_usec - tv1.tv_usec + 1000000) / 1000.0f);
	else
		printf("Time: %.2f s\n", (tv2.tv_usec - tv1.tv_usec) / 1000.0f);

	if(jpeg_enc_size == 0 || bufToEncode == nullptr)
		printf("JPEG encoder failed.\n");
	else
		cout << "JPEG encode success, data pointer: 0x" << hex << (unsigned long long)bufToEncode << ", JPEG data length: " << dec << jpeg_enc_size << endl;
}

void DalsaLinearCamera::_GetUniqueFilename(char *filename, size_t size, char *basename)
{
	// Create a filename based on the current time (to 0.01 seconds)
	struct timeval tm;
	uint32_t years, days, hours, seconds;

	if((filename != NULL) && (basename != NULL))
	{
		if (size > (16 + sizeof(basename)))
		{

			// Get the time and turn it into a 10 msec resolution counter to use as an index.
			gettimeofday( &tm, NULL);
			years = ((tm.tv_sec / 86400) / 365);
			tm.tv_sec = tm.tv_sec - (years * 86400 * 365);
			days  = (tm.tv_sec / 86400);
			tm.tv_sec = tm.tv_sec - (days * 86400);
			hours = (tm.tv_sec / 3600);
			seconds = tm.tv_sec - (hours * 3600);                       

			snprintf(filename, size, "%s_%03d%02d%04d%02d", basename, days,hours, (int)seconds, (int)(tm.tv_usec/10000));
		}
	}
}

void DalsaLinearCamera::saveConvertedImage()
{
	GEV_STATUS status = 0;
	uint32_t macLow;
	char filename[128] = {0};
	uint32_t saveFormat = camera_ctrlr_param_.camera_sptr.format;
	void *bufToSave = camera_ctrlr_param_.camera_sptr.m_latestBuffer;
	int allocate_conversion_buffer = 0;
	uint32_t width = 0, height = 0;
	char uniqueName[128] = {0};

	//=======================================================================
	// Get the GenICam FeatureNodeMap object and access the camera features
	// GenApi::CNodeMapRef *Camera = static_cast<GenApi::CNodeMapRef *>(GevGetFeatureNodeMap(camera_ctrlr_param_.camera_sptr.handle));
	// if(Camera)
	{
		// Access some features using ths bare GenApi interface methods
		try
		{
			// Mandatory features...
			GenApi::CIntegerPtr ptrIntNode = camera_ctrlr_param_.CameraMapRef._GetNode("Width");
			width = (uint32_t)ptrIntNode->GetValue();
			ptrIntNode = camera_ctrlr_param_.CameraMapRef._GetNode("Height");
			height = (uint32_t)ptrIntNode->GetValue();
			printf("GenApi::CNodeMapRef width = %d height = %d\n", width, height);
		}
		CATCH_GENAPI_ERROR(status);
	}
	// Make sure we have data to save
	if(camera_ctrlr_param_.camera_sptr.m_latestBuffer != nullptr)
	{
		uint32_t component_cnt = 1;
		uint32_t convertedFmt = 0;

		convertedFmt = GevGetConvertedPixelType(0, saveFormat);
		
		if(GevIsPixelTypeBayer(convertedFmt) && ENABLE_BAYER_CONVERSION)
		{
			int img_size = 0;
			int img_depth = 0;
			uint8_t fill = 0;

			// Bayer will be converted to RGB
			saveFormat = GevGetBayerAsRGBPixelType(convertedFmt);

			// Convert the image to RGB
			img_depth = GevGetPixelDepthInBits(saveFormat);			// The number of bits taken up by a single color component in a pixel for the input raw image format.
			component_cnt = GevGetPixelComponentCount(saveFormat);	// The number of color components in a pixel for the input raw image format.
			img_size = width * height * component_cnt * ((img_depth + 7) / 8);
			bufToSave = malloc(img_size);
			if(bufToSave == nullptr)
				perror("malloc");
			fill = (component_cnt == 4) ? 0xff : 0;
			memset(bufToSave, fill, img_size);

			allocate_conversion_buffer = 1;

			// Convert the Bayer to RGB
//			ConvertBayerToRGB(0, height, width, convertedFmt, camera_ctrlr_param_.camera_sptr.m_latestBuffer, saveFormat, bufToSave);
		}
		else
		{
			saveFormat = convertedFmt;
			allocate_conversion_buffer = 0;
		}

		// Get the low part of the Mac address (use it as part of a unique file name for saving images)
		// Generate a unique base name to be used for saving image files
		// based on the last 3 octets of the Mac address.
		macLow = camera_ctrlr_param_.pCamera[camera_ctrlr_param_.camera_n - 1].macLow;
		macLow &= 0x00ffffff;
		snprintf(uniqueName, sizeof(uniqueName), "imag_%06x", macLow);

		// Generate a file name from the unique base name
		_GetUniqueFilename(filename, sizeof(filename) - 5, uniqueName);

		// Add the file extension we want.
		strncat(filename, ".tif", sizeof(filename));

#if defined(LIBTIFF_AVAILABLE)
		int ret = Write_GevImage_ToTIFF(filename, width, height, saveFormat, bufToSave);	// Writes the input image to the specified TIFF file.
		if(ret > 0)
		{
			printf("Image saved as:%s : %d bytes written\n", filename, ret);
		}
		else
			printf("Error %d saving image\n", ret);
#else
		printf("*** Library libtiff not installed ***\n");
#endif
	}
	else
	{
		printf("No image buffer has been acquired yet!\n");
	}
	if(allocate_conversion_buffer)
	{
		free(bufToSave);
	}
}

void DalsaLinearCamera::stopImageGrab()
{
    /* 停止采图 */
	printLog(CLog::FILE_AND_TERMINAL, CLog::INFO, "Stop image grab.");

	GevStopTransfer(camera_ctrlr_param_.camera_sptr.handle);
	GevAbortImageTransfer(camera_ctrlr_param_.camera_sptr.handle);
	/* 停止采集线程 */
	if(camera_ctrlr_param_.camera_sptr.isRun)
		camera_ctrlr_param_.camera_sptr.isRun = false;
#if USE_GEV_DISPLAY_WINDOW
	// if necessary
	if(camera_ctrlr_param_.camera_sptr.View != NULL)
	{
		DestroyDisplayWindow(camera_ctrlr_param_.camera_sptr.View);
		camera_ctrlr_param_.camera_sptr.View = NULL;
	}
#endif
	if(camera_ctrlr_param_.pause_img_grab == false)
		camera_ctrlr_param_.pause_img_grab = true;

	if(!camera_ctrlr_param_.tri_flag)
		camera_ctrlr_param_.tri_flag = true;

	if(camera_ctrlr_param_.grabbing_image)
		camera_ctrlr_param_.grabbing_image = false;
	return;
}

bool DalsaLinearCamera::setImageSize(unsigned int width, unsigned int height)   // modified by sj @ 2020.05.14
{
	if(camera_ctrlr_param_.grabbing_image)
	{
		printf("Setting camera resolution is forbidden under working condition.\n");
		return false;
	}

	GEV_STATUS status = -1;
	unsigned int width_max, width_min, height_max, height_min, _width, _height;

	GevGetParamThresholdInt(&camera_ctrlr_param_, "Width", &width_max, &width_min);
	GevGetParamThresholdInt(&camera_ctrlr_param_, "Height", &height_max, &height_min);

	if(width < width_min)
		_width = width_min;
	else if(width > width_max)
		_width = width_max;
	else
		_width = width;
		
	status = GevSetFeatureValue(camera_ctrlr_param_.camera_sptr.handle, "Width", sizeof(_width), &_width);
	if(status != 0)
	{
		printLog(CLog::FILE_AND_TERMINAL, CLog::ERROR, "Set width failed.");
		camera_ctrlr_param_.camera_ready = false;
		return false;
	}
	
	if(height < height_min)
		_height = height_min;
	else if(height > height_max)
		_height = height_max;
	else
		_height = height;
	
	status = GevSetFeatureValue(camera_ctrlr_param_.camera_sptr.handle, "Height", sizeof(_height), &_height);
	if(status != 0)
	{
		printLog(CLog::FILE_AND_TERMINAL, CLog::ERROR, "Set Height failed.");
		camera_ctrlr_param_.camera_ready = false;
		return false;
	}

	saveUserSettings(&camera_ctrlr_param_);
	
    return true;
}

bool DalsaLinearCamera::setPixelFormat(unsigned int pixel_format)   // modified by sj @ 2020.05.14
{
    GEV_STATUS status = -1;

    char pixel_format_str[64] = {0};
    switch (pixel_format)
    {
        case Mono8:
        {
			strcpy(pixel_format_str, "Mono8");
            break;
        }
		case Mono10Packed:
		{
			strcpy(pixel_format_str, "Mono10Packed");
			break;
		}
        case Mono12:
        {
			strcpy(pixel_format_str, "Mono12");
            break;
        }
        case Mono12Packed:
        {
			strcpy(pixel_format_str, "Mono12Packed");
            break;
        }
        default:
            return false;
    }

    status = GevSetFeatureValueAsString(camera_ctrlr_param_.camera_sptr.handle, "PixelFormat", pixel_format_str);
    if(status != 0)
    {
		printLog(CLog::FILE_AND_TERMINAL, CLog::ERROR, "Set pixel format failed.");
        camera_ctrlr_param_.camera_ready = false;
        return false;
    }

    saveUserSettings(&camera_ctrlr_param_);

    return true;
}

bool DalsaLinearCamera::setExposureTime(float exposure_time)   // modified by sj @ 2020.05.14
{
    GEV_STATUS status = 0;
	double exp_time_max, exp_time_min;
	float exposureTime;

	GevGetParamThresholdDouble(&camera_ctrlr_param_, "ExposureTime", &exp_time_max, &exp_time_min);

	if(exposure_time >= exp_time_min && exposure_time <= exp_time_max)
	{
		exposureTime = exposure_time;
	}
	else if(exposure_time < exp_time_min)
	{
		exposureTime = (float)exp_time_min;
	}
	else
	{
		exposureTime = (float)exp_time_max;
	}
	status = GevSetFeatureValue(camera_ctrlr_param_.camera_sptr.handle, "ExposureTime", sizeof(exposureTime), &exposureTime);
	if(status != 0)
	{
		printLog(CLog::FILE_AND_TERMINAL, CLog::ERROR, "Set ExposureTime failed.");
		camera_ctrlr_param_.camera_ready = false;
		return false;
	}

	printf("Set ExposureTime: %.2f ms\n", exposureTime);
	saveUserSettings(&camera_ctrlr_param_);
	
    return true;
}

bool DalsaLinearCamera::setGain(float gain) // modified by sj @ 2020.05.14
{
    GEV_STATUS status = 0;
	double gain_max, gain_min;
	float _gain;
	GevGetParamThresholdDouble(&camera_ctrlr_param_, "Gain", &gain_max, &gain_min);

	if(gain < gain_min)
		_gain = (float)gain_min;
	else if(gain > gain_max)
		_gain = (float)gain_max;
	else
		_gain = gain;

	if((status = GevSetFeatureValue(camera_ctrlr_param_.camera_sptr.handle, "Gain", sizeof(_gain), &_gain)) != 0)
	{
		printLog(CLog::FILE_AND_TERMINAL, CLog::ERROR, "Set Gain failed.");
		camera_ctrlr_param_.camera_ready = false;
		return false;
	}

	printf("Set Gain: %.2f\n", _gain);
	saveUserSettings(&camera_ctrlr_param_);
	
    return true;
}

bool DalsaLinearCamera::setImageOffset(unsigned int offset_x, unsigned int offset_y)    // modified by sj @ 2020.05.14
{
	if(camera_ctrlr_param_.grabbing_image)
	{
		printf("Setting offset is forbidden under working condition.\n");
		return false;
	}
    GEV_STATUS status = -1;
	unsigned int offset_x_max, offset_x_min, offset;

	GevGetParamThresholdInt(&camera_ctrlr_param_, "OffsetX", &offset_x_max, &offset_x_min);

    if(offset_y != 0)
    {
        printf("Offset in Y-axis of linear camera should be null, you can set it in any value but it takes no effect.\n");
    }

	if(offset_x < offset_x_min)
		offset = offset_x_min;
	else if(offset_x > offset_x_max)
		offset = offset_x_max;
	else
		offset = offset_x;
	
	if((status = GevSetFeatureValue(camera_ctrlr_param_.camera_sptr.handle, "OffsetX", sizeof(offset), &offset)) != 0)
	{
		printLog(CLog::FILE_AND_TERMINAL, CLog::ERROR, "Set OffsetX failed.");
		camera_ctrlr_param_.camera_ready = false;
		return false;
	}
	printf("Set offsetX : %d.\n", offset);

	saveUserSettings(&camera_ctrlr_param_);

    return true;
}

bool DalsaLinearCamera::setAcquisitionFrameRate(unsigned int frame_rate)        // modified by sj @ 2020.05.14
{
    GEV_STATUS status = -1;
	double line_rate_max = 0, line_rate_min = 0;
	float line_rate;

	GevGetParamThresholdDouble(&camera_ctrlr_param_, "AcquisitionLineRate", &line_rate_max, &line_rate_min);
	printf("line_max:%.2f   line_min:%.2f\n", line_rate_max, line_rate_min);

	if(frame_rate >= line_rate_min && frame_rate <= line_rate_max)
		line_rate = (float)frame_rate;
	else if(frame_rate < line_rate_min)
		line_rate = (float)line_rate_min;
	else
		line_rate = (float)line_rate_max;

	if((status = GevSetFeatureValue(camera_ctrlr_param_.camera_sptr.handle, "AcquisitionLineRate", sizeof(line_rate), &line_rate)) != 0)
	{
		printLog(CLog::FILE_AND_TERMINAL, CLog::ERROR, "Set AcquisitionLineRate failed.");
		camera_ctrlr_param_.camera_ready = false;
		return false;
	}

	saveUserSettings(&camera_ctrlr_param_);
	printf("FrameRate set:%.2f success.\n", line_rate);
	
    return true;
}

bool DalsaLinearCamera::enableTriggerMode(bool enable_trigger)      // modified by sj @ 2020.05.14
{
    GEV_STATUS status = -1;

    const char *enable_trigger_str;
    if (enable_trigger)
    {
        enable_trigger_str = "On";
    }
    else
    {
        enable_trigger_str = "Off";
    }

    if((status = GevSetFeatureValueAsString(camera_ctrlr_param_.camera_sptr.handle, "TriggerMode", enable_trigger_str)) != 0)
    {
		printLog(CLog::FILE_AND_TERMINAL, CLog::ERROR, "Set TriggerMode failed.");
       	camera_ctrlr_param_.camera_ready = false;
        return false;
    }
	printf("Set TriggerMode:%s\n", enable_trigger_str);
    saveUserSettings(&camera_ctrlr_param_);

    return true;
}

bool DalsaLinearCamera::setDefaultSettings()
{
	if(camera_ctrlr_param_.pause_img_grab == false)
	{
		printf("Set camera resolution is forbidden under working condition!\n");
		return false;
	}

	GEV_STATUS status = -1;
	int cmd = 6;
	status = GevSetFeatureValue(camera_ctrlr_param_.camera_sptr.handle, "UserSetLoad", sizeof(int), &cmd);
	if(status == 0)
	{
		printLog(CLog::LOGFILE, CLog::INFO, "Set TriggerMode success.");
	}
	else
	{
		printLog(CLog::FILE_AND_TERMINAL, CLog::ERROR, "Set TriggerMode failed.");
	}

	saveUserSettings(&camera_ctrlr_param_);
	return true;
}

bool DalsaLinearCamera::setTriggerSource(unsigned int trigger_source)       // modified by sj @ 2020.05.14
{
    GEV_STATUS status = -1;
    const char *trigger_source_str;
    /* 设置触发源（硬触发为Line1~Line4，软触发为Software） */
    switch (trigger_source)
    {
        case TS_LINE1:
        {
            trigger_source_str = "Line1";
            break;
        }
        case TS_LINE2:
        {
            trigger_source_str = "Line2";
            break;
        }
        case TS_LINE3:
        {
            trigger_source_str = "Line3";
            break;
        }
		case TS_LINE4:
		{
			trigger_source_str = "Line4";
			break;
		}
		case TS_ROTARY_ENCODER:
		{
			trigger_source_str = "rotaryEncoder1";
			break;
		}
        case TS_SOFT :
        {
			printf("TS_SOFT\n");
            trigger_source_str = "Software";
            break;
        }
        default:
            printf("Unknown trigger source.\n");
            return false;
    }

    if((status = GevSetFeatureValueAsString(camera_ctrlr_param_.camera_sptr.handle, "TriggerSource", trigger_source_str)) != 0)
    {
		printLog(CLog::FILE_AND_TERMINAL, CLog::ERROR, "Set TriggerSource failed.");
       	camera_ctrlr_param_.camera_ready = false;
        return false;
    }

	printf("Set TriggerSource:%s\n", trigger_source_str);
    saveUserSettings(&camera_ctrlr_param_);

    return true;
}

bool DalsaLinearCamera::setTriggerSelector(unsigned int trigger_selector)
{
	GEV_STATUS status = -1;

	const char *trigger_selector_str;
	switch(trigger_selector)
	{
		case LineStart:
			trigger_selector_str = "LineStart";
			break;
		case FrameStart:
			trigger_selector_str = "FrameStart";
			break;
		case FrameBurstStart:
			trigger_selector_str = "FrameBurstStart";
			break;
		case FrameActive:
			trigger_selector_str = "FrameActive";
			break;
		case FrameBurstActive:
			trigger_selector_str = "FrameBurstActive";
			break;
		default:
			printf("Unknown TriggerSelector.\n");
			break;
	}

	if((status = GevSetFeatureValueAsString(camera_ctrlr_param_.camera_sptr.handle, "TriggerSelector", trigger_selector_str)) != 0)
	{
		printLog(CLog::FILE_AND_TERMINAL, CLog::ERROR, "Set TriggerSelector failed.");
		camera_ctrlr_param_.camera_ready = false;
		return false;
	}

	printf("Set TriggerSelector:%s\n", trigger_selector_str);
	saveUserSettings(&camera_ctrlr_param_);
	return true;
}

bool DalsaLinearCamera::setTriggerActivation(unsigned int trigger_act)
{
	GEV_STATUS status = -1;

	const char *trigger_act_str;
	switch(trigger_act)
	{
		case TA_RISING_EDGE:
			trigger_act_str = "RisingEdge";
			break;
		case TA_FALLING_EDGE:
			trigger_act_str = "FallingEdge";
			break;
		case TA_ANY_EDGE:
			trigger_act_str = "AnyEdge";
			break;
		default:
			printf("Unknown TriggerActivation.\n");
			break;
	}

	if((status = GevSetFeatureValueAsString(camera_ctrlr_param_.camera_sptr.handle, "TriggerActivation", trigger_act_str)) != 0)
	{
		printLog(CLog::FILE_AND_TERMINAL, CLog::ERROR, "Set TriggerActivation failed.");
		camera_ctrlr_param_.camera_ready = false;
		return false;
	}

	printf("Set TriggerActivation:%s\n", trigger_act_str);
	saveUserSettings(&camera_ctrlr_param_);
	return true;
}

bool DalsaLinearCamera::setTriggerDelay(float trigger_delay)
{
	GEV_STATUS status = -1;

	printf("TriggerDelay set:%.2f.\n", trigger_delay);

	if((status = GevSetFeatureValue(camera_ctrlr_param_.camera_sptr.handle, "TriggerDelay", sizeof(float), &trigger_delay)) != 0)
	{
		printLog(CLog::FILE_AND_TERMINAL, CLog::ERROR, "Set TriggerDelay failed.");
		camera_ctrlr_param_.camera_ready = false;
		return false;
	}

	saveUserSettings(&camera_ctrlr_param_);
	printf("TriggerDelay set:%.2f success.\n", trigger_delay);
	
    return true;
}

bool DalsaLinearCamera::setTriggerDelaySrc(unsigned int trigger_delay_src)
{
	GEV_STATUS status = -1;

	const char *trigger_delay_src_str;
	switch(trigger_delay_src)
	{
		case TDS_Internal_Clock:
			trigger_delay_src_str = "InternalClock";
			break;
		case TDS_Line_Trigger_Signal:
			trigger_delay_src_str = "lineTriggerSignal";
			break;
		default:
			printf("Unknown triggerDelaySource.\n");
			break;
	}

	if((status = GevSetFeatureValueAsString(camera_ctrlr_param_.camera_sptr.handle, "triggerDelaySource", trigger_delay_src_str)) != 0)
	{
		printLog(CLog::FILE_AND_TERMINAL, CLog::ERROR, "Set triggerDelaySource failed.");
		camera_ctrlr_param_.camera_ready = false;
		return false;
	}

	printf("Set triggerDelaySource:%s\n", trigger_delay_src_str);
	saveUserSettings(&camera_ctrlr_param_);
	return true;
}

bool DalsaLinearCamera::setTriggerLineCnt(unsigned int trigger_line_cnt)
{
	GEV_STATUS status = -1;

	printf("TriggerLineCount set:%u.\n", trigger_line_cnt);

	if((status = GevSetFeatureValue(camera_ctrlr_param_.camera_sptr.handle, "triggerLineCount", sizeof(unsigned int), &trigger_line_cnt)) != 0)
	{
		printLog(CLog::FILE_AND_TERMINAL, CLog::ERROR, "Set triggerLineCount failed.");
		camera_ctrlr_param_.camera_ready = false;
		return false;
	}

	saveUserSettings(&camera_ctrlr_param_);
	printf("TriggerLineCount set:%u success.\n", trigger_line_cnt);
	
    return true;
}

bool DalsaLinearCamera::setTriggerFrameCnt(unsigned int trigger_frame_cnt)
{
	GEV_STATUS status = -1;

	printf("TriggerFrameCount set:%u.\n", trigger_frame_cnt);

	if((status = GevSetFeatureValue(camera_ctrlr_param_.camera_sptr.handle, "triggerFrameCount", sizeof(unsigned int), &trigger_frame_cnt)) != 0)
	{
		printLog(CLog::FILE_AND_TERMINAL, CLog::ERROR, "Set triggerFrameCount failed.");
		camera_ctrlr_param_.camera_ready = false;
		return false;
	}

	saveUserSettings(&camera_ctrlr_param_);
	printf("TriggerFrameCount set:%u success.\n", trigger_frame_cnt);
	
    return true;
}

bool DalsaLinearCamera::setLineSelector(unsigned int line_selector)
{
	GEV_STATUS status = -1;

	const char *line_selector_str;
	switch(line_selector)
	{
		case LS_LINE1:
			line_selector_str = "Line1";
			break;
		case LS_LINE2:
			line_selector_str = "Line2";
			break;
		case LS_LINE3:
			line_selector_str = "Line3";
			break;
		case LS_LINE4:
			line_selector_str = "Line4";
			break;
		default:
			printf("Unknown LineSelector.\n");
			break;
	}

	if((status = GevSetFeatureValueAsString(camera_ctrlr_param_.camera_sptr.handle, "LineSelector", line_selector_str)) != 0)
	{
		printLog(CLog::FILE_AND_TERMINAL, CLog::ERROR, "Set LineSelector failed.");
		camera_ctrlr_param_.camera_ready = false;
		return false;
	}

	printf("Set LineSelector:%s\n", line_selector_str);
	saveUserSettings(&camera_ctrlr_param_);
	return true;
}

bool DalsaLinearCamera::setLineFormat(unsigned int line_format)
{
	GEV_STATUS status = -1;

	const char *line_format_str;
	switch(line_format)
	{
		case LF_RS422:
			line_format_str = "RS422";
			break;
		case LS_LINE2:
			line_format_str = "SingleEnded";
			break;
		default:
			printf("Unknown LineFormat.\n");
			break;
	}

	if((status = GevSetFeatureValueAsString(camera_ctrlr_param_.camera_sptr.handle, "LineFormat", line_format_str)) != 0)
	{
		printLog(CLog::FILE_AND_TERMINAL, CLog::ERROR, "Set LineFormat failed.");
		camera_ctrlr_param_.camera_ready = false;
		return false;
	}

	printf("Set LineFormat:%s\n", line_format_str);
	saveUserSettings(&camera_ctrlr_param_);
	return true;
}

bool DalsaLinearCamera::setLineDetectionLevel(unsigned int input_lv)
{
	GEV_STATUS status = -1;

	const char *input_lv_str;
	switch(input_lv)
	{
		case Threshold_for_3V3:
			input_lv_str = "Threshold_for_3V3";
			break;
		case Threshold_for_5V:
			input_lv_str = "Threshold_for_5V";
			break;
		case Threshold_for_12V:
			input_lv_str = "Threshold_for_12V";
			break;
		case Threshold_for_24V:
			input_lv_str = "Threshold_for_24V";
			break;
		default:
			printf("Unknown LineDetectionLevel.\n");
			break;
	}

	if((status = GevSetFeatureValueAsString(camera_ctrlr_param_.camera_sptr.handle, "lineDetectionLevel", input_lv_str)) != 0)
	{
		printLog(CLog::FILE_AND_TERMINAL, CLog::ERROR, "Set lineDetectionLevel failed.");
		camera_ctrlr_param_.camera_ready = false;
		return false;
	}

	printf("Set lineDetectionLevel:%s\n", input_lv_str);
	saveUserSettings(&camera_ctrlr_param_);
	return true;
}

bool DalsaLinearCamera::setLineDebouncingPeriod(unsigned long long deb_period)
{
	GEV_STATUS status = -1;

	if((status = GevSetFeatureValue(camera_ctrlr_param_.camera_sptr.handle, "lineDebouncingPeriod", sizeof(unsigned long long), &deb_period)) != 0)
	{
		printLog(CLog::FILE_AND_TERMINAL, CLog::ERROR, "Set lineDebouncingPeriod failed.");
		camera_ctrlr_param_.camera_ready = false;
		return false;
	}

	saveUserSettings(&camera_ctrlr_param_);
	printf("LineDebouncingPeriod set:%llu success.\n", deb_period);
	
    return true;
}

bool DalsaLinearCamera::softTrigger()
{
	GEV_STATUS status = 0;
	
	/* 以下两种软触发均有效 */
#if 0
	/* GenICam标准提供的方式 */
	//=======================================================================
	// Put the camera in "Activation of software trigger"
	CCommandPtr ts_node = camera_ctrlr_param_.CameraMapRef._GetNode("TriggerSoftware");
	if(ts_node)
	{
		try {
			int done = false;
			int timeout = 5;
			ts_node->Execute();
			while(!done && (timeout-- > 0))
			{
				Sleep(10);
				done = ts_node->IsDone();
			}
		}
		// Catch all possible exceptions from a node access
		CATCH_GENAPI_ERROR(status);
	}
#else
	/* 相机提供的API */
	GEV_CAMERA_HANDLE handle = camera_ctrlr_param_.camera_sptr.handle;
	printf("TriggerSoftware\n");
	int cmd = 1;
    status = GevSetFeatureValue(handle, "TriggerSoftware", sizeof(UINT32), &cmd);
	if(status != 0)
		printLog(CLog::FILE_AND_TERMINAL, CLog::ERROR, "Set TriggerSoftware failed.");
#endif
	
    return true;
}

bool DalsaLinearCamera::setAcquisitionFrameRateEnable(bool bEnable) // modified by sj @ 2020.05.14
{
    // Nothing to do

    return true;
}

bool DalsaLinearCamera::isCameraReady()
{
    return camera_ctrlr_param_.camera_ready;
}

bool DalsaLinearCamera::waitCameraRead()
{
    int i = 0;
    while (!camera_ctrlr_param_.camera_ready && i < 10)
    {
		printf("camera_ready:%d\n", camera_ctrlr_param_.camera_ready);
        ++i;
        usleep(300 * 1000);
    }

    return isCameraReady();
}

bool DalsaLinearCamera::isCameraGrabbingImages()
{
    return camera_ctrlr_param_.grabbing_image;
}

bool DalsaLinearCamera::getImageSize(unsigned int *width_ptr, unsigned int *height_ptr)   // modified by sj @ 2020.05.14
{
	GEV_STATUS status = 0;

	//=======================================================================
	// Access some features using ths bare GenApi interface methods
	try
	{
		// Mandatory features...
		GenApi::CIntegerPtr ptrIntNode = camera_ctrlr_param_.CameraMapRef._GetNode("Width");
		*width_ptr = (uint32_t)ptrIntNode->GetValue();
		ptrIntNode = camera_ctrlr_param_.CameraMapRef._GetNode("Height");
		*height_ptr = (uint32_t)ptrIntNode->GetValue();
	}
	// Catch all possible exceptions from a node access
	CATCH_GENAPI_ERROR(status);

    return true;
}

void DalsaLinearCamera::getFrameSize(DalsaLinearCameraCtrlrParam *camera_ctrlr_param_ptr, unsigned int *frame_size, unsigned int *width_size, unsigned *height_size)
{
	GEV_STATUS status = 0;
	GEV_CAMERA_HANDLE handle;
	uint64_t payload_size, size;
	uint32_t maxHeight = 1600;
	uint32_t maxWidth = 2048;
	uint32_t maxDepth = 2;
	uint32_t width = 0, height = 0, format = 0;

	handle = camera_ctrlr_param_ptr->camera_sptr.handle;
	//=======================================================================
	// Access some features using ths bare GenApi interface methods
	try
	{
		// Mandatory features...
		GenApi::CIntegerPtr ptrIntNode = camera_ctrlr_param_ptr->CameraMapRef._GetNode("Width");
		width = (uint32_t)ptrIntNode->GetValue();
		ptrIntNode = camera_ctrlr_param_ptr->CameraMapRef._GetNode("Height");
		height = (uint32_t)ptrIntNode->GetValue();
		ptrIntNode = camera_ctrlr_param_ptr->CameraMapRef._GetNode("PayloadSize");
		payload_size = (uint64_t)ptrIntNode->GetValue();
		GenApi::CEnumerationPtr ptrEnumNode = camera_ctrlr_param_ptr->CameraMapRef._GetNode("PixelFormat");
		format = (uint32_t)ptrEnumNode->GetIntValue();
	}
	// Catch all possible exceptions from a node access
	CATCH_GENAPI_ERROR(status);

	//======================================================================
	// Set up a grab/transfer from this camera
	//
	maxHeight = height;
	maxWidth = width;
	maxDepth = GetPixelSizeInBytes(format);

	// Allocate image buffers
	// (Either the image size or the payload_size, whichever is larger -allows for packed pixle formats)
	//
	size = maxDepth * maxWidth * maxHeight;
	size = (payload_size > size) ? payload_size : size;

	*width_size = width;
	*height_size = height;
	*frame_size = size;
}

//获取参数阈值
void DalsaLinearCamera::GevGetParamThresholdInt(DalsaLinearCameraCtrlrParam *camera_ctrlr_param_ptr, const char *key, unsigned int *max, unsigned int *min)
{
	GEV_STATUS status = 0;

	try
	{
		if(strcmp(key, "Width") == 0 || strcmp(key, "Height") == 0 || strcmp(key, "OffsetX") == 0)
		{
			GenApi::CIntegerPtr ptrIntNode = camera_ctrlr_param_ptr->CameraMapRef._GetNode(key);
			*max = (uint32_t)ptrIntNode->GetMax();
			*min = (uint32_t)ptrIntNode->GetMin();
			// printf("max value:%u min value:%u\n", *max, *min);
			// GenApi::CFloatPtr ptrFloatNode = camera_ctrlr_param_ptr->CameraMapRef._GetNode("ExposureTime");
			// printf("exptime=%.2f\n", (float)ptrFloatNode->GetValue());
		}
		else
		{
			printf("Unknown key\n");
		}
	}
	// Catch all possible exceptions from a node access
	CATCH_GENAPI_ERROR(status);
}

//获取参数阈值
void DalsaLinearCamera::GevGetParamThresholdDouble(DalsaLinearCameraCtrlrParam *camera_ctrlr_param_ptr, const char *key, double *max, double *min)
{
	GEV_STATUS status = 0;

	try
	{
		if(strcmp(key, "Gain") == 0 || strcmp(key, "ExposureTime") == 0 || strcmp(key, "AcquisitionLineRate") == 0)
		{
			GenApi::CFloatPtr ptrFloatNode = camera_ctrlr_param_ptr->CameraMapRef._GetNode(key);
			*max = ptrFloatNode->GetMax();
			*min = ptrFloatNode->GetMin();
			// printf("max value:%.2f min value:%.2f\n", *max, *min);
			// ptrFloatNode = camera_ctrlr_param_ptr->CameraMapRef._GetNode("ExposureTime");
			// printf("exptime=%.2f\n", (float)ptrFloatNode->GetValue());
		}
		else
		{
			printf("Unknown key\n");
		}
	}
	// Catch all possible exceptions from a node access
	CATCH_GENAPI_ERROR(status);
}

bool DalsaLinearCamera::getImageOffset(unsigned int *offset_x_ptr, unsigned int *offset_y_ptr)    // modified by sj @ 2020.05.14
{
	GEV_STATUS status = 0;

	//=======================================================================
	// Access some features using ths bare GenApi interface methods
	try
	{
		// Mandatory features...
		GenApi::CIntegerPtr ptrIntNode = camera_ctrlr_param_.CameraMapRef._GetNode("OffsetX");
		*offset_x_ptr = (uint32_t)ptrIntNode->GetValue();
	}
	// Catch all possible exceptions from a node access
	CATCH_GENAPI_ERROR(status);

    return true;
}

bool DalsaLinearCamera::getTriggerSource(unsigned int * trigger_src_ptr)    // modified by sj @ 2020.05.14
{
    GEV_STATUS status = -1;
    int type;

    uint32_t val;
    if((status = GevGetFeatureValue(camera_ctrlr_param_.camera_sptr.handle, "LineSelector", &type, sizeof(val), &val)) != 0)
    {
		printLog(CLog::FILE_AND_TERMINAL, CLog::ERROR, "Get LineSelector failed.");
        camera_ctrlr_param_.camera_ready = false;
        return false;
    }

    const char *trigger_source_str;
    enum TriggerSource tgr_src = (enum TriggerSource)val;
    switch (tgr_src)
    {
        case TS_LINE1:
            trigger_source_str = "Line1";
            *trigger_src_ptr = TS_LINE1;
            break;
        case TS_LINE2:
            trigger_source_str = "Line2";
            *trigger_src_ptr = TS_LINE2;
            break;
        case TS_LINE3:
            trigger_source_str = "Line3";
            *trigger_src_ptr = TS_LINE3;
            break;
        case TS_LINE4:
            trigger_source_str = "Line4";
            *trigger_src_ptr = TS_LINE4;
            break;
        case TS_SOFT:
            trigger_source_str = "Software";
            *trigger_src_ptr = TS_SOFT;
            break;
        default:
            *trigger_src_ptr = TS_Unknown;
            break;
    }

    return true;
}

bool DalsaLinearCamera::getTriggerMode(unsigned int *trigger_mode_ptr)
{
    GEV_STATUS status = -1;
    int type;
    uint32_t val;
    
    if((status = GevGetFeatureValue(camera_ctrlr_param_.camera_sptr.handle, "TriggerMode", &type, sizeof(val), &val)) != 0)
    {
		printLog(CLog::FILE_AND_TERMINAL, CLog::ERROR, "Get TriggerMode failed.");
        camera_ctrlr_param_.camera_ready = false;
        return false;
    }

    const char *trigger_mode_str;
    enum TriggerMode tgr_mod = (enum TriggerMode)val;
    switch(tgr_mod)
    {
        case TM_ON:
            trigger_mode_str = "On";
             *trigger_mode_ptr = TM_ON;
            break;
        case TM_OFF:
            trigger_mode_str = "Off";
            *trigger_mode_ptr = TM_OFF;
            break;
        default:
            *trigger_mode_ptr = TM_Unknown;
            break;
    }

    return true;
}

bool DalsaLinearCamera::getCameraTemperature(float *temp_ptr)
{
	GEV_STATUS status = 0;

#if 0
	int type;
	char temp_ptr_t[16] = {0};
	if((status = GevGetFeatureValueAsString(camera_ctrlr_param_.camera_sptr.handle, "DeviceTemperature", &type, sizeof(temp_ptr_t), temp_ptr_t)) != 0)
	{
		printLog(CLog::FILE_AND_TERMINAL, CLog::ERROR, "Get DeviceTemperature failed.");
       	// camera_ctrlr_param_.camera_ready = false;
        return false;
	}

	*temp_ptr = atof(temp_ptr_t);
#else
	//=======================================================================
	// Access some features using ths bare GenApi interface methods
	try
	{
		// Mandatory features...
		GenApi::CFloatPtr ptrFloatNode = camera_ctrlr_param_.CameraMapRef._GetNode("DeviceTemperature");
		*temp_ptr = ptrFloatNode->GetValue();
	}
	// Catch all possible exceptions from a node access
	CATCH_GENAPI_ERROR(status);
#endif
    return true;
}

bool DalsaLinearCamera::getAcquisitionFrameRate(unsigned int *frame_rate)
{
	GEV_STATUS status = 0;
	//=======================================================================
	// Access some features using ths bare GenApi interface methods
	try
	{
		// Mandatory features...
		GenApi::CFloatPtr ptrFloatNode = camera_ctrlr_param_.CameraMapRef._GetNode("AcquisitionLineRate");
		*frame_rate = (unsigned int)ptrFloatNode->GetValue();
	}
	// Catch all possible exceptions from a node access
	CATCH_GENAPI_ERROR(status);

    return true;
}

bool DalsaLinearCamera::getExposureTime(float *exposure_time)
{
	GEV_STATUS status = 0;
	//=======================================================================
	// Access some features using ths bare GenApi interface methods
	try
	{
		// Mandatory features...
		GenApi::CFloatPtr ptrFloatNode = camera_ctrlr_param_.CameraMapRef._GetNode("ExposureTime");
		*exposure_time = ptrFloatNode->GetValue();
	}
	// Catch all possible exceptions from a node access
	CATCH_GENAPI_ERROR(status);

    return true;
}

bool DalsaLinearCamera::getGain(float *gain)
{
	GEV_STATUS status = 0;
	//=======================================================================
	// Access some features using ths bare GenApi interface methods
	try
	{
		// Mandatory features...
		GenApi::CFloatPtr ptrFloatNode = camera_ctrlr_param_.CameraMapRef._GetNode("Gain");
		*gain = ptrFloatNode->GetValue();
	}
	// Catch all possible exceptions from a node access
	CATCH_GENAPI_ERROR(status);

    return true;
}

void DalsaLinearCamera::printCameraIP(int index)
{
    if(index > camera_ctrlr_param_.camera_n - 1)
    {
        printf("Error index, index value should be in the range of 0 to camera number - 1.\n");
        return;
    }
    uint32_t ipAddr = camera_ctrlr_param_.pCamera[index].ipAddr;
    uint32_t macHigh = camera_ctrlr_param_.pCamera[index].macHigh;
    uint32_t macLow = camera_ctrlr_param_.pCamera[index].macLow;

    printf("\n");
    printf("%d camera IP address is: %d.%d.%d.%d\n", index + 1, (ipAddr & 0xff000000) >> 24, (ipAddr & 0x00ff0000) >> 16, 
                (ipAddr & 0x0000ff00) >> 8, ipAddr & 0x000000ff);
    printf("Mac address is:%02X:%02X:%02X:%02X:%02X:%02X\n", (macHigh & 0x0000ff00) >> 8, macHigh & 0x000000ff,
                (macLow & 0xff000000) >> 24, (macLow & 0x00ff0000) >> 16, (macLow & 0x0000ff00) >> 8, (macLow & 0x000000ff));
    printf("\n");

    return;
}

bool DalsaLinearCamera::forceSetCameraIP(unsigned int *ipAddr, unsigned int *sub_mask)
{
    GEV_STATUS status = -1;

    unsigned int val;
    bool dhcp_t = true;
	char str_ipcfg[] = "GevCurrentIPConfiguration";
	char str_ipaddr[] = "GevCurrentIPAddress";
	char str_submask[] = "GevCurrentSubnetMask";

    if((status = GevReadRegisterByName(camera_ctrlr_param_.camera_sptr.handle, str_ipcfg, 0, sizeof(val), &val)) != 0)
    {
        printf("Read camera IP register failed.\n");
        return false;
    }

    if((val & IPCONFIG_MODE_PERSISTENT) != 0)
    {
        dhcp_t = false;
    }
    else if((val & IPCONFIG_MODE_DHCP) != 0)
    {
        dhcp_t = true;
    }
    else if((val & IPCONFIG_MODE_LLA) != 0)
    {
        dhcp_t = true;
    }

    if(dhcp_t)
    {
        val &= 0x00;
        val |= IPCONFIG_MODE_PERSISTENT;
    }

    status = GevWriteRegisterByName(camera_ctrlr_param_.camera_sptr.handle, str_ipcfg, 0, sizeof(val), &val);
    if(status != 0)
    {
        printf("Set camera IP in static mode failed.\n");
        return false;
    }

    // Set the current IP address and subnet mask
    if(ipAddr != nullptr)
    {   
        status = GevWriteRegisterByName(camera_ctrlr_param_.camera_sptr.handle, str_ipaddr, 0, sizeof(uint32_t), ipAddr);
        if(status != 0)
        {
            printf("Set IP address failed.\n");
            return false;
        }
    }
    if(sub_mask != nullptr)
    {
        status = GevWriteRegisterByName(camera_ctrlr_param_.camera_sptr.handle, str_submask, 0, sizeof(uint32_t), (void *)sub_mask);
        if(status != 0)
        {
            printf("Set subnet mask failed.\n");
            return false;
        }
    }
	return true;
}

bool DalsaLinearCamera::autoSetCameraIp()
{
    GEV_STATUS status = -1;

    uint32_t val, hostIP, index;
    bool dhcp_t = true;
	char str_ipcfg[] = "GevCurrentIPConfiguration";

	char g_ListStr[10][64] = {0};
	index = camera_ctrlr_param_.camera_n - 1;
	hostIP = camera_ctrlr_param_.pCamera[index].host.ipAddr;
	if(hostIP == 0)
	{
		sprintf(g_ListStr[index], "---.---.---.---");
	}
	else
	{
		sprintf(g_ListStr[index], "%3d.%3d.%3d.%3d", (hostIP & 0xff000000) >> 24,
				(hostIP & 0x00ff0000) >> 16, (hostIP & 0x0000ff00) >> 8, hostIP & 0x000000ff);
	}

	printf("Host IP addr is: %s\n", g_ListStr[index]);

    if((status = GevReadRegisterByName(camera_ctrlr_param_.camera_sptr.handle, str_ipcfg, 0, sizeof(val), &val)) != 0)
    {
        printf("Read camera IP register failed.\n");
        return false;
    }

    if((val & IPCONFIG_MODE_PERSISTENT) != 0)
    {
        dhcp_t = false;
    }
    else if((val & IPCONFIG_MODE_DHCP) != 0)
    {
        dhcp_t = true;
    }
    else if((val & IPCONFIG_MODE_LLA) != 0)
    {
        dhcp_t = true;
    }

    if(!dhcp_t)
    {
        val &= ~IPCONFIG_MODE_PERSISTENT;
        val |= IPCONFIG_MODE_DHCP;
    }

    status = GevWriteRegisterByName(camera_ctrlr_param_.camera_sptr.handle, str_ipcfg, 0, sizeof(val), &val);
    if(status != 0)
    {
        printf("Set camera IP in DHCP mode failed.\n");
        return false;
    }

    return true;
}

void *DalsaLinearCamera::ImageGrabThread(void *context)
{
	ImageInfo image_info = {0};
	CONTEXT *dispContext = (CONTEXT *)context;
	uint32_t timeout = 1000;	// Timeout period (in msec) to wait for the next frame.
	
	// FILE *fp = fopen("/home/tye/Dalsa_code/LQ/app_lq_20201105_dbg/test.txt", "w+");

	set_cpu(4);
	if(dispContext != nullptr)
	{
		while(!dispContext->exit)
		{
			if(dispContext->isRun)
			{
				GEV_BUFFER_OBJECT *img = nullptr;
				GEV_STATUS status = -1;

				// Wait for images to be received
				status = GevWaitForNextImage(dispContext->handle, &img, timeout);	// Waits for the next image object to be acquired and returns its pointer.
				if((img != nullptr)  && (status == GEVLIB_OK))
				{
					if(img->status == 0)
					{
						dispContext->m_latestBuffer = img->address;
						// 获取图像数据参数
						{							
							image_info.image_width = dispContext->nWidth = img->w;
							image_info.image_height = dispContext->nHeight = img->h;
							if(img->id % 0xffff == 0)
							{
								image_info.multiple_num++;
							}
							image_info.frame_num = image_info.multiple_num * 0xffff + img->id;	// 帧号

							// printf("img->frame_n :%lld   img->w:%d  img->h:%d img->addr:%x\n", image_info.frame_num, img->w, img->h, img->address);
							dispContext->user_handler_ptr((unsigned char *)img->address, (img->w)*(img->h), &image_info, nullptr);
						}
#if USE_GEV_DISPLAY_WINDOW
						// Can the acquired buffer be displayed?
						// IsGevPixelTypeX11Displayable returns true/false (1/0) if the input GigE Vision pixel type is displayable 
						// by the X11 Utility function provided with the example programs.
						//
						if(IsGevPixelTypeX11Displayable(img->format) || dispContext->convertFormat)
						{
							// Convert the image format if required
							if(dispContext->convertFormat)
							{
								int gev_depth = GevGetPixelDepthInBits(img->format);

								// Creates an X11 display window.
								// Convert the image to a displayable format
								// (Note : Not all formats can be displayed properly at this time (planar, YUV*, 10/12 bit packed))
								ConvertGevImageToX11Format(img->w, img->h, gev_depth, img->format, img->address, \
										dispContext->depth, dispContext->format, dispContext->convertBuffer);

								// Display the image in the (supported) converted format
								Display_Image(dispContext->View, dispContext->depth, img->w, img->h, dispContext->convertBuffer);
							}
							else
							{
								// Display the image in the (supported) received format
								Display_Image(dispContext->View, img->d, img->w, img->h, img->address);
							}
						}
						else
						{
							// Not displayable
							printf("Not displayable.\n");
						}
#endif
					}
					else
					{
						// Image had an error (incomplete (timeout/overflow/lost))
						// Do any handling of this condition necessary
						printLog(CLog::FILE_AND_TERMINAL, CLog::ERROR, "Image had an error.");
					}
				}				

#if USE_SYNCHRONOUS_BUFFER_CYCLING
				if(img != nullptr)
				{
					// Release the buffer back to the image transfer process for re-use
					GevReleaseImage(dispContext->handle, &img);
				}
#endif
			}		
			else
			{
				usleep(1000);
			}
		}
	}
	printf("ImageGrabThread QUIT.\n");
	pthread_detach(pthread_self());
	return NULL;
}

bool DalsaLinearCamera::initCamera(DalsaLinearCameraCtrlrParam *camera_ctrlr_param_ptr) 
{
    GEV_STATUS status = -1;
	GEV_CAMERA_HANDLE handle;

	// 初始化GenApi，在关闭相机时关闭GenApi
	status = GevApiInitialize();
	if(status != GEVLIB_OK)
	{
		printLog(CLog::LOGFILE, CLog::ERROR, "Init Api failed.");
		return false;
	}
	// read_profile_string("setting", "UserSettingFilePath", usrSettingFile, sizeof(usrSettingFile), "", FILE_INI_NAME);

	if (access(USER_CAMERA_CONFIG, F_OK) != 0)
	{
		camera_ctrlr_param_ptr->usr_setting_fp = fopen(USER_CAMERA_CONFIG, "a");
		if(camera_ctrlr_param_ptr->usr_setting_fp == nullptr)
		{
			printLog(CLog::FILE_AND_TERMINAL, CLog::ERROR, "Open user setting file failed.");
			return false;
		}
		fclose(camera_ctrlr_param_ptr->usr_setting_fp);
	}

	initApiLib();

    /* Discover Cameras */
    // Get device number
    status = GevGetCameraList(camera_ctrlr_param_ptr->pCamera, MAX_CAMERAS, &camera_ctrlr_param_ptr->camera_n);
    if(status != GEVLIB_OK)
    {
		printLog(CLog::FILE_AND_TERMINAL, CLog::ERROR, "Open user setting file failed.");
        return false;
    }

    if(camera_ctrlr_param_ptr->camera_n == 0)
    {
		printLog(CLog::FILE_AND_TERMINAL, CLog::ERROR, "No device found in the system.");
        return false;
    }

	int i = 0, numBuffers = NUM_BUF;
	uint64_t payload_size, size;
	uint32_t maxHeight = 1600;
	uint32_t maxWidth = 2048;
	uint32_t maxDepth = 2;
	// uint32_t pixFormat = 0, pixDepth = 0, convertedGevFormat = 0;
	uint32_t width = 2048, height = 1024, format = 0, offset = 0;
	double gain = 1.00, lineFreq = 10000.00, expTime = 50.00;
    /* GigE相机时，连接前设置相机Ip与网卡处于同一网段上 */
    // TODO

    /* 连接相机 */
	status = GevOpenCamera(&camera_ctrlr_param_ptr->pCamera[camera_ctrlr_param_ptr->camera_n - 1], GevExclusiveMode, &handle);
	if(status != 0)
	{
		printLog(CLog::FILE_AND_TERMINAL, CLog::ERROR, "Open camera failed.");
        return false;
	}

    camera_ctrlr_param_ptr->camera_sptr.handle = handle;
    camera_ctrlr_param_ptr->camera_ready = true;

	loadUserSettings(camera_ctrlr_param_ptr);

    printf("Camera is ready\n");

	/* GenICam feature access via Camera XML file enabled by "Open" */
	// Get the name of XML file name back
	//
	// char xmlFileName[MAX_PATH] = {0};
	// status = GevGetGenICamXML_FileName(handle, sizeof(xmlFileName), xmlFileName);
	// if(status == GEVLIB_OK)
	// 	printf("XML stored as %s\n", xmlFileName);
	// else
	// 	printf("XML file generated failed.\n");
	// status = GEVLIB_OK;

	/* Go on to adjust some API related setting (for tuning / diagnostics / etc...) */
	if(status == 0)
	{
		GEV_CAMERA_OPTIONS camOptions = {0};
		// Adjust the camera interface options if desired (see the manual)
		//
		GevGetCameraInterfaceOptions(handle, &camOptions);
		camOptions.heartbeat_timeout_ms = 5000;	// Disconnect detection 5 seconds

#if TUNE_STREAMING_THREADS
		// Some tuning can be done here.
		camOptions.streamFrame_timeout_ms = 864000;	// Internal timeout for frame reception
		camOptions.streamNumFramesBuffered = 4;	// Buffer frames internally
		camOptions.streamMemoryLimitMax = 64*1024*1024;	// Adjust packet memory buffering limit
		camOptions.streamPktSize = 9180;		// Adjust the GVSP packet size
		camOptions.streamPktDelay = 10;		// Add usecs between packets to pace arrive at NIC

		// Assign specific CPUs to threads (affinity) - if required for better performance
		{
			int numCpus = _GetNumCpus();
			if(numCpus > 1)
			{
				camOptions.streamThreadAffinity = numCpus - 1;
				camOptions.serverThreadAffinity = numCpus - 2;
			}
		}
#endif

		// Write the adjusted interface options back
		GevSetCameraInterfaceOptions(handle, &camOptions);

		//=======================================================================
		// Get the GenICam FeatureNodeMap object and access the camera features
		// GenApi::CNodeMapRef *Camera = static_cast<GenApi::CNodeMapRef*>(GevGetFeatureNodeMap(handle));
		// camera_ctrlr_param_ptr->CameraMapRef = static_cast<GenApi::CNodeMapRef*>(GevGetFeatureNodeMap(handle));
			// Access some features using ths bare GenApi interface methods
		try
		{
			// Mandatory features...
			// update setting.ini
			GenApi::CIntegerPtr ptrIntNode = camera_ctrlr_param_ptr->CameraMapRef._GetNode("Width");
			width = (uint32_t)ptrIntNode->GetValue();
			write_profile_int("setting", "WidthVal", width, FILE_INI_NAME);
			ptrIntNode = camera_ctrlr_param_ptr->CameraMapRef._GetNode("Height");
			height = (uint32_t)ptrIntNode->GetValue();
			write_profile_int("setting", "HeightVal", height, FILE_INI_NAME);
			ptrIntNode = camera_ctrlr_param_ptr->CameraMapRef._GetNode("OffsetX");
			offset = (uint32_t)ptrIntNode->GetValue();
			write_profile_int("setting", "Offset_x", offset, FILE_INI_NAME);
			ptrIntNode = camera_ctrlr_param_ptr->CameraMapRef._GetNode("PayloadSize");
			payload_size = (uint64_t)ptrIntNode->GetValue();
			GenApi::CFloatPtr ptrFloatNode = camera_ctrlr_param_ptr->CameraMapRef._GetNode("Gain");
			gain = ptrFloatNode->GetValue();
			write_profile_double("setting", "GainVal", gain, FILE_INI_NAME);
			ptrFloatNode = camera_ctrlr_param_ptr->CameraMapRef._GetNode("AcquisitionLineRate");
			lineFreq = ptrFloatNode->GetValue();
			write_profile_double("setting", "LineFreq", lineFreq, FILE_INI_NAME);
			ptrFloatNode = camera_ctrlr_param_ptr->CameraMapRef._GetNode("ExposureTime");
			expTime = ptrFloatNode->GetValue();
			write_profile_double("setting", "ExpTime", expTime, FILE_INI_NAME);
			GenApi::CEnumerationPtr ptrEnumNode = camera_ctrlr_param_ptr->CameraMapRef._GetNode("PixelFormat");
			format = (uint32_t)ptrEnumNode->GetIntValue();
		}
		// Catch all possible exceptions from a node access
		CATCH_GENAPI_ERROR(status);

		if(status == 0)
		{
			//======================================================================
			// Set up a grab/transfer from this camera
			//
			printf("Camera RIO set for \n\tHeight = %d\n\tWidth = %d\n\tPixelFormat (val) = 0x%08x\n", height, width, format);
			maxHeight = height;
			maxWidth = width;
			maxDepth = GetPixelSizeInBytes(format);
			// Allocate image buffers
			// (Either the image size or the payload_size, whichever is larger -allows for packed pixle formats)
			//
			size = maxDepth * maxWidth * maxHeight;
			size = (payload_size > size) ? payload_size : size;

#if USE_HUGEPAGE
		//hugepages
		size = (size % 0x200000) ? ((size/0x200000 + 1) * 0x200000) : ((size/0x200000) * 0x200000);	// 最小单位2M
		for(i = 0; i < numBuffers; i++)
		{
			camera_ctrlr_param_ptr->bufAddress[i] = (PUINT8)mmap(NULL, size, PROT_READ | PROT_WRITE, 
								MAP_SHARED | MAP_ANONYMOUS | MAP_LOCKED | 0x40000, -1, 0);
			if(MAP_FAILED == camera_ctrlr_param_ptr->bufAddress[i])
			{
				perror("mmap");
			}
		}
#else
		for(i = 0; i < numBuffers; i++)
		{
			camera_ctrlr_param_ptr->bufAddress[i] = (PUINT8)malloc(size);
			if(camera_ctrlr_param_ptr->bufAddress[i] == nullptr)
				printf("[%d] Buffer address malloc failed.\n", i);

			memset(camera_ctrlr_param_ptr->bufAddress[i], 0x00, size);
		}
#endif

#if USE_SYNCHRONOUS_BUFFER_CYCLING
			// Initialize a transfer with synchronous buffer handling
			status = GevInitializeTransfer(handle, SynchronousNextEmpty, size, numBuffers, camera_ctrlr_param_ptr->bufAddress);
			if(status != 0)
				printf("Call GevInitializeTransfer function error.\n");
#else
			// Initialize a transfer with asynchronous buffer handling
			status = GevInitializeTransfer(handle, Asynchronous, size, numBuffers, camera_ctrlr_param_ptr->bufAddress);
			if(status == GEVLIB_OK)
				camera_ctrlr_param_ptr->flag_bindBuf = true;
			else
				printf("Call GevInitializeTransfer function error.\n");
#endif
			// Create an image display window
			// This works best for monochrome and RGB. The packed color formats (with Y, U, V, etc...) require
			// conversion as do, if desired, Bayer formats
			// (Packed pixels are unpacked internally unless passthru mode is enabled)
			// Translate the raw pixel format to one suitable for the (limited) Linux display routines
			//
#if USE_GEV_DISPLAY_WINDOW
			status = GetX11DisplayablePixelFormat(ENABLE_BAYER_CONVERSION, format, &convertedGevFormat, &pixFormat);
			if(format != convertedGevFormat)
			{
				// We MAY need to convert the data on the fly to display it
				if(GevIsPixelTypeRGB(convertedGevFormat))
				{
					// Conversion to RGB888 required
					pixDepth = 32;	// Assume 4 8bit components for color display (RGBA)
					camera_ctrlr_param_ptr->camera_sptr.format = Convert_SaperaFormat_To_X11(pixFormat);
					camera_ctrlr_param_ptr->camera_sptr.pixFormat = pixFormat;
					camera_ctrlr_param_ptr->camera_sptr.depth = pixDepth;
					camera_ctrlr_param_ptr->camera_sptr.convertBuffer = malloc((maxWidth * maxHeight * ((pixDepth + 7) / 8)));
					camera_ctrlr_param_ptr->camera_sptr.convertFormat = true;
				}
				else
				{
					// Converted format is MONO - generally this is handled
					// internally (unpacking etc...) unless in passthru mode
					//
					pixDepth = GevGetPixelDepthInBits(convertedGevFormat);
					camera_ctrlr_param_ptr->camera_sptr.format = Convert_SaperaFormat_To_X11(pixFormat);
					camera_ctrlr_param_ptr->camera_sptr.pixFormat = pixFormat;
					camera_ctrlr_param_ptr->camera_sptr.depth = pixDepth;
					camera_ctrlr_param_ptr->camera_sptr.convertBuffer = nullptr;
					camera_ctrlr_param_ptr->camera_sptr.convertFormat = false;
				}
			}
			else
			{
				pixDepth = GevGetPixelDepthInBits(convertedGevFormat);
				camera_ctrlr_param_ptr->camera_sptr.format = Convert_SaperaFormat_To_X11(pixFormat);
				camera_ctrlr_param_ptr->camera_sptr.pixFormat = pixFormat;
				camera_ctrlr_param_ptr->camera_sptr.depth = pixDepth;
				camera_ctrlr_param_ptr->camera_sptr.convertBuffer = nullptr;
				camera_ctrlr_param_ptr->camera_sptr.convertFormat = false;
			}
#endif
			/* Create a window for display */
			/*
				param 1:title Window title
				param 2:visible true--visible  false--not visible
				param 3:window height, in pixels
				param 4:window width, in pixels
				param 5:Pixel depth, in bits
				param 6:use_shared_memory
			*/
			// View = CreateDisplayWindow("DALSA Camera Console", true, height, width, pixDepth, pixFormat, false);
			// camera_ctrlr_param_ptr->camera_sptr.View = View;
			camera_ctrlr_param_ptr->camera_sptr.exit = false;
			camera_ctrlr_param_ptr->camera_sptr.isRun = false;
			camera_ctrlr_param_ptr->size = size;

			if((status = GevSetFeatureValueAsString(camera_ctrlr_param_ptr->camera_sptr.handle, "DeviceTemperatureSelector", "FPGA_Board")) != 0)
			{
					printLog(CLog::LOGFILE, CLog::ERROR, "Set DeviceTemperatureSelector failed.");
//					camera_ctrlr_param_ptr->camera_ready = false;
					return false;
			}
			// Create a thread to receive images from the API and display them
			pthread_create(&camera_ctrlr_param_ptr->camera_grab_thread_id, NULL, ImageGrabThread, &camera_ctrlr_param_ptr->camera_sptr);
		}
		else
		{
			printf("Error : 0x%0x : opening camera\n", status);
		}
	}

    return true;
}

void *DalsaLinearCamera::cameraReconnectProcess(void *user_ptr)
{
	set_cpu(5);
	DalsaLinearCameraCtrlrParam *camera_ctrlr_param_ptr = (DalsaLinearCameraCtrlrParam *)(user_ptr);

	if(!initCamera(camera_ctrlr_param_ptr))
	{
		printLog(CLog::LOGFILE, CLog::ERROR, "Camera is not ready.");
	}
	while (1)
	{
		if(camera_ctrlr_param_ptr->check_camera_connection)
		{
			// 相机是否准备好以及是否被初始化
			if (!camera_ctrlr_param_ptr->camera_ready)
			{
				printLog(CLog::FILE_AND_TERMINAL, CLog::ERROR, "Camera is not ready.");
				// 退出采集线程
				camera_ctrlr_param_ptr->grabbing_image = false;
				camera_ctrlr_param_ptr->m_blsLoadXMLFile = false;
				camera_ctrlr_param_ptr->pause_img_grab = true;
				// camera_ctrlr_param_.CameraMapRef = nullptr;
				camera_ctrlr_param_ptr->camera_sptr.isRun = false;
                camera_ctrlr_param_ptr->camera_sptr.exit = true;

				// 释放巨页地址
				if(camera_ctrlr_param_ptr->flag_bindBuf)
				{
					GEV_STATUS status = GevFreeTransfer(camera_ctrlr_param_ptr->camera_sptr.handle);
					if(status != GEVLIB_OK)
					{
						printf("Free the streaming transfer to the list of buffers indicated failed.\n");
						continue;
					}
					for(int i = 0; i < NUM_BUF; i++)
					{
						if(camera_ctrlr_param_ptr->bufAddress[i])
						{
							munmap(camera_ctrlr_param_ptr->bufAddress[i], camera_ctrlr_param_ptr->size);
							camera_ctrlr_param_ptr->bufAddress[i] = nullptr;
						}
					}
					camera_ctrlr_param_ptr->flag_bindBuf = false;
				}
				
				if(camera_ctrlr_param_ptr->camera_sptr.convertBuffer != nullptr)
				{
					free(camera_ctrlr_param_ptr->camera_sptr.convertBuffer);
					camera_ctrlr_param_ptr->camera_sptr.convertBuffer = nullptr;
				}

				if(camera_ctrlr_param_ptr->usr_setting_fp)
				{
					fclose(camera_ctrlr_param_ptr->usr_setting_fp);
					camera_ctrlr_param_ptr->usr_setting_fp = nullptr;
				}
				// close camera
				GevCloseCamera(&camera_ctrlr_param_ptr->camera_sptr.handle);

				// Close down the API
				GevApiUninitialize();
				// Close socket API
				_CloseSocketAPI();

                sleep(1);
				if(!initCamera(camera_ctrlr_param_ptr))
				{
					printf("Camera is not ready.\n");
				}
				continue;
			}
			else
			{
				usleep(1000);
			}
		}
		else
			usleep(1000);
    }

    camera_ctrlr_param_ptr->camera_ready = false;

    return 0;
}

// 根据设置的分辨率，重新分配缓冲大小
bool DalsaLinearCamera::reinitBuffer()
{	
	printLog(CLog::LOGFILE, CLog::INFO, "reinitbuffer.");
	GEV_CAMERA_HANDLE handle;
	GEV_STATUS status = 0;
	int i = 0, numBuffers = NUM_BUF;
	uint64_t payload_size, size;
	uint32_t maxHeight = 1600;
	uint32_t maxWidth = 2048;
	uint32_t maxDepth = 2;
	uint32_t width = 0, height = 0, format = 0;
	handle = camera_ctrlr_param_.camera_sptr.handle;

	// Frees a streaming transfer to the list of buffers indicated.
	GevFreeTransfer(handle);	// 一定要执行该步骤，将原先分配的buff从句柄中清除掉，否则下次再将buff绑定到句柄中时会出错
	usleep(2000);
	for(i = 0; i < numBuffers; i++)
	{
		if(camera_ctrlr_param_.bufAddress[i])
		{
#if USE_HUGEPAGE			
			munmap(camera_ctrlr_param_.bufAddress[i], camera_ctrlr_param_.size);
#else
			free(camera_ctrlr_param_.bufAddress[i]);
#endif
			camera_ctrlr_param_.bufAddress[i] = nullptr;
		}
	}

	camera_ctrlr_param_.flag_bindBuf = false;

	// Access some features using ths bare GenApi interface methods
	try
	{
		// Mandatory features...
		GenApi::CIntegerPtr ptrIntNode = camera_ctrlr_param_.CameraMapRef._GetNode("Width");
		width = (uint32_t)ptrIntNode->GetValue();
		ptrIntNode = camera_ctrlr_param_.CameraMapRef._GetNode("Height");
		height = (uint32_t)ptrIntNode->GetValue();
		ptrIntNode = camera_ctrlr_param_.CameraMapRef._GetNode("PayloadSize");
		payload_size = (uint64_t)ptrIntNode->GetValue();
		GenApi::CEnumerationPtr ptrEnumNode = camera_ctrlr_param_.CameraMapRef._GetNode("PixelFormat");
		format = (uint32_t)ptrEnumNode->GetIntValue();
	}
	// Catch all possible exceptions from a node access
	CATCH_GENAPI_ERROR(status);

	if(status == 0)
	{
		//======================================================================
		// Set up a grab/transfer from this camera
		//
		// printf("Camera RIO reset for \n\tHeight = %d\n\tWidth = %d\n\tPixelFormat (val) = 0x%08x\n", height, width, format);
		maxHeight = height;
		maxWidth = width;
		maxDepth = GetPixelSizeInBytes(format);
		// Allocate image buffers
		// (Either the image size or the payload_size, whichever is larger -allows for packed pixle formats)
		//
		size = maxDepth * maxWidth * maxHeight;
		size = (payload_size > size) ? payload_size : size;

#if USE_HUGEPAGE
		//hugepages
		size = (size % 0x200000) ? ((size/0x200000 + 1) * 0x200000) : ((size/0x200000) * 0x200000);	// 最小单位2M
		for(i = 0; i < numBuffers; i++)
		{
			camera_ctrlr_param_.bufAddress[i] = (PUINT8)mmap(NULL, size, PROT_READ | PROT_WRITE, 
								MAP_SHARED | MAP_ANONYMOUS | MAP_LOCKED | 0x40000, -1, 0);
			if(MAP_FAILED == camera_ctrlr_param_.bufAddress[i])
			{
				printLog(CLog::FILE_AND_TERMINAL, CLog::ERROR, "mmap error.");
			}
		}
#else
		for(i = 0; i < numBuffers; i++)
		{
			camera_ctrlr_param_.bufAddress[i] = (PUINT8)malloc(size);
			if(camera_ctrlr_param_.bufAddress[i] == nullptr)
				printf("[%d] Buffer address malloc failed.\n", i);

			memset(camera_ctrlr_param_.bufAddress[i], 0x00, size);
		}
#endif
	}
#if USE_SYNCHRONOUS_BUFFER_CYCLING
	// Initialize a transfer with synchronous buffer handling
	status = GevInitializeTransfer(handle, SynchronousNextEmpty, size, numBuffers, camera_ctrlr_param_.bufAddress);
	if(status != 0)
		printf("Call GevInitializeTransfer function error.\n");
#else
	// Initialize a transfer with asynchronous buffer handling
	// usleep(1000);
	status = GevInitializeTransfer(handle, Asynchronous, size, numBuffers, camera_ctrlr_param_.bufAddress);
	if(status == GEVLIB_OK)
	{
		camera_ctrlr_param_.flag_bindBuf = true;
	}

#endif
	camera_ctrlr_param_.size = size;
	printf("reinitBuffer size:%lu\n", size);
	return true;
}

