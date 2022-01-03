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
#include <LittleFS.h>

#include "grmcdorman/device/InfoDisplay.h"

namespace grmcdorman::device
{
    namespace
    {
        const char info_name[] PROGMEM = "System Overview";
        const char info_identifier[] PROGMEM = "system_overview";
    }

    InfoDisplay::InfoDisplay():
        Device(FPSTR(info_name), FPSTR(info_identifier)),
        title(F("<script>periodicUpdateList.push(\"system_overview\");</script>"
            )),
        host(F("Host"), F("host")),
        station_ssid(F("Connected to AP"), F("station_ssid")),
        rssi(F("Signal Strength"), F("rssi")),
        softap(F("Soft AP SSID"), F("softap")),
        heap_status(F("Allocatable memory"), F("heap_status")),
        uptime(F("Uptime"), F("uptime")),
        filesystem(F("File system status"), F("filesystem")),
        device_status(F("Sensor & controls status"), F("device_status"))
    {
        initialize({}, {&title, &host, &station_ssid, &rssi, &softap, &heap_status, &uptime, &filesystem, &device_status});
        host.set_request_callback([this] (const ::grmcdorman::InfoSettingHtml &) {
            // This, unfortunately, is the hostname configured in the WiFi setup.
            // It is _not_ the hostname that may be returned from a DHCP query, particularly
            // if said DHCP server has a static lease configured for this host. Note that
            // the DHCP protocol does support this (DHCPACK returns the assigned host name).
            //
            // Further, a `get host by address` function appears to be missing in the system libraries
            // (lwip/dns.h has dns_gethostbyname, no dns_gethostbyaddress or similar).
            //
            // As a result, there's no way to discover a DHCP and/or DNS assigned host name :-/
            host.set(WiFi.hostname() + F(" [") + WiFi.localIP().toString() + F("]"));
        });
        station_ssid.set_request_callback([this] (const ::grmcdorman::InfoSettingHtml &) {
            station_ssid.set(WiFi.SSID());
        });
        rssi.set_request_callback([this] (const ::grmcdorman::InfoSettingHtml &) {
            if (WiFi.RSSI() != 0)
            {
                // This creates a poor man's bar graph for the signal strength.
                // The values are, according to Google U, equivalent to the bars
                // on most phones.
                static constexpr size_t bar_sequence_size = sizeof("◾") - 1;
                static const char bars_string[] PROGMEM = "◾◾◾◾ ";
                static const char dbm[] PROGMEM = " dBm";
                auto signal = WiFi.RSSI();
                const char *bars;

                if (signal < -89 || signal == 0)
                {
                    bars = &bars_string[4 * bar_sequence_size + 1]; // empty; also skip space.
                }
                else if (signal < -78)  // -78 to -88
                {
                    bars = &bars_string[3 * bar_sequence_size]; // 1 bar
                }
                else if (signal < -67) // -67 to -77
                {
                    bars = &bars_string[2 * bar_sequence_size]; // 2 bars
                }
                else if (signal < -56) // -56 to -66
                {
                    bars = &bars_string[1 * bar_sequence_size]; // 3 bars
                }
                else // -55 or higher
                {
                    bars = &bars_string[0];
                }

                // The 'message' array is sized to the maximum size of 'bars',
                // the size of the 'dbm' string, plus 4 characters for the signal value.
                // There will also be two extra characters from the null bytes on
                // `bars_string` and `dbm`.
                //
                // This C-style assembly - instead of concatenating String objects - is used
                // for memory efficiency, which matters on the ESP.
                char message[sizeof(bars_string) + sizeof(dbm) + 4];  // reserves about 4 characters for the signal value, which can only range from -128 to +127.
                strcpy_P(message, bars);
                itoa(signal, &message[strlen(message)], 10);
                strcat_P(message, dbm);
                rssi.set(message);
            }
            else
            {
                rssi.set(String());
            }
        });
        softap.set_request_callback([this] (const ::grmcdorman::InfoSettingHtml &) {
            softap.set(WiFi.softAPSSID());
        });
        heap_status.set_request_callback([this] (const ::grmcdorman::InfoSettingHtml &) {
            static const char bytes_frag[] PROGMEM = " bytes (fragmentation: ";
            static const char closing_paren[] PROGMEM = ")";
            char heap_status_string[sizeof(bytes_frag) + sizeof(closing_paren) + 22]; // allowing 11 digits each for free heap, fragmentation
            itoa(ESP.getFreeHeap(), heap_status_string, 10);
            strcat_P(heap_status_string, bytes_frag);
            itoa(ESP.getHeapFragmentation(), heap_status_string + strlen(heap_status_string), 10);
            strcat_P(heap_status_string, closing_paren);
            heap_status.set(heap_status_string);
        });
        uptime.set_request_callback([this] (const ::grmcdorman::InfoSettingHtml &) {
            auto now = millis();
            auto hours = now / 1000 / 60 /60;
            auto minutes = now / 1000 / 60  % 60;
            auto seconds = now / 1000 % 60;
            char message[sizeof("000000:00:00")]; // This allows hours up to 99999, which is larger than 1200 hours.
            snprintf(message, sizeof (message) -1, "%ld:%02ld:%02ld", hours, minutes, seconds);
            uptime.set(message);
        });
        filesystem.set_request_callback([this] (const ::grmcdorman::InfoSettingHtml &) {
            fs::FSInfo64 info;
            if (LittleFS.info64(info))
            {
                static const char first_part[] PROGMEM = "LittleFS: total bytes ";
                static const char second_part[] PROGMEM = ", used bytes: ";
                // Try to build message without too many mallocs.
                String message;
                message.reserve(sizeof (first_part) + sizeof(second_part) + 64);    // Allow up to 32 digits each for totalBytes, usedBytes, but can grow
                message += FPSTR(first_part);
                message += info.totalBytes;
                message += FPSTR(second_part);
                message += info.usedBytes;
                filesystem.set(message);
            }
            else
            {
                filesystem.set(F("No LittleFS information available"));
            }
        });

        device_status.set_request_callback([this] (const ::grmcdorman::InfoSettingHtml &) {
            on_request_device_status();
        });
    }

    void InfoDisplay::on_request_device_status()
    {
        if (devices == nullptr)
        {
            return;
        }

        // Get all enabled devices reporting a non-blank status message.
        // These arrays are going to be oversized; the extra values should
        // not hurt.
        String messages[devices->size()];
        const __FlashStringHelper *names[devices->size()];
        int count = 0;
        size_t message_bytes(0);
        for (const auto &device: *devices)
        {
            if (!device->is_enabled() || device == this || device == nullptr)
            {
                continue;
            }

            messages[count] = device->get_status();
            if (!messages[count].isEmpty())
            {
                names[count] = device->name();
                // Each message line format:
                // <device-name>: <message><br>
                // i.e device name length, 2 characters, message length, 4 characters.
                message_bytes += messages[count].length() + strlen_P(reinterpret_cast<const char *>(device->name())) + 6;
                ++count;
            }
        }

        if (message_bytes > 0)
        {
            String full_message;
            full_message.reserve(message_bytes);
            for (int index = 0; index < count; ++index)
            {
                if (index != 0)
                {
                    full_message += F("<br>");
                }
                full_message += names[index];
                full_message += ':';
                full_message += ' ';
                full_message += messages[index];
            }

            device_status.set(full_message);
        }
        else
        {
            device_status.set(String());
        }
    }

    DynamicJsonDocument InfoDisplay::as_json() const
    {
        DynamicJsonDocument json(1024);

        static const char enabled_string[] PROGMEM = "enabled";
        static const char host_string[] PROGMEM = "host";
        static const char ip_string[] PROGMEM = "ip";
        static const char station_ssid_string[] PROGMEM = "station_ssid";
        static const char rssi_string[] PROGMEM = "rssi";
        static const char softap_string[] PROGMEM = "softap";
        static const char heap_string[] PROGMEM = "heap";
        static const char free_string[] PROGMEM = "free";
        static const char fragmentation_string[] PROGMEM = "fragmentation";
        static const char uptime_string[] PROGMEM = "uptime_seconds";
        static const char filesystem_string[] PROGMEM = "littlefs";
        static const char used_string[] PROGMEM = "used";

        json[FPSTR(enabled_string)] = is_enabled();
        json[FPSTR(host_string)] = WiFi.hostname();
        json[FPSTR(ip_string)] = WiFi.localIP().toString();
        json[FPSTR(station_ssid_string)] = WiFi.SSID();
        json[FPSTR(softap_string)] = WiFi.softAPSSID();
        DynamicJsonDocument heap_status(128);
        heap_status[FPSTR(free_string)] = ESP.getFreeHeap();
        heap_status[FPSTR(fragmentation_string)] = ESP.getHeapFragmentation();
        json[FPSTR(heap_string)] = std::move(heap_status);
        json[FPSTR(uptime_string)] = millis() / 1000;
        // JSON does not support 64-bit.
        fs::FSInfo info;
        if (LittleFS.info(info))
        {
            DynamicJsonDocument fs_status(128);
            fs_status[FPSTR(free_string)] = info.totalBytes - info.usedBytes;
            fs_status[FPSTR(used_string)] = info.usedBytes;
            json[FPSTR(filesystem_string)] = std::move(fs_status);
        }

        return json;
    }
}