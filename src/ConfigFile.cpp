#include <ArduinoJson.h>
#include <LittleFS.h>

#include "grmcdorman/device/ConfigFile.h"
#include "grmcdorman/device/Device.h"

namespace grmcdorman::device
{
    void ConfigFile::save(const std::vector<Device *> &devices)
    {
        DynamicJsonDocument json(4096);
        bool set_a_device = false;
        for (auto &device: devices)
        {
            if (strlen_P(reinterpret_cast<const char *>(device->identifier())) != 0 && !device->get_settings().empty())
            {
                DynamicJsonDocument deviceJson(2048);
                bool set_setting = false;
                for (auto &setting: device->get_settings())
                {
                    if (strlen_P(reinterpret_cast<const char *>(setting->name())) != 0 && setting->is_persistable())
                    {
                        deviceJson[setting->name()] = setting->as_string();
                        set_a_device = true;
                        set_setting = true;
                    }
                }
                if (set_setting)
                {
                    json[device->identifier()] = deviceJson;
                }
            }
        }
        if (set_a_device)
        {
            save(json);
        }
    }

    bool ConfigFile::load(const std::vector<Device *> &devices)
    {
        auto json = load();
        if (json)
        {
            for (auto &device : devices)
            {
                auto deviceJson = (*json)[device->identifier()];
                for (auto &setting: device->get_settings())
                {
                    if (!deviceJson[setting->name()].isNull())
                    {
                        setting->set_from_string(deviceJson[setting->name()]);
                    }
                }
            }
        }

        return json.has_value();
    }

    void ConfigFile::save(DynamicJsonDocument &json)
    {
        File configFile = LittleFS.open(get_path(), "w");
        if (!configFile) {
            return;
        }

        serializeJson(json, configFile);
        configFile.close();
    }

    std::optional<DynamicJsonDocument> ConfigFile::load()
    {
        if (!LittleFS.begin()) {
            return std::nullopt;
        }

        if (!LittleFS.exists(get_path())) {
            return std::nullopt;
        }

        File configFile = LittleFS.open(get_path(), "r");

        if (!configFile) {
            return std::nullopt;
        }

        DynamicJsonDocument json(configFile.size() * 10);

        auto status = deserializeJson(json, configFile);
        if (DeserializationError::Ok != status) {
            Serial.print(F("Deserialization error: "));
            Serial.println(status.c_str());
            return std::nullopt;
        }
        return json;
    }
}
