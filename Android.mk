#
# CP DAEMON 
#

LOCAL_PATH := $(call my-dir)


#
# cpdd - e911 CP daemon
#
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := eng

LOCAL_C_INCLUDES:=          \
    external/icu4c/common   \
    external/icu4c/i18n     \
    external/libxml2/include

LOCAL_SRC_FILES :=  cpdd.c  \
					cpdInit.c  \
					cpdStart.c \
					cpdUtil.c \
					cpdModem.c \
					cpdModemReadWrite.c \
					cpdXmlParser.c \
					cpdXmlUtils.c \
					cpdDebug.c \
					cpdXmlFormatter.c\
					cpdGpsComm.c  \
					cpdSocketServer.c \
					cpdSystemMonitor.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)

LOCAL_STATIC_LIBRARIES := libc libxml2
LOCAL_SHARED_LIBRARIES := libicuuc libcutils

LOCAL_MODULE := cpdd

include $(BUILD_EXECUTABLE)


#
# CPDD communication library for GPS.
# GPS links this library.
#
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := eng


LOCAL_SRC_FILES += \
                    $(CPD_PATH)/cpdInit.c  \
                    $(CPD_PATH)/cpdUtil.c \
                    $(CPD_PATH)/cpdDebug.c \
                    $(CPD_PATH)/cpdGpsComm.c  \
                    $(CPD_PATH)/cpdSocketServer.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)

LOCAL_STATIC_LIBRARIES := libc 
LOCAL_SHARED_LIBRARIES := libicuuc libcutils

LOCAL_MODULE := libCpd
include $(BUILD_STATIC_LIBRARY)
