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

# Include common espresso BoardConfig
-include device/samsung/espresso-common/BoardConfigCommon.mk

TARGET_BOARD_INFO_FILE := device/samsung/p5100/board-info.txt

# assert
TARGET_OTA_ASSERT_DEVICE := espresso,p5100,GT-P5100,espresso10rf,espresso10rfxx

# Use the non-open-source parts, if they're present
-include vendor/samsung/p51xx/BoardConfigVendor.mk
