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
#include <Wire.h>
#include <SHT31.h>

#include <algorithm>

#include "grmcdorman/device/Sht31Sensor.h"
#include "grmcdorman/Setting.h"

namespace grmcdorman::device
{
    namespace {
        // Defaults.
        constexpr int DEFAULT_ADDRESS = 0x44;       /// Default address. The device can be changed to 0x45 by a jumper.
        constexpr int DEFAULT_SDA = Device::D5;     /// Default SDA connection.
        constexpr int DEFAULT_SCL = Device::D6;     /// Default SCL connection.
        constexpr int address_map[2] = { 0x44, 0x45 };
        const char sht31_name[] PROGMEM = "SHT31-D";
        const char sht31_identifier[] PROGMEM = "sht31_d";
        const ExclusiveOptionSetting::names_list_t address_names{ FPSTR("0x44"), FPSTR("0x45")};

        class Sht31Device_Temperature_Definition: public Device::Definition
        {
            public:
                virtual const __FlashStringHelper *get_name_suffix() const override
                {
                    return F(" SHT31-D Temperature");
                }
                virtual const __FlashStringHelper *get_value_template() const override
                {
                    return F("{{value_json.sht31_d.temperature.average}}");
                }
                virtual const __FlashStringHelper *get_unique_id_suffix() const override
                {
                    return F("_sht31d_temperature");
                }
                virtual const __FlashStringHelper *get_unit_of_measurement() const override
                {
                    return F("°C");
                }
                virtual const __FlashStringHelper *get_json_attributes_template() const override
                {
                    return F("{\"last\": \"{{value_json.sht31_d.temperature.last}}\", \"age\": \"{{value_json.sht31_d.temperature.sample_age_ms}}\"}");
                }
                virtual const __FlashStringHelper *get_icon() const override
                {
                    return F("mdi:thermometer");
                }
        };
        class Sht31Device_Humidity_Definition: public Device::Definition
        {
            public:
                virtual const __FlashStringHelper *get_name_suffix() const override
                {
                    return F(" SHT31-D Humidity");
                }
                virtual const __FlashStringHelper *get_value_template() const override
                {
                    return F("{{value_json.sht31_d.humidity.average}}");
                }
                virtual const __FlashStringHelper *get_unique_id_suffix() const override
                {
                    return F("_sht31d_humidity");
                }
                virtual const __FlashStringHelper *get_unit_of_measurement() const override
                {
                    return F("%");
                }
                virtual const __FlashStringHelper *get_json_attributes_template() const override
                {
                    return F("{\"last\": \"{{value_json.sht31_d.humidity.last}}\", \"age\": \"{{value_json.sht31_d.humidity.sample_age_ms}}\"}");
                }
                virtual const __FlashStringHelper *get_icon() const override
                {
                    return F("mdi:water-percent");
                }
        };
    }

    Sht31Sensor::Sht31Sensor():
        AbstractTemperaturePressureSensor(FPSTR(sht31_name), FPSTR(sht31_identifier)),
        title(F("<h2>SHT31-D Temperature and Humidity Sensor</h2>")),
        dataPin(F("SDA (Data) Connection"), F("sda"), data_line_names),
        clockPin(F("SCL (Clock) Connection"), F("scl"), data_line_names),
        address(F("Sensor address"), F("address"), address_names),
        temperatureOffset(F("Temperature offset"), F("temperature_offset")),
        temperatureScale(F("Temperature Scale Factor"), F("temperature_scale")),
        humidityOffset(F("Humidity Offset"), F("humidity_offset")),
        humidityScale(F("Humidity Scale Factor"), F("humidity_scale")),
        readInterval(F("Polling interval (seconds)"), F("poll_interval")),
        device_status(F("Sensor status<script>periodicUpdateList.push(\"sht31_d&setting=device_status\");</script>"), F("device_status"))
    {
        static const Sht31Device_Temperature_Definition temperature_definition;
        static const Sht31Device_Humidity_Definition humidity_definition;

        initialize({&temperature_definition, &humidity_definition}, {&title, &dataPin, &clockPin, &address, &temperatureOffset, &temperatureScale, &humidityOffset, &humidityScale,
            &readInterval, &device_status, &enabled});

        dataPin.set(dataline_to_index(DEFAULT_SDA));
        clockPin.set(dataline_to_index(DEFAULT_SCL));
        address.set(0);
        temperatureOffset.set(0);
        temperatureScale.set(1);
        humidityOffset.set(0);
        humidityScale.set(1);
        readInterval.set(statusReadInterval / 1000);
        set_enabled(false);

        device_status.set_request_callback([this] (const InfoSettingHtml &)
        {
            if (!is_enabled())
            {
                device_status.set(F("Sensor is disabled"));
                return;
            }

            if (!available)
            {
                device_status.set(F("SHT31-D failed to start or is not connected, or was disabled at boot."));
                return;
            }

            device_status.set(get_status());
        });
    }

    void Sht31Sensor::setup()
    {
        if (!is_enabled())
        {
            return;
        }

        wire.begin(index_to_dataline(dataPin.get()), index_to_dataline(clockPin.get()));
        sht = new SHT31(address_map[address.get()], &wire);
        if (!sht->begin())
        {
            return;
        }
        wire.setClock(100000);
        if (!sht->isConnected())
        {
            return;
        }

        available = true;
        set_timer();
        sht->requestData();                // request for next sample
        requested = true;
    }

    void Sht31Sensor::loop()
    {
        if (!available || !is_enabled())
        {
            return;
        }

        if (requested && sht->dataReady())
        {
            bool success  = sht->readData();   // default = true = fast
            if (success)
            {
                last_read_millis = millis();
                temperature.new_reading(sht->getTemperature() * temperatureScale.get() + temperatureOffset.get());
                humidity.new_reading(sht->getHumidity() * humidityScale.get() + humidityOffset.get());
                clear_is_published();
            }

            requested = false;
        }
    }

    String Sht31Sensor::get_status() const
    {
        if (!is_enabled() || !available)
        {
            return String();
        }

        String message;
        message.reserve(150);

        if (temperature.has_accumulation())
        {
            char float_str[64];
            dtostrf(temperature.get_last_reading(), 1, 1, float_str);
            message = float_str;
            message += F(" °C, ");
            dtostrf(humidity.get_last_reading(), 1, 1, float_str);
            message += float_str;
            message += F("% R.H.; ");
            auto since = millis() - last_read_millis;
            message += since / 1000;
            message += F(" seconds since last reading.");
        }
        else
        {
            message += F("No readings have been performed.");
        }

        return message;
    }

    void Sht31Sensor::set_timer()
    {
        current_polling_seconds = readInterval.get();
        ticker.attach_scheduled(current_polling_seconds, [this] {
            if (!requested)
            {
                statusReadPreviousMillis = millis();
                sht->requestData();                // request for next sample
                requested = true;
            }
        });

    }
}