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
            /**
             * @brief Publish to MQTT.
             *
             * This class does not publish to MQTT.
             * @param json  Not used.
             * @return `false`  No data published.
             */
            bool publish(DynamicJsonDocument &json) override
            {
                return false;
            }
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