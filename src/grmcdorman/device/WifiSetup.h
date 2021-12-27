#pragma once

#include "grmcdorman/device/Device.h"

#include <DNSServer.h>

namespace grmcdorman::device
{
    /**
     * @brief This "device" supports WiFi configuration.
     *
     * In particular, on a fresh install or reset, it will create a soft AP,
     * with the local host set as a captive portal. Once connected, it will
     * initialize the WiFi connection in station (STA) mode with the configured
     * parameters.
     *
     * Failure to connect to the configured station will result in falling back
     * to the soft AP.
     *
     * Note that this is similar to WifiManager; however, the web service is
     * not present in this device. It should be possible to use entirely different
     * mechanisms to configure the WiFi connection without ever creating a web server.
     *
     * At present, the soft AP is not password protected.
     */
    class WifiSetup: public Device
    {
        public:
            WifiSetup();

            /**
             * @brief Set defaults.
             *
             * This sets the host name and SoftAP SSID from the system identifier.
             */
            void set_defaults() override;
            void setup() override;
            void loop() override;
            bool publish(DynamicJsonDocument &json) override;

            /**
             * @brief Get the local host name.
             *
             * This is the configured host name; it is not necessarily
             * the host name as actually present in the WiFi class.
             * @return const String&
             */
            const String &get_configured_local_hostname() const
            {
                return hostname.get();
            }

        private:
            void connect_to_ap();                           //!< Try to connect to the configured AP.
            StringSetting hostname;                         //!< The local host name configuration.
            StringSetting ssid;                             //!< The SSID to connect to in station mode.
            PasswordSetting password;                       //!< The password for the SSID.
            ToggleSetting use_dhcp;                         //!< When connecting, use DHCP instead of a fixed address.
            StringSetting ip_address;                       //!< The station fixed address; ignored if use_dhcp is true.
            StringSetting subnet_mask;                      //!< The station subnet mask; ignore if use_dhcp is true.
            StringSetting default_gateway;                  //!< The default gateway; ignored if use_dhcp is true.
            ToggleSetting auto_dns;                         //!< When connecting, automatically set DNS servers.
            StringSetting dns_1;                            //!< DNS server 1; ignored if auto_dns is true.
            StringSetting dns_2;                            //!< DNS server 2; ignored if auto_dns is true.
            UnsignedIntegerSetting connection_timeout;      //!< Station association timeout, in seconds.
            ToggleSetting publish_rssi;                     //!< If true, publish RSSI (signal quality) to MQTT.

            bool tried_connect_on_setup = false;            //!< If true, `setup` tried to connect to an AP.

            std::unique_ptr<DNSServer> dns_server;          //!< The local DNS server for Soft AP mode.
    };
}
