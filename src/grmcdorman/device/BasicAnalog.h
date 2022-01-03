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

#include "grmcdorman/device/AbstractAnalog.h"

namespace grmcdorman::device
{
    /**
     * @brief A basic analog device.
     *
     * This device reads A0; the value can be transformed by
     * a scale and an offset before reporting. The value can also
     * be inverted (i.e. 1/value).
     *
     * This can be used for devices like potentiometers or photoresistors; anything
     * where the input is linear. Note that thermistors do not have a linear response;
     * there is a seperate class - `ThermistorSensor` for handling them.
     *
     * Appropriate scaling and offsets can be applied, possibly with inversion, to
     * convert the raw reading to a range of interest.
     *
     * The raw reading has a range of 0 to 1024 on ESP8266. The maximum value
     * may correspond to 1V, or 3.3V on the A0 (ADC) pin. You can determine this maximum
     * by using a potentiometer connected across the +3.3V and GND, with the middle connection
     * to the A0. If the reading reaches the maximum (1023 or 1024) when the potentiometer
     * is only about 1/3 turned and remains at that value for the rest of the range, then your ADC has a maximum
     * value of 1.0V.
     *
     * @note The units passed in the constructor are only obeyed for the first BasicAnalog device;
     * all other BasicAnalog devices get the same units. This is probably not a problem, as the
     * ESP8266 has only one ADC anyway.
     *
     * If the application requires fixed scaling, pass `false` as the second parameter to the
     * constructor so that the scaling cannot be changed.
     */
    class BasicAnalog: public AbstractAnalog
    {
        public:
            /**
             * @brief Construct a new Basic Analog object.
             *
             * @param units             Units. Required for publishing to MQTT/Home Assistant.
             * @param allowUserAdjust   If `true`, the user can adust scaling/offset/inversion in the UI, and the values are saved.
             * @param defaultScale      Default scaling value.
             * @param defaultOffset     Default offset value.
             * @param invert            If true, invert the reading before applying scale and offset.
             */
            explicit BasicAnalog(const __FlashStringHelper *units, bool allowUserAdjust,
                float defaultScale = 1.0f, float defaultOffset = 0.0f, bool invert = false);


            /**
             * @brief Get a status report.
             *
             * This is also used for the `device_status` info message, with the exception
             * of various disabled states.
             *
             * @return String containing status report.
             */
            virtual String get_status() const;
        protected:
            virtual float transform_raw_reading(int reading) override
            {
                return reading;
            };
            NoteSetting title;                  //!< Device tab title.
            InfoSettingHtml device_status;      //!< Last update
    };
}