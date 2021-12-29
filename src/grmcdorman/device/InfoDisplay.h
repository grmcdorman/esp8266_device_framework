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
            /**
             * @brief Add a list of devices that will report status.
             *
             * The status message (`get_status`) will be reported any device
             * that is enabled and returns a non-blank status message.
             *
             * @param list      List of devices to query for status reports..
             */
            virtual void set_devices(const std::vector<Device *> &list) override
            {
                devices = &list;
            }

            /**
             * @brief Get a status report.
             *
             * For this device, it is the RSSI value. No other values
             * are reported.
             *
             * The `on_request_device_status` will bypass collecting this status
             * if this device is included in the reporting list.
             *
             * @return String containing status report.
             */
            virtual String get_status() const
            {
                return String();
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
            InfoSettingHtml device_status;                  //!< Status of linked devices.

            const std::vector<Device *> *devices = nullptr; //!< The list of attached devices which will report status.

            /**
             * @brief Accumulate status message for all attached enabled devices.
             *
             */
            void on_request_device_status();
    };
}