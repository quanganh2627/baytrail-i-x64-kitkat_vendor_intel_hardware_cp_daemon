#
# CP DAEMON 
#
ifeq ($(USE_GPS_CP_DAEMON),true)
ifneq (,$(filter true,$(BOARD_HAVE_GPS_CSR_GSD4T) $(BOARD_HAVE_GPS_CSR_GSD5T)))

LOCAL_PATH := $(call my-dir)


#
# cpdd - e911 CP daemon
#
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

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

LOCAL_MODULE_TAGS := optional


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

# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
# Unit-test code:
# =======================================================
# the folowing builds build unit-test code, not part of released code, for debugging only!
# =======================================================
#
# cpd - e911 CP deamon simulator
#
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := eng

ifeq ($(TARGET_BUILD_VARIANT),eng)
COMMON_GLOBAL_CFLAGS += -DCPDD_DEBUG_ENABLED
endif


LOCAL_C_INCLUDES:=          \
    external/icu4c/common   \
    external/icu4c/i18n     \
    external/libxml2/include

LOCAL_SRC_FILES :=  cpd.c  \
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

LOCAL_MODULE := cpd

include $(BUILD_EXECUTABLE)
endif
endif
