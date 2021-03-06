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

#include <ESP8266WiFi.h>
#include "grmcdorman/device/WifiDisplay.h"

namespace grmcdorman::device
{
    namespace {
        inline const __FlashStringHelper *bool_to_yesno(bool value)
        {
            static const char yes[] PROGMEM = "Yes";
            static const char no[] PROGMEM = "No";
            return value ? FPSTR(yes) : FPSTR(no);
        }

        const char info_name[] PROGMEM = "WiFi Status";
        const char info_identifier[] PROGMEM = "wifi_status";
    }

    WifiDisplay::WifiDisplay():
        Device(FPSTR(info_name), FPSTR(info_identifier)),
        title(F("<script>periodicUpdateList.push(\"wifi_status"
            // access_point_mac and MAC address are fixed and don't need to be updated.
            // Really, a lot of these are unlikely to change while visible in a browser anyway...
            "&setting=access_point_ip"
            "&setting=bssid"
            "&setting=station_ip"
            "&setting=station_gateway_ip"
            "&setting=station_subnet_mask"
            "&setting=dns_server"
            "&setting=station_mac "
            "&setting=station_connected"
            "&setting=station_autoconnect"
            "\");</script>"
            )),
        access_point_ip(F("Soft AP IP Address"), F("access_point_ip")),
        access_point_mac(F("Soft AP MAC Address"), F("access_point_mac")),
        wifi_bssid(F("BSSID"), F("bssid")),
        station_ip(F("IP Address"), F("station_ip")),
        station_gateway_ip(F("Gateway IP Address"), F("station_gateway_ip")),
        station_subnet_mask(F("Subnet Mask"), F("station_subnet_mask")),
        dns_server(F("DNS Server Address"), F("dns_server")),
        station_mac(F("MAC Address"), F("station_mac")),
        station_connected(F("Connected"), F("station_connected")),
        station_autoconnect(F("Auto Connect"), F("station_autoconnect"))
    {
        initialize({}, {&title, &access_point_ip, &access_point_mac, &wifi_bssid, &station_ip, &station_gateway_ip, &station_subnet_mask,
            &dns_server, &station_mac, &station_connected, &station_autoconnect});

        access_point_ip.set_request_callback([this] (const ::grmcdorman::InfoSettingHtml &) {
            access_point_ip.set(WiFi.softAPIP().toString());
        });
        access_point_mac.set_request_callback([this] (const ::grmcdorman::InfoSettingHtml &) {
            access_point_mac.set(WiFi.softAPmacAddress());
        });
        wifi_bssid.set_request_callback([this] (const ::grmcdorman::InfoSettingHtml &) {
            wifi_bssid.set(WiFi.BSSIDstr());
        });
        station_ip.set_request_callback([this] (const ::grmcdorman::InfoSettingHtml &) {
            station_ip.set(WiFi.localIP().toString());
        });
        station_gateway_ip.set_request_callback([this] (const ::grmcdorman::InfoSettingHtml &) {
            station_gateway_ip.set(WiFi.gatewayIP().toString());
        });
        station_subnet_mask.set_request_callback([this] (const ::grmcdorman::InfoSettingHtml &) {
            station_subnet_mask.set(WiFi.subnetMask().toString());
        });
        dns_server.set_request_callback([this] (const ::grmcdorman::InfoSettingHtml &) {
            dns_server.set(WiFi.dnsIP().toString());
        });
        station_mac.set_request_callback([this] (const ::grmcdorman::InfoSettingHtml &) {
            station_mac.set(WiFi.macAddress());
        });
        station_connected.set_request_callback([this] (const ::grmcdorman::InfoSettingHtml &) {
            station_connected.set(bool_to_yesno(WiFi.isConnected()));
        });
        station_autoconnect.set_request_callback([this] (const ::grmcdorman::InfoSettingHtml &) {
            station_autoconnect.set(bool_to_yesno(WiFi.getAutoConnect()));
        });

    }
    DynamicJsonDocument WifiDisplay::as_json() const
    {
        DynamicJsonDocument json(128 * get_settings().size());

        static const char enabled_string[] PROGMEM = "enabled";

        json[FPSTR(enabled_string)] = is_enabled();
        for (const auto &setting: get_settings())
        {
            if (setting->send_to_ui() && setting != &title && setting != &enabled && setting != &station_connected && setting != &station_autoconnect)
            {
                // This will invoke the request callback, which will update the contents as a side effect.
                json[setting->name()] = setting->as_string();
            }
        }
        // These two are booleans.
        json[station_connected.name()] = WiFi.isConnected();
        json[station_autoconnect.name()] = WiFi.getAutoConnect();

        return json;
    }
}