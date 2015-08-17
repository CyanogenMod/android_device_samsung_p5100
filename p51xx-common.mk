#
# Copyright (C) 2012 The CyanogenMod Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Include espresso-common makefile
$(call inherit-product, device/samsung/espresso-common/espresso-common.mk)

LOCAL_PATH := device/samsung/p5100

# Enable higher-res drawables while keeping mdpi as primary source
PRODUCT_AAPT_CONFIG := large
PRODUCT_AAPT_PREF_CONFIG := mdpi
PRODUCT_LOCALES += mdpi

# Ramdisk
PRODUCT_PACKAGES += \
    fstab.espresso \
    fstab.espresso10 \
    init.espresso10.usb.rc \
    init.espresso10.rc \
    ueventd.espresso10.rc

$(call inherit-product-if-exists, vendor/samsung/p51xx/p51xx-vendor.mk)
