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

#include "grmcdorman/device/BasicAnalog.h"
#include "grmcdorman/Setting.h"

namespace grmcdorman::device
{
    namespace {
        // Defaults.
        const char basic_analog_name[] PROGMEM = "Basic Analog Reading";
        const char basic_analog_identifier[] PROGMEM = "basic_analog";

        class BasicAnalog_Definition: public Device::Definition
        {
            public:
                BasicAnalog_Definition(const __FlashStringHelper *units): units(units)
                {
                }

                virtual const __FlashStringHelper *get_name_suffix() const override
                {
                    return F(" Analog Reading");
                }
                virtual const __FlashStringHelper *get_value_template() const override
                {
                    return F("{{value_json.basic_analog.average}}");
                }
                virtual const __FlashStringHelper *get_unique_id_suffix() const override
                {
                    return F("_basic_analog");
                }
                virtual const __FlashStringHelper *get_unit_of_measurement() const override
                {
                    return units;
                }
                virtual const __FlashStringHelper *get_json_attributes_template() const override
                {
                    return F("{\"last\": \"{{value_json.basic_analog.last}}\", \"age\": \"{{value_json.basic_analog.sample_age_ms}}\"}");
                }
                virtual const __FlashStringHelper *get_icon() const override
                {
                    return F("mdi:alpha-s-circle");
                }

            private:
                const __FlashStringHelper *units;
        };
    }

    BasicAnalog::BasicAnalog(const __FlashStringHelper *units, bool allowUserAdjust, float defaultScale, float defaultOffset, bool invert):
        AbstractAnalog(FPSTR(basic_analog_name), FPSTR(basic_analog_identifier), defaultScale, defaultOffset, invert),
        title(F("<h2>Analog Data Line Reading (A0 input)</h2>")),
        device_status(F("Sensor status<script>periodicUpdateList.push(\"basic_analog&setting=device_status\");</script>"), F("device_status"))
    {
        static const BasicAnalog_Definition definition(units);

        if (allowUserAdjust)
        {
            initialize({&definition}, {&title, &scale, &offset, &invertReading,
                &readInterval, &device_status, &enabled});
        }
        else
        {
            initialize({&definition}, {&title,
                &readInterval, &device_status, &enabled});
        }

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


    String BasicAnalog::get_status() const
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
        message += ';';
        message += ' ';
        auto since = millis() - last_read_millis;
        message += since / 1000;
        message += F(" seconds since last reading.");

        return message;
    }
}