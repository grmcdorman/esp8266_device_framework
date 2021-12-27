#pragma once


#include "grmcdorman/device/Device.h"
#include "grmcdorman/Setting.h"

namespace grmcdorman::device
{
    /**
     * @brief This class is a readonly system-information panel.
     *
     * It does not publish data and has no configuration.
     *
     * All fields in this panel update periodically.
     */
    class InfoDisplay: public Device
    {
        public:
            /**
             * @brief Construct a new InfoDisplay Device object.
             *
             */
            InfoDisplay();

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
            InfoSettingHtml host;                           //!< The host name.
            InfoSettingHtml station_ssid;                   //!< The station SSID (connected AP).
            InfoSettingHtml rssi;                           //!< The signal strength.
            InfoSettingHtml softap;                         //!< The Soft AP SSID.
            InfoSettingHtml heap_status;                    //!< Free heap, max heap alloc, fragmentation.
            InfoSettingHtml uptime;                         //!< System up time.
            InfoSettingHtml filesystem;                     //!< File system information, if available.
    };
}