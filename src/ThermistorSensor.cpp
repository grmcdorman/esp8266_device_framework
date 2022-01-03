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

#include "grmcdorman/device/ThermistorSensor.h"
#include "grmcdorman/Setting.h"

namespace grmcdorman::device
{
    namespace {
        // Defaults.
        const char thermistor_name[] PROGMEM = "Temperature";
        const char thermistor_identifier[] PROGMEM = "thermistor";

        class Thermistor_Definition: public Device::Definition
        {
            public:
                virtual const __FlashStringHelper *get_name_suffix() const override
                {
                    return F(" Temperature");
                }
                virtual const __FlashStringHelper *get_value_template() const override
                {
                    return F("{{value_json.thermistor.average}}");
                }
                virtual const __FlashStringHelper *get_unique_id_suffix() const override
                {
                    return F("_thermistor");
                }
                virtual const __FlashStringHelper *get_unit_of_measurement() const override
                {
                    return F("°C");
                }
                virtual const __FlashStringHelper *get_json_attributes_template() const override
                {
                    return F("{\"last\": \"{{value_json.thermistor.last}}\", \"age\": \"{{value_json.thermistor.sample_age_ms}}\"}");
                }
                virtual const __FlashStringHelper *get_icon() const override
                {
                    return F("mdi:thermometer");
                }
        };
    }

    ThermistorSensor::ThermistorSensor(float thermalIndex, float t1Kelvin):
        AbstractAnalog(FPSTR(thermistor_name), FPSTR(thermistor_identifier)),
        inverse_thermal_index(1.0 / thermalIndex),
        inverse_t1(1.0 / t1Kelvin),
        title(F("<h2>Temperature (ThermistorSensor)</h2>")),
        device_status(F("Sensor status<script>periodicUpdateList.push(\"thermistor&setting=device_status\");</script>"), F("device_status"))
    {
        static const Thermistor_Definition definition;

        initialize({&definition}, {&title, &scale, &offset,
            &readInterval, &device_status, &enabled});

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

    DynamicJsonDocument ThermistorSensor::as_json() const
    {
        static const char temperature_string[] PROGMEM = "temperature";
        static const char last_temperature_string[] PROGMEM = "last_temperature";
        static const char enabled_string[] PROGMEM = "enabled";
        DynamicJsonDocument json(512);

        json[FPSTR(enabled_string)] = is_enabled();
        json[FPSTR(temperature_string)] = get_current_average();
        json[FPSTR(last_temperature_string)] = get_last_reading();

        return json;
    }

    String ThermistorSensor::get_status() const
    {
        if (!is_enabled())
        {
            return String();
        }

        String message;
        message.reserve(150);
        char float_str[64];
        dtostrf(get_last_reading(), 1, 1, float_str);
        message = float_str;
        message += F("°C; ");
        auto since = millis() - last_read_millis;
        message += since / 1000;
        message += F(" seconds since last reading.");

        return message;
    }

    float ThermistorSensor::transform_raw_reading(int reading)
    {
        // Equations are from https://www.jameco.com/Jameco/workshop/TechTip/temperature-measurement-ntc-thermistors.html
        // Due to the requirements that:
        // a) the maximum ADC and the Vref supplied to the thermistor are the same;
        // b) the series resistor is sufficiently close to the thermistor at the temperatures being measured
        // the simplified equation can be used:
        //   1/T0 + 1/B * ln( ( adcMax / adcVal ) – 1 )
        float tempK_inverse = inverse_t1 + inverse_thermal_index * log(1023.0 / reading - 1);
        // Convert 1/K to degrees C.
        float tempC = 1 / tempK_inverse - 273.15;

        return tempC;
    }
}