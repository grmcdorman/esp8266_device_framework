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

#include <Arduino.h>
#include <LittleFS.h>

#include "grmcdorman/device/SystemDetailsDisplay.h"

namespace grmcdorman::device
{
    namespace {
        const char details_name[] PROGMEM = "System Details";
        const char details_identifier[] PROGMEM = "system_details";
    }

    SystemDetailsDisplay::SystemDetailsDisplay():
        Device(FPSTR(details_name), FPSTR(details_identifier)),
        firmware_name(F("Installed Firmware"), F("firmware_name")),
        compile_datetime(F("Firmware built"), F("compile_datetime")),
        architecture(F("Architecture"), F("architecture")),
        device_chip_id(F("Device Chip ID"), F("device_chip_id")),
        flash_chip(F("Flash Chip ID"), F("flash_chip")),
        last_reset(F("Last reset reason"), F("last_reset")),
        flash_size(F("Flash memory size"), F("flash_size")),
        real_flash_size(F("Real flash size"), F("real_flash_size")),
        sketch_size(F("Sketch space"), F("sketch_size")),
        vendor_chip_id(F("Vendor Chip ID"), F("vendor_chip_id")),
        core_version(F("Core version"), F("core_version")),
        boot_version(F("Boot version"), F("boot_version")),
        sdk_version(F("SDK version"), F("sdk_version")),
        cpu_frequency(F("CPU frequency"), F("cpu_frequency"))
    {
        initialize({}, {&firmware_name, &compile_datetime, &architecture,
            &device_chip_id,
            &flash_chip, &last_reset, &flash_size, &real_flash_size,
            &sketch_size, &vendor_chip_id,
            &core_version, &boot_version, &sdk_version, &cpu_frequency});

        architecture.set(F("esp8266"));
        compile_datetime.set(F(__DATE__ " " __TIME__));
        device_chip_id.set(String(ESP.getChipId(), 16));
        flash_chip.set(String(ESP.getFlashChipId(), 16));
        last_reset.set(ESP.getResetInfo());
        flash_size.set(String(ESP.getFlashChipSize()));
        real_flash_size.set(String(ESP.getFlashChipRealSize()));

        auto sketchSize = ESP.getSketchSize();
        auto totalSketchSpace = sketchSize + ESP.getFreeSketchSpace();
        static const char of_str[] PROGMEM = " of ";
        static const char bytes_str[] PROGMEM = " bytes";
        char sketch_size_string[sizeof(of_str) + sizeof(bytes_str) + 22];   // allocated 11 digits for sketch size and total size.
        itoa(sketchSize, sketch_size_string, 10);
        strcat_P(sketch_size_string, of_str);
        itoa(totalSketchSpace, sketch_size_string + strlen(sketch_size_string), 10);
        strcat_P(sketch_size_string, bytes_str);
        sketch_size.set(sketch_size_string);

        vendor_chip_id.set(String(ESP.getFlashChipVendorId(), 16));
        core_version.set(ESP.getCoreVersion());
        boot_version.set(String(ESP.getBootVersion()));
        sdk_version.set(ESP.getSdkVersion());

        static const char mhz[] PROGMEM = " MHz";
        char frequency_string[3 + sizeof(mhz)];
        itoa(ESP.getCpuFreqMHz(), frequency_string, 10);
        strcat_P(frequency_string, mhz);
        cpu_frequency.set(frequency_string);
    }

    DynamicJsonDocument SystemDetailsDisplay::as_json() const
    {
        DynamicJsonDocument json(128 * get_settings().size());

        static const char enabled_string[] PROGMEM = "enabled";

        json[FPSTR(enabled_string)] = is_enabled();
        // Most values can simply be retrieved from settings. Exceptions are
        // sketch size and cpu frequency.
        for (const auto &setting: get_settings())
        {
            if (setting->send_to_ui() && setting != &enabled && setting != &sketch_size && setting != &cpu_frequency)
            {
                // This will invoke the request callback, which will update the contents as a side effect.
                json[setting->name()] = setting->as_string();
            }
        }

        static const char sketch_string[] PROGMEM = "sketch";
        static const char used_string[] PROGMEM = "size";
        static const char total_string[] PROGMEM = "total";
        DynamicJsonDocument sketch_json(512);
        sketch_json[FPSTR(used_string)] = ESP.getSketchSize();
        sketch_json[FPSTR(total_string)] = ESP.getFreeSketchSpace() + ESP.getSketchSize();
        json[FPSTR(sketch_string)] = std::move(sketch_json);
        json[cpu_frequency.name()] = ESP.getCpuFreqMHz();

        return json;
    }
}