#
# Copyright (C) 2015 SlimRoms
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

# This variable is set first, so it can be overridden
# by BoardConfigVendor.mk

-include device/samsung/espresso-common/BoardConfigCommon.mk

# Bluetooth
BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := device/samsung/p5100/bluetooth
BOARD_BLUEDROID_VENDOR_CONF := device/samsung/p5100/bluetooth/vnd_espresso10.txt

# Use the non-open-source parts, if they're present
-include vendor/samsung/p51xx/BoardConfigVendor.mk
