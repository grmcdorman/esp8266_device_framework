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

#include "grmcdorman/device/WifiSetup.h"

namespace grmcdorman::device
{
    namespace {
        const char ssid_string_lower[] PROGMEM = "ssid";
        const char wifi_string_lower[] PROGMEM = "wifi";
        auto SSID_STRING_LOWER = FPSTR(ssid_string_lower);
        auto WIFI_STRING_LOWER = FPSTR(wifi_string_lower);
        const char wifi_name[] PROGMEM = "WiFi";
        const char wifi_identifier[] PROGMEM = "wifi_setup";
        class WifiSetupDevice_Definition: public Device::Definition
        {
            public:
                virtual const __FlashStringHelper *get_name_suffix() const override
                {
                    return F(" WiFi");
                }
                virtual const __FlashStringHelper *get_value_template() const override
                {
                    return F("{{value_json.wifi.rssi}}");
                }
                virtual const __FlashStringHelper *get_unique_id_suffix() const override
                {
                    return F("_wifi");
                }
                virtual const __FlashStringHelper *get_unit_of_measurement() const override
                {
                    return F("dBm");
                }
                virtual const __FlashStringHelper *get_json_attributes_template() const override
                {
                    return F("{\"ssid\": \"{{value_json.wifi.ssid}}\", \"ip\": \"{{value_json.wifi.ip}}\"}");
                }
                virtual const __FlashStringHelper *get_icon() const override
                {
                    return F("mdi:wifi");
                }
        };
    }

    WifiSetup::WifiSetup():
        Device(FPSTR(wifi_name), FPSTR(wifi_identifier)),
        hostname(F("Hostname"), F("hostname")),
        ssid(F("Access point SSID"), SSID_STRING_LOWER),
        password(F("Access point password"), F("password")),
        use_dhcp(F("Obtain an IP address automatically"), F("use_dhcp")),
        ip_address(F("IP address"), F("ip_address")),
        subnet_mask(F("Subnet mask"), F("subnet_mask")),
        default_gateway(F("Default gateway"), F("default_gateway")),
        auto_dns(F("Obtain DNS server address automatically"), F("auto_dns")),
        dns_1(F("Preferred DNS server"), F("dns_1")),
        dns_2(F("Alternative DNS server"), F("dns_2")),
        connection_timeout(F("Connection timeout (seconds)"), F("connection_timeout")),
        publish_rssi(F("Publish WiFi signal strength"), F("publish_rssi"))
    {
        const static WifiSetupDevice_Definition wifi_device_definition;

        initialize({&wifi_device_definition}, {&hostname, &ssid, &password, &use_dhcp, &ip_address, &subnet_mask, &default_gateway,
            &auto_dns, &dns_1, &dns_2,
            &connection_timeout,
            &publish_rssi});

        use_dhcp.set(true);
        publish_rssi.set(true);
        auto_dns.set(true);
        connection_timeout.set(60);
    }

    void WifiSetup::set_defaults()
    {
        hostname.set(get_system_identifier());
    }

    void WifiSetup::setup()
    {
        connect_to_ap();

        if (WiFi.status() != WL_CONNECTED)
        {
            // No SSID. Start in AP.
            Serial.println(F("Starting in AP mode"));
  #ifdef ESP8266
    // @bug workaround for bug #4372 https://github.com/esp8266/Arduino/issues/4372
            WiFi.enableAP(true);
            delay(500); // workaround delay
  #endif
            WiFi.mode(WIFI_AP);
            // "target_hostname" is the SSID.
            WiFi.softAP(hostname.get().isEmpty()? get_system_identifier() : hostname.get());
            delay(500);
            Serial.print(F("Soft AP started at address "));
            Serial.println(WiFi.softAPIP().toString());
            dns_server = std::make_unique<DNSServer>();
            dns_server->setErrorReplyCode(DNSReplyCode::NoError);
            dns_server->start(53, "*", WiFi.softAPIP());
        }
    }

    void WifiSetup::connect_to_ap()
    {
        String target_hostname(hostname.get().isEmpty()? get_system_identifier() : hostname.get());

        if (ip_address.get().isEmpty() ||
            subnet_mask.get().isEmpty())
        {
            use_dhcp.set(true);
        }

        if (!ssid.get().isEmpty())
        {
            tried_connect_on_setup = true;

            Serial.print(F("Attempting to connect to "));
            Serial.println(ssid.get());
            // Do not use persistent WiFi settings, we manage those ourselves.
            WiFi.persistent(false);
            if (!WiFi.mode(WIFI_STA))
            {
                Serial.println(F("Unable to set STA mode"));
            }
            if (!WiFi.hostname(target_hostname))
            {
                Serial.println(F("Unable to set host name"));
            }

            WiFi.begin(ssid.get(), password.get());

            if (use_dhcp.get())
            {
                if (!WiFi.config(0u, 0u, 0u))
                {
                    Serial.println(F("Config for DHCP failed"));
                }
            }
            else
            {
                // This will explode if the user hasn't set things correctly. :-(
                IPAddress host_ip;
                IPAddress gateway_ip;
                IPAddress subnet_mask_ip;
                IPAddress dns_1_ip;
                IPAddress dns_2_ip;
                if (!host_ip.fromString(ip_address.get()))
                {
                    Serial.print(F("Host IP address '"));
                    Serial.print(ip_address.get());
                    Serial.println(F("' could not be converted to IP."));
                }
                if (!subnet_mask_ip.fromString(subnet_mask.get()))
                {
                    Serial.print(F("Subnet mask '"));
                    Serial.print(subnet_mask.get());
                    Serial.println(F("' could not be converted to IP."));
                }
                gateway_ip.fromString(default_gateway.get());
                if (!auto_dns.get())
                {
                    dns_1_ip.fromString(dns_1.get());
                    dns_2_ip.fromString(dns_2.get());
                }

                WiFi.config(host_ip, gateway_ip, subnet_mask_ip, dns_1_ip, dns_2_ip);
            }
            // Loop continuously while WiFi is not connected
            unsigned int tries = 0;
            // Each poll is 100ms, so 10 polls = 1 second.
            auto status = WiFi.status();
            while (status != WL_CONNECTED && status != WL_CONNECT_FAILED && tries < connection_timeout.get() * 10)
            {
                delay(100);
                ++tries;
                status = WiFi.status();
            }
            if (status != WL_CONNECTED)
            {
                Serial.print(F("Unable to connect to the access point, status =") + String(status));
            }
        }

    }
    void WifiSetup::loop()
    {
        if (dns_server)
        {
            if (!ssid.get().isEmpty() && !tried_connect_on_setup)
            {
                // There is an SSID, but there was no attempt to connect on the `setup` call.
                // Try to connect now. This may disconnect the Soft AP.
                connect_to_ap();
                if (WiFi.status() == WL_CONNECTED)
                {
                    dns_server.reset();
                    return;
                }
            }

            dns_server->processNextRequest();
        }
    }

    bool WifiSetup::publish(DynamicJsonDocument &json) const
    {
        if (!publish_rssi.get())
        {
            return false;
        }

        // This device is unique in not using the device identifier here.
        json[WIFI_STRING_LOWER] = as_json();

        return true;
    }

    DynamicJsonDocument WifiSetup::as_json() const
    {
        static const char enabled_string[] PROGMEM = "enabled";
        DynamicJsonDocument json(512);

        json[FPSTR(enabled_string)] = is_enabled();
        json[SSID_STRING_LOWER] = WiFi.SSID();
        json[F("ip")] = WiFi.localIP().toString();
        json[F("rssi")] = WiFi.RSSI();

        return json;
    }
}
