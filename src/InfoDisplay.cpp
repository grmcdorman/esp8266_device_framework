
#include <ESP8266WiFi.h>
#include <LittleFS.h>

#include "grmcdorman/device/InfoDisplay.h"

namespace grmcdorman::device
{
    static const char info_name[] PROGMEM = "System Overview";

    InfoDisplay::InfoDisplay():
        Device(FPSTR(info_name), FPSTR(info_name)),
        title(F("<script>window.addEventListener(\"load\", () => { periodicUpdateList.push(\"System Overview\"); });</script>"
            )),
        host(F("Host"), F("host")),
        station_ssid(F("Connected to AP"), F("station_ssid")),
        rssi(F("Signal Strength"), F("rssi")),
        softap(F("Soft AP SSID"), F("softap")),
        heap_status(F("Allocatable memory"), F("heap_status")),
        uptime(F("Uptime"), F("uptime")),
        filesystem(F("File system status"), F("filesystem"))
    {
        initialize({}, {&title, &host, &station_ssid, &rssi, &softap, &heap_status, &uptime, &filesystem});
        host.set_request_callback([this] (const ::grmcdorman::InfoSettingHtml &) {
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
                filesystem.set("LittleFS: total bytes " + String(info.totalBytes) + ", used bytes: " + String(info.usedBytes));
            }
            else
            {
                filesystem.set("No LittleFS information available");
            }
        });
    }
}