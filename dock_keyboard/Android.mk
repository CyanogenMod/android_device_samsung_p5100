LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= dock_keyboard_attach.c
LOCAL_MODULE:= dock_kbd_attach
LOCAL_MODULE_PATH := $(TARGET_OUT_EXECUTABLES)
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES += $(KERNEL_HEADERS) 
LOCAL_SHARED_LIBRARIES += libutils libcutils

include $(BUILD_EXECUTABLE)
