
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
        flash_chip(F("Flash Chip"), F("flash_chip")),
        last_reset(F("Last reset reason"), F("last_reset")),
        flash_size(F("Flash memory size"), F("flash_size")),
        real_flash_size(F("Real flash size"), F("real_flash_size")),
        sketch_size(F("Sketch space"), F("sketch_size")),
        chipid(F("Chip ID"), F("chipid")),
        core_version(F("Core version"), F("core_version")),
        boot_version(F("Boot version"), F("boot_version")),
        sdk_version(F("SDK version"), F("sdk_version")),
        cpu_frequency(F("CPU frequency"), F("cpu_frequency"))
    {
        initialize({}, {&architecture, &flash_chip, &last_reset, &flash_size, &real_flash_size, &sketch_size, &chipid,
            &core_version, &boot_version, &sdk_version, &cpu_frequency});
        architecture.set(F("esp8266"));
        compile_datetime.set(F(__DATE__ " " __TIME__));
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

        chipid.set(String(ESP.getFlashChipVendorId(), 16));
        core_version.set(ESP.getCoreVersion());
        boot_version.set(String(ESP.getBootVersion()));
        sdk_version.set(ESP.getSdkVersion());

        static const char mhz[] PROGMEM = " MHz";
        char frequency_string[3 + sizeof(mhz)];
        itoa(ESP.getCpuFreqMHz(), frequency_string, 10);
        strcat_P(frequency_string, mhz);
        cpu_frequency.set(frequency_string);
    }
}