#
# Copyright (C) 2016 The CyanogenMod Project
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

# Release name
PRODUCT_RELEASE_NAME := p5100

# Boot animation
# TARGET_SCREEN_HEIGHT := 800
TARGET_SCREEN_HEIGHT := 480
TARGET_SCREEN_WIDTH := 1280

# Inherit common CM configuration
$(call inherit-product, vendor/cm/config/common_full_phone.mk)

# CyanogenMod specific overlay
DEVICE_PACKAGE_OVERLAYS += device/samsung/p5100/overlay/cm
DEVICE_PACKAGE_OVERLAYS += device/samsung/espresso-common/overlay/cm-common

# Use 44.1 kHz UI sounds
$(call inherit-product-if-exists, frameworks/base/data/sounds/AudioPackage13.mk)

# Inherit from the common Open Source product configuration
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)

# Inherit device specific configuration
$(call inherit-product, device/samsung/p5100/device.mk)

# Device identifier. This must come after all inclusions
PRODUCT_DEVICE := p5100
PRODUCT_NAME := cm_p5100
PRODUCT_BRAND := samsung
PRODUCT_MODEL := GT-P5100
PRODUCT_MANUFACTURER := samsung

# Set build fingerprint / ID / Product Name etc.
PRODUCT_BUILD_PROP_OVERRIDES += \
    PRODUCT_NAME=espresso10rfxx \
    TARGET_DEVICE=espresso10rf \
    BUILD_FINGERPRINT="samsung/espresso10rfxx/espresso10rf:4.2.2/JDQ39/P5100XXDNA1:user/release-keys" \
    PRIVATE_BUILD_DESC="espresso10rfxx-user 4.2.2 JDQ39 P5100XXDNA1 release-keys"
