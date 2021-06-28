
CROSS_COMPILE = 
CXX = $(CROSS_COMPILE)g++ $(CFLAGS)
GCC=gcc
IROOT=/usr/dalsa/GigeV
ALGLIBBIN=/home/tye/alg_lib/EmbeddedLQ/Em_lq/install/0.1.0/Release/bin
ALGLIBINC=/home/tye/alg_lib/EmbeddedLQ/Em_lq/install/0.1.0/Release/include

# Get the configured include defs file (required for direct GenApi access)
# (It gets installed to the distribution tree).
ifeq ($(shell if test -e archdefs.mk; then echo exists; fi), exists)
	include archdefs.mk
else
# Force an error
$(error archdefs.mk file not found. It gets configured on installation ***)
endif

ifeq ($(DBG), 1)
	DEBUGFLAGS += -g
endif

HOST_ARCH=$(shell uname -m | sed -e 's/aarch64/aarch64/')

ifeq ($(HOST_ARCH), x86_64)
	CPP_FLAGS += -m64
	SDK_LIB_SUBDIR=m64x86
else ifeq ($(HOST_ARCH), i686)
	CPP_FLAGS += -m32
	SDK_LIB_SUBDIR=m32x86
else ifeq ($(HOST_ARCH), aarch64)
else
# Not supported
$(error Architecture $(HOST_ARCH) not configured for this installation.)
endif

TARGET = main

INCLUDES = -I$(IROOT)/include -I$(IROOT)/examples/common $(INC_GENICAM)
INCLUDES += -I./Base/ \
			-I./CmdManager/ \
			-I./DataTransfer/ \
			-I./CameraManager/ \
			-I./DataHandler/ \
			-I./DataType/ \
			-I./SysMessageManager/ \
			-I./ProcStat/ \
			-I$(ALGLIBINC) \
			-I./JpegEncoder/ \
			-I./Inifile/ \
			-I./DeviceManager/ \
			-I./Timer/ \
			-I./Log/
#
# Conditonal definitions for the common demo files
# (They depend on libraries installed in the system)
#
#include /home/tye/GigE_vision/GigE-V-Framework_2.10.0.0157/DALSA/GigeV/examples/common/commondefs.mk
ifneq ("$(shell find /usr/include -iname tiffio.h -print)","")
	COMMON_OPTIONS += -DLIBTIFF_AVAILABLE
	COMMONLIBS += -L/usr/lib -ltiff
endif

ifneq ("$(shell find /usr/include -iname jpeglib.h -print)","")
	COMMON_OPTIONS += -DLIBJPEG_AVAILABLE
endif

ifneq ("$(shell find /usr/include -iname png.h -print)","")
	COMMON_OPTIONS += -DLIBPNG_AVAILABLE
endif

ifneq ("$(shell find /usr/include -iname xlib.h -print)","")
	COMMON_OPTIONS += -DX11_DISPLAY_AVAILABLE
endif

ifneq ("$(shell find /usr/include -iname freeglut.h -print)","")
	COMMON_OPTIONS += -DFREEGLUT_DISPLAY_AVAILABLE
endif

CXX_COMPILE_OPTIONS = -c $(DEBUGFLAGS) -DPOSIX_HOSTPC -D_REENTRANT -fno-for-scope \
					  -Wall -Wno-parentheses -Wno-missing-braces -Wno-unused-but-set-variable \
					  -Wno-unknown-pragmas -Wno-cast-qual -Wno-unused-function -Wno-unused-label
C_COMPILE_OPTIONS = $(DEBUGFLAGS) -fhosted -Wall -Wno-parentheses -Wno-missing-braces \
					-Wno-unknown-pragmas -Wno-cast-qual -Wno-unused-function -Wno-unused-label -Wno-unused-but-set-variable

LINKLIBS = -L$(ARCHLIBDIR) $(COMMONLIBS) $(GENICAM_LIBS) -lpthread -lrt -lXext -lX11 -L/usr/local/lib -lGevApi -lCorW32 -lstdc++ -L$(ALGLIBBIN)
#/*-lEmbeddedLQ*/

include Rules.mk

VPATH=. : /home/tye/GigE_vision/GigE-V-Framework_2.10.0.0157/DALSA/GigeV/examples/common

SOURCES = ./CmdManager/CmdParse.cpp \
		  ./CmdManager/CmdBuild.cpp \
		  ./CmdManager/CmdManager.cpp \
		  ./CmdManager/CmdRegister.cpp \
		  ./DataTransfer/TcpCtrlr.cpp \
		  ./DataTransfer/UdpCtrlr.cpp \
		  ./DataTransfer/DataTransfer.cpp \
		  ./CameraManager/DalsaLinearCamera.cpp \
		  ./CameraManager/CameraCtrlr.cpp \
		  ./CameraManager/CameraManager.cpp \
		  ./DataHandler/DataHandler.cpp \
		  ./DataHandler/DataPkg.cpp \
		  ./DataType/DataType.cpp \
		  ./SysMessageManager/SystemMessageBase.cpp \
		  ./SysMessageManager/SystemMessageManager.cpp \
		  ./ProcStat/ProcessStatistics.cpp \
		  ./JpegEncoder/EncodeManager.cpp \
		  ./JpegEncoder/JpegEncoder.cpp \
		  ./Inifile/Inifile.cpp \
		  ./Timer/Timer.cpp \
		  ./DeviceManager/DeviceManager.cpp \
		  ./Log/Log.cpp \
		  ./main.cpp \
		  $(wildcard $(CLASS_DIR)/*.cpp)

%.o : %.cpp
	$(CXX) $(INCLUDES) $(CXX_COMPILE_OPTIONS) $(COMMON_OPTIONS) $(ARCH_OPTIONS) $(CPPFLAGS) -c $< -o $@
%.o : %.c
	$(GCC) $(INCLUDES) $(C_COMPILE_OPTIONS) $(COMMON_OPTIONS) $(ARCH_OPTIONS) -c $< -o $@

OBJECTS = GevUtils.o \
		  convertBayer.o \
		  GevFileUtils.o \
		  FileUtil_tiff.o \
		  X_Display_utils.o \
		  
OBJECTS += $(SOURCES:.cpp=.o)

$(CLASS_DIR)/%.o: $(CLASS_DIR)/%.cpp
	$(MAKE) -C $(CLASS_DIR)

$(TARGET):$(OBJECTS)
	$(CXX) $(INCLUDES) $(OBJECTS) $(LINKLIBS) $(CPPFLAGS) $(LDFLAGS) -o $(TARGET)


.PHONY: clean
clean:
	rm -f $(OBJECTS) $(TARGET)
