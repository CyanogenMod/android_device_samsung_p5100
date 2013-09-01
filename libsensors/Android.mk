#
# Copyright (C) 2013 Paul Kocialkowski
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

LOCAL_PATH := $(call my-dir)
PIRANHA_SENSORS_PATH := $(LOCAL_PATH)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	piranha_sensors.c \
	input.c \
	bma250.c \
	yas530c.c \
	yas_orientation.c \
	bh1721.c \
	gp2a_light.c \
	gp2a_proximity.c

LOCAL_SHARED_LIBRARIES := libutils libcutils liblog libhardware
LOCAL_PRELINK_MODULE := false

LOCAL_MODULE := sensors.piranha
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE_TAGS := optional

ifeq ($(TARGET_DEVICE),p5100)
	LOCAL_CFLAGS += -DTARGET_DEVICE_P5100
endif
ifeq ($(TARGET_DEVICE),p5110)
	LOCAL_CFLAGS += -DTARGET_DEVICE_P5100
endif
ifeq ($(TARGET_DEVICE),p3100)
	LOCAL_CFLAGS += -DTARGET_DEVICE_P3100
endif
ifeq ($(TARGET_DEVICE),p3110)
	LOCAL_CFLAGS += -DTARGET_DEVICE_P3100
endif

include $(BUILD_SHARED_LIBRARY)

LOCAL_PATH := $(PIRANHA_SENSORS_PATH)/geomagneticd

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	geomagneticd.c

LOCAL_SHARED_LIBRARIES := libutils libcutils liblog
LOCAL_PRELINK_MODULE := false

LOCAL_MODULE := geomagneticd
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)

LOCAL_PATH := $(PIRANHA_SENSORS_PATH)/orientationd

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	input.c \
	vector.c \
	orientationd.c

LOCAL_SHARED_LIBRARIES := libutils libcutils liblog
LOCAL_PRELINK_MODULE := false

LOCAL_MODULE := orientationd
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
