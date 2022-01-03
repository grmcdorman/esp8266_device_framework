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
                if (deviceJson.isNull())
                {
                    deviceJson = (*json)[device->name()];
                }
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
