/*
 * Copyright (c) 2021, 2022 G. R. McDorman
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once


#include "grmcdorman/device/Device.h"
#include "grmcdorman/Setting.h"

namespace grmcdorman::device
{
    /**
     * @brief This class is a readonly system-details panel.
     *
     * It does not publish data and has no configuration.
     *
     * This panel contains only static non-updating data.
     */
    class SystemDetailsDisplay: public Device
    {
        public:
            /**
             * @brief Construct a new SystemDetailsDisplay Device object.
             *
             */
            SystemDetailsDisplay();

            /**
             * @brief The device identifier, "system_details".
             *
             * @return Device identifier.
             */
            const __FlashStringHelper *identifier() const override
            {
                static const char info_name[] PROGMEM = "system_details";
                return FPSTR(info_name);
            }

            /**
             * @brief Setup.
             *
             * Set the firmware identifier.
             */
            void setup() override
            {
                firmware_name.set(get_firmware_name());
            }
            /**
             * @brief Loop.
             *
             * This class has no loop operations.
             */
            void loop() override
            {
            }
            DynamicJsonDocument as_json() const override;
        private:
            InfoSettingHtml firmware_name;                  //!< Firmware identifier, as set by the system.
            InfoSettingHtml compile_datetime;               //!< Compile date and time.
            InfoSettingHtml architecture;                   //!< The platform architecture. This is determined at compile time.
            InfoSettingHtml device_chip_id;                 //!< The device-specific chip ID.
            InfoSettingHtml flash_chip;                     //!< The flash chip ID.
            InfoSettingHtml last_reset;                     //!< Last reset reason.
            InfoSettingHtml flash_size;                     //!< Flash size.
            InfoSettingHtml real_flash_size;                //!< Real flash size.
            InfoSettingHtml sketch_size;                    //!< Sketch size, total sketch space available.
            InfoSettingHtml vendor_chip_id;                 //!< Vendor chip ID (hex).
            InfoSettingHtml core_version;                   //!< The core version.
            InfoSettingHtml boot_version;                   //!< The boot version.
            InfoSettingHtml sdk_version;                    //!< SDK version.
            InfoSettingHtml cpu_frequency;                  //!< CPU frequency.
    };
}