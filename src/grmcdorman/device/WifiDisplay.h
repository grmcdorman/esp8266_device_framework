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
     * @brief This class is a readonly WiFi-information panel.
     *
     * It does not publish data and has no configuration.
     *
     * All fields in this panel update periodically.
     */
    class WifiDisplay: public Device
    {
        public:
            /**
             * @brief Construct a new WifiDisplay Device object.
             *
             */
            WifiDisplay();

            /**
             * @brief Setup.
             *
             * This class has no setup operations.
             */
            void setup() override
            {
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
            NoteSetting title;                              //!< The panel title. Includes script for panel updating.
            InfoSettingHtml access_point_ip;
            InfoSettingHtml access_point_mac;
            InfoSettingHtml wifi_bssid;
            InfoSettingHtml station_ip;
            InfoSettingHtml station_gateway_ip;
            InfoSettingHtml station_subnet_mask;
            InfoSettingHtml dns_server;
            InfoSettingHtml station_mac;
            InfoSettingHtml station_connected;
            InfoSettingHtml station_autoconnect;
    };
}