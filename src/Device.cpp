#include "grmcdorman/device/Device.h"

#include <algorithm>
#include <array>
#include <cstdlib>

namespace grmcdorman::device
{
    static const char unspecified_firmware_name[] PROGMEM = "unspecified_firmware";

    const __FlashStringHelper *Device::firmware_name = FPSTR(unspecified_firmware_name);
    String Device::system_identifier = std::move([] {
        constexpr size_t unspecified_firmware_name_length = sizeof(unspecified_firmware_name) - 1;
        char name[unspecified_firmware_name_length + 1 + 9 + 1];
        memcpy_P(name, unspecified_firmware_name, unspecified_firmware_name_length);
        name[unspecified_firmware_name_length] = '-';
        itoa(ESP.getChipId(), &name[unspecified_firmware_name_length + 1], 16);
        return std::move(String(name));
    }());

    Device::Device(const __FlashStringHelper *device_name, const __FlashStringHelper *device_identifier):
        enabled(F("Enabled"), F("enabled")),
        device_name(device_name), device_identifier(device_identifier)
    {
        set_enabled(true);
    }

    void Device::set_system_identifiers(const __FlashStringHelper *firmware_name_value, const String &system_identifier_value)
    {
        firmware_name = firmware_name_value;
        if (system_identifier_value.isEmpty())
        {
            // Values: Firmware prefix length, one for '-', up to 9 for ChipID (32 bit unsigned), one for trailing NULL.
            system_identifier.reserve(strlen_P(reinterpret_cast<const char *>(firmware_name)) + 1 + 9 + 1);
            system_identifier = firmware_name;
            system_identifier += '-';
            char id[9];
            itoa(ESP.getChipId(), id, 16);
            system_identifier += id;
        }
        else
        {
            system_identifier = system_identifier_value;
        }
    }

    const ExclusiveOptionSetting::names_list_t Device::data_line_names{ FPSTR("D1"), FPSTR("D2"), FPSTR("D3"), FPSTR("D5"), FPSTR("D6"), FPSTR("D7")};

    const int Device::settingsMap[6] =
        {
            Device::D1, Device::D2, Device::D3, Device::D5, Device::D6, Device::D7
        };

    int Device::dataline_to_index(int index)
    {
        return std::min(static_cast<int>(std::find(&settingsMap[0], &settingsMap[6], index) - &settingsMap[0]), 5);
    }

}