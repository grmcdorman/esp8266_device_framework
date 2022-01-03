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

#include "grmcdorman/device/AbstractAnalog.h"
#include "grmcdorman/Setting.h"

namespace grmcdorman::device
{

    AbstractAnalog::AbstractAnalog(const __FlashStringHelper *device_name, const __FlashStringHelper *device_identifier,
        float defaultScale, float defaultOffset, bool invert):
        Device(device_name, device_identifier),
        scale(F("Scaling"), F("scale")),
        offset(F("Offset"), F("offset")),
        invertReading(F("Invert reading before transform"), F("invert_reading")),
        readInterval(F("Polling interval (seconds)"), F("poll_interval"))
    {
        // Superclass will initialize.
        scale.set(defaultScale);
        offset.set(defaultOffset);
        invertReading.set(invert);
        readInterval.set(statusReadInterval / 1000);
    }

    void AbstractAnalog::setup()
    {
        if (!is_enabled())
        {
            return;
        }

        set_timer();
    }

    void AbstractAnalog::loop()
    {
        if (!is_enabled())
        {
            return;
        }

        if (current_polling_seconds != readInterval.get() || !ticker.active())
        {
            set_timer();
        }
    }

    bool AbstractAnalog::publish(DynamicJsonDocument &json) const
    {
        if (!sensor_reading.has_accumulation() || !is_enabled())
        {
            return false;
        }

        json[identifier()] = as_json();

        return true;
    }

    DynamicJsonDocument AbstractAnalog::as_json() const
    {
        DynamicJsonDocument json(512);
        static const char enabled_string[] PROGMEM = "enabled";
        json[FPSTR(enabled_string)] = is_enabled();
        json[identifier()] = sensor_reading.as_json();

        return json;
    }

    void AbstractAnalog::set_timer()
    {
        current_polling_seconds = readInterval.get();
        ticker.attach(current_polling_seconds, [this]
        {
            if (is_enabled())
            {
                last_read_millis = millis();
                last_raw_value = analogRead(A0);
                if (invertReading.get())
                {
                    sensor_reading.new_reading(scale.get() / transform_raw_reading(last_raw_value) + offset.get());
                }
                else
                {
                    sensor_reading.new_reading(scale.get() * transform_raw_reading(last_raw_value) + offset.get());
                }
                clear_is_published();
            }

        });
    }
}