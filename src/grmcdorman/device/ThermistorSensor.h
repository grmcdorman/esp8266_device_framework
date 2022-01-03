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
#include "grmcdorman/Setting.h"

namespace grmcdorman::device
{
    /**
     * @brief A thermistor analog device.
     *
     * This device reads A0 and transforms the reading
     * according to the standard thermistor properties
     * to get a reading in degrees Celcius.
     *
     * Scaling can be applied to the reading after conversion
     * to Celcius; this can be used to transform to other scales,
     * such as Fahrenheit, or to correct inaccuracies. To convert
     * to Fahrenheit, set the scale to 1.8 (9/5) and the offset to +32.
     *
     * The calculations make the following assumptions:
     * - The ADC maximum value, 1023, corresponds to the +V supplied to the thermistor; i.e. the reference voltage and ADC voltage source are the same.
     * - The resistor in series with the thermistor has a value close to the thermistor's resistance.
     * - The thermistor is connected between the GND and the resistor.
     *
     * In other words, if your ESP8266 reads 1023 or 1024 for +3.3V, and your thermistor has a nominal resistance of
     * 10K ohms at about 20 Celcius, then connect a 10K resistor in series with the thermistor between
     * +3.3V and GND, and connect A0 to the junction of the thermistor and the resistor. In rough ASCII art:
     * <pre>
     *  [3.3V]------[10K resistor]------[thermistor]------[GND]
     *                              |
     *                             A0
     * </pre>
     *
     * If your ADC does not have a maximum of 3.3V, then use a voltage divider to supply the ADC
     * maximum - probably 1V - to the resistor.

     * Thermistors have three parameters:
     * - Thermal index (or Beta)
     * - Calibration temperature, or T1, in Kelvin
     * - Nominal resistance at T1, R.
     *
     * For example, I have a thermistor with B of 3950, R of 10k, and T1 of 25C, or (273.15+25)=298.15K.
     *
     * Only the thermal index (beta) and T1 value are needed.
     *
     * When publishing to MQTT, only the last reading is published, not the average reading over the interval.
     */
    class ThermistorSensor: public AbstractAnalog
    {
        public:
            /**
             * @brief Construct a new ThermistorSensor object.
             *
             * @param thermalIndex          The thermal index of the thermistor, e.g. 3950.
             * @param t1Kelvin              The T1 temperature of the thermistor in Kelvin, e.g. 298.15 (25C).
             */
            ThermistorSensor(float thermalIndex, float t1Kelvin);

            DynamicJsonDocument as_json() const override;

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
            /**
             * @brief Transform the raw reading to a temperature.
             *
             * The parameters from the constructor, and the assumptions about the wiring,
             * are used to compute the temperature.
             *
             * @param reading   Raw reading.
             * @return Temperature, in degrees C.
             */
            virtual float transform_raw_reading(int reading) override;
            float inverse_thermal_index;        //!< The inverse of thermal index of the thermistor.
            float inverse_t1;                   //!< The inverse of the T1 temperature of the thermistor.
            NoteSetting title;                  //!< Device tab title.
            InfoSettingHtml device_status;      //!< Last update
    };
}