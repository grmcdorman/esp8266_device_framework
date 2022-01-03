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

#include <SHT31.h>
#include <Ticker.h>

#include "grmcdorman/device/AbstractTemperaturePressureSensor.h"

namespace grmcdorman::device
{
    /**
     * @brief This class supports the SHT31-D sensor as a device.
     *
     * The device provides humidity and temperature. Readings are published
     * as the average of all readings made since the last publish.
     */
    class Sht31Sensor: public AbstractTemperaturePressureSensor
    {
        public:
            Sht31Sensor();

            void setup() override;
            void loop() override;

            /**
             * @brief Get a status report.
             *
             * This is also used for the `device_status` info message, with the exception
             * of various disabled states.
             *
             * When the SHT31Sensor is disabled or nonoperational,
             * this returns an empty string.
             *
             * @return String containing status report.
             */
            virtual String get_status() const;

        private:
            void set_timer();                   //!< Set up ticker timer.

            SHT31 sht;
            Ticker ticker;                      //!< Ticker to handle readings.
            uint32_t current_polling_seconds = 0;//!< Current polling interval.

            uint32_t last_read_millis;          //!< Timestamp of last read.
            bool requested = false;             //!< Whether a reading was requested.
            bool available = false;             //!< Whether the device is available.
            uint32_t statusReadPreviousMillis = 0;  // The time since the last read.
            //!< Default read interval. Chosen such that there should be 5 readings per 30 seconds.
            constexpr static uint32_t statusReadInterval = (30 / 5) * 1000;
            NoteSetting title;                  //!< Device tab title.
            ExclusiveOptionSetting dataPin;     //!< Data pin (SDA) configuration.
            ExclusiveOptionSetting clockPin;    //!< Clock pin (SCL) configuration.
            ExclusiveOptionSetting address;     //!< Address configuration; either 0x44 or 0x45.
            FloatSetting temperatureOffset;     //!< Offset applied to temperature readings.
            FloatSetting temperatureScale;      //!< Scaling applied to temperature readings.
            FloatSetting humidityOffset;        //!< Offset applied to humidity readings.
            FloatSetting humidityScale;         //!< Scaling applied to humidity readings.
            UnsignedIntegerSetting readInterval;//!< How often to request a reading.
            InfoSettingHtml device_status;        //!< Last update
    };
}