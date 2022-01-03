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

#include <Arduino.h>
#include <Ticker.h>

#include "grmcdorman/device/Device.h"
#include "grmcdorman/device/Accumulator.h"

namespace grmcdorman::device
{
    /**
     * @brief Abstract analog device.
     *
     * This device reads A0; the value can be transformed by
     * a scale and an offset before reporting. The value can also
     * be inverted (i.e. 1/value).
     *
     * This device doesn't provide full services; it's intended to
     * be derived in other classes that can transform the raw
     * value into meaningful output.
     *
     */
    class AbstractAnalog: public Device
    {
        public:
            /**
             * @brief Construct a new Basic Analog object.
             *
             * @param device_name       Device name.
             * @param device_identifier Device identifier.
             * @param defaultScale      Default scaling value.
             * @param defaultOffset     Default offset value.
             * @param invert            If true, invert the reading before applying scale and offset.
             */
            explicit AbstractAnalog(const __FlashStringHelper *device_name, const __FlashStringHelper *device_identifier,
                float defaultScale = 1.0f, float defaultOffset = 0.0f, bool invert = false);

            void setup() override;
            void loop() override;
            bool publish(DynamicJsonDocument &json) const override;

            /**
             * @brief Last computed reading.
             *
             * @return float
             */
            float get_last_reading() const
            {
                return sensor_reading.get_last_reading();
            }

            /**
             * @brief Last raw value (no scale or offset applied).
             *
             * @return float
             */
            float raw_value() const
            {
                return last_raw_value;
            }

            /**
             * @brief Get the last average reading.
             *
             * Only valid after a call to `reset_accumulation`
             * that returns `true`.
             *
             * @return float
             */
            float get_current_average() const
            {
                return sensor_reading.get_current_average();
            }

            DynamicJsonDocument as_json() const override;
        protected:
            /**
             * @brief Transform the raw reading into the reported value.
             *
             * This can, for example, transform the raw A0 value into
             * volts, or transform a thermistor value in to degrees Celcius.
             *
             * Do not apply the scaling, offset, and invert values in this method;
             * they will be automatically applied on the returned value.
             * @param reading   The raw reading.
             * @return float    The transformed reading.
             */
            virtual float transform_raw_reading(int reading) = 0;
            FloatSetting scale;                 //!< Offset.
            FloatSetting offset;                //!< Scaling.
            ToggleSetting invertReading;        //!< Whether to invert the reading.
            UnsignedIntegerSetting readInterval;//!< How often to request a reading.
            uint32_t last_read_millis = 0;      //!< Timestamp of last read.
            //!< Default read interval. Chosen such that there should be 5 readings per 30 seconds.
            constexpr static uint32_t statusReadInterval = (30 / 5) * 1000;
            Accumulator<float, 5> sensor_reading;  //!< Reading.
        private:
            void set_timer();                   //!< Set up ticker timer.
            Ticker ticker;                      //!< Ticker to handle readings.
            uint32_t current_polling_seconds = 0;//!< Current polling interval.
            float last_raw_value = 0;           //!< Last raw value.
    };
}