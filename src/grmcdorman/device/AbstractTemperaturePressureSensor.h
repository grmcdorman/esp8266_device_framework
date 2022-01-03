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

#pragma once

#include "grmcdorman/device/Device.h"
#include "grmcdorman/device/Accumulator.h"

namespace grmcdorman::device
{
    /**
     * @brief This is an abstract base class for temperature/pressor sensors.
     *
     * This provides support for devices that publish humidity and temperature.
     * It provides the common functionality to publish averages of these two
     * readings.
     */
    class AbstractTemperaturePressureSensor: public Device
    {
        public:
            /**
             * @brief Construct a new Abstract Temperature Pressure Sensor object.
             *
             * @param device_name       Device name.
             * @param device_identifier Device identifier.
             */
            AbstractTemperaturePressureSensor(const __FlashStringHelper *device_name, const __FlashStringHelper *device_identifier):
                Device(device_name, device_identifier)
            {
            }
            DynamicJsonDocument as_json() const override
            {
                static const char temperature_string[] PROGMEM = "temperature";
                static const char humidity_string[] PROGMEM = "humidity";
                static const char enabled_string[] PROGMEM = "enabled";
                DynamicJsonDocument json(1024);

                json[FPSTR(enabled_string)] = is_enabled();
                json[FPSTR(temperature_string)] = std::move(temperature.as_json());
                json[FPSTR(humidity_string)] = std::move(humidity.as_json());

                return json;
            }

            bool publish(DynamicJsonDocument &json) const
            {
                if (!temperature.has_accumulation() || !is_enabled())
                {
                    return false;
                }

                json[identifier()] = as_json();

                return true;
            }

            /**
             * @brief Get the last temperature reading.
             *
             * @return Last temperature reading; INVALID_READING if never read.
             */
            float get_temperature() const
            {
                return temperature.get_last_reading();
            }

            /**
             * @brief Get the last humidity reading.
             *
             * @return Last humidity reading; INVALID_READING if never read.
             */
            float get_humidity() const
            {
                return humidity.get_last_reading();
            }

            static constexpr int INVALID_READING = -273;        //!< The invalid reading value.
        protected:
            Accumulator<float, 5, INVALID_READING> temperature;    //!< The temperature accumulator.
            Accumulator<float, 5, INVALID_READING> humidity;       //!< The humidity accumulator.
    };
}