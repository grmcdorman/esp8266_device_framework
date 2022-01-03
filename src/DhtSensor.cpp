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
#include <DHT.h>
#include <Ticker.h>

#include <algorithm>

#include "grmcdorman/device/DhtSensor.h"
#include "grmcdorman/Setting.h"

namespace grmcdorman::device
{
    namespace {
        // Defaults.
        constexpr int DEFAULT_SDA = Device::D1;     /// Default SDA connection.
        const char dht_name[] PROGMEM = "DHT";
        const char dht_identifier[] PROGMEM = "dht";
        const ExclusiveOptionSetting::names_list_t dht_models{ FPSTR("DHT11"), FPSTR("DHT22")};

        class dhtDevice_Temperature_Definition: public Device::Definition
        {
            public:
                virtual const __FlashStringHelper *get_name_suffix() const override
                {
                    return F(" DHT Temperature");
                }
                virtual const __FlashStringHelper *get_value_template() const override
                {
                    return F("{{value_json.dht.temperature.average}}");
                }
                virtual const __FlashStringHelper *get_unique_id_suffix() const override
                {
                    return F("_dht_temperature");
                }
                virtual const __FlashStringHelper *get_unit_of_measurement() const override
                {
                    return F("°C");
                }
                virtual const __FlashStringHelper *get_json_attributes_template() const override
                {
                    return F("{\"last\": \"{{value_json.dht.temperature.last}}\", \"age\": \"{{value_json.dht.temperature.sample_age_ms}}\"}");
                }
                virtual const __FlashStringHelper *get_icon() const override
                {
                    return F("mdi:thermometer");
                }
        };
        class dhtDevice_Humidity_Definition: public Device::Definition
        {
            public:
                virtual const __FlashStringHelper *get_name_suffix() const override
                {
                    return F(" DHT Humidity");
                }
                virtual const __FlashStringHelper *get_value_template() const override
                {
                    return F("{{value_json.dht.humidity.average}}");
                }
                virtual const __FlashStringHelper *get_unique_id_suffix() const override
                {
                    return F("_dht_humidity");
                }
                virtual const __FlashStringHelper *get_unit_of_measurement() const override
                {
                    return F("%");
                }
                virtual const __FlashStringHelper *get_json_attributes_template() const override
                {
                    return F("{\"last\": \"{{value_json.dht.humidity.last}}\", \"age\": \"{{value_json.dht.humidity.sample_age_ms}}\"}");
                }
                virtual const __FlashStringHelper *get_icon() const override
                {
                    return F("mdi:water-percent");
                }
        };
    }

    DhtSensor::DhtSensor():
        AbstractTemperaturePressureSensor(FPSTR(dht_name), FPSTR(dht_identifier)),
        title(F("<h2>DHT11/DHT22 Temperature and Humidity Sensor</h2>")),
        dataPin(F("SDA (Data) Connection"), F("sda"), data_line_names),
        dhtModel(F("DHT model"), F("dht_model"), dht_models),
        temperatureOffset(F("Temperature offset"), F("temperature_offset")),
        temperatureScale(F("Temperature Scale Factor"), F("temperature_scale")),
        humidityOffset(F("Humidity Offset"), F("humidity_offset")),
        humidityScale(F("Humidity Scale Factor"), F("humidity_scale")),
        readInterval(F("Polling interval (seconds)"), F("poll_interval")),
        device_status(F("Sensor status<script>periodicUpdateList.push(\"dht&setting=device_status\");</script>"), F("device_status"))
    {
        static const dhtDevice_Temperature_Definition temperature_definition;
        static const dhtDevice_Humidity_Definition humidity_definition;

        initialize({&temperature_definition, &humidity_definition}, {&title, &dataPin, &dhtModel, &temperatureOffset, &temperatureScale, &humidityOffset, &humidityScale,
            &readInterval, &device_status, &enabled});

        dataPin.set(dataline_to_index(DEFAULT_SDA));
        dhtModel.set(0);    // DHT11

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

            device_status.set(get_status());
        });
    }

    void DhtSensor::setup()
    {
        if (!is_enabled())
        {
            return;
        }

        dht.reset(dhtModel.get() == 0 ? static_cast<DHT *>(new DHT11()) : static_cast<DHT *>(new DHT22()));

        reset_dht();
        set_timer();
    }

    void DhtSensor::reset_dht()
    {
        dht->setPin(index_to_dataline(dataPin.get()));
        dht->onData([this](float new_humidity, float new_temperature)
        {
            last_status = 0;
            schedule_function([this, new_humidity, new_temperature]() {
                last_read_millis = millis();
                temperature.new_reading(new_temperature* temperatureScale.get() + temperatureOffset.get());
                humidity.new_reading(new_humidity* humidityScale.get() + humidityOffset.get());
                clear_is_published();
                requested = false;
            });
        });
        dht->onError([this] (uint8_t status)
        {
            last_status = status;
            requested = false;
        });
    }

    void DhtSensor::loop()
    {
        if (!is_enabled())
        {
            if (dht)
            {
                ticker.detach();
                dht.reset();
            }
            return;
        }

        if (!dht)
        {
            setup();
        }

        if (current_polling_seconds != readInterval.get())
        {
            set_timer();
        }
    }

    String DhtSensor::get_status() const
    {
        if (!is_enabled())
        {
            return String();
        }

        String message;
        message.reserve(150);

        if (last_status != 0)
        {
            // DHT has a `getError method,
            // which returns a string based on
            // the internal status. Unfortunately:
            // a) This method has no way of translating a previous status.
            // b) The current status value is inaccessable.
            // The values in the switch statement below correspond to
            // the values in the DHT code. It is not clear what they mean.
            switch (last_status)
            {
                case 0:
                    // No error.
                    break;
                case 1:
                    message += F("DHT read timeout");
                    break;
                case 2:
                    message += F("DHT responded with a NACK");
                    break;
                case 3:
                    message += F("DHT data was invalid");
                    break;
                case 4:
                    message += F("DHT data had an invalid checksum");
                    break;
                default:
                    message += F("DHT reported an unknown error code: ");
                    message += last_status;
            }
            if (temperature.get_last_reading() != INVALID_READING)
            {
                message += F(";");
            }
            else
            {
                message += '.';
                return message;
            }
        }

        if (temperature.get_last_reading() != INVALID_READING)
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

    void DhtSensor::set_timer()
    {
        current_polling_seconds = readInterval.get();
        // A `detach` isn't necessary, this will automatically detach if required.
        ticker.attach_scheduled(current_polling_seconds, [this]
        {
            if (!requested)
            {
                requested = true;
                dht->read();
            }
        });
    }
}