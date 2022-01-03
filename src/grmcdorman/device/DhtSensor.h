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

#include <memory>

#include <DHT.h>
#include <Ticker.h>

#include "grmcdorman/device/AbstractTemperaturePressureSensor.h"

namespace grmcdorman::device
{
    /**
     * @brief This class supports the DHT11 and DHT22 sensors as a device.
     *
     * The specific sensor is selected at run-time by the "dht_model" setting.
     * The sensor provides humidity and temperature. Readings are published
     * as the average of all readings made since the last publish.
     *
     * This uses Bert Melis' https://github.com/bertmelis/DHT, an interrupt-driven handler,
     * to perform the readings.
     *
     * For four-pin DHT packages, a 5K or 10K pull-up resistor connected to VDD (typically +5V)
     * is required on the signal pin (pin 2). Pin 1 is VDD, and pin 4 is GND. Pins are numbered
     * from left to right when viewing the front (performated) side of the sensor.
     *
     * The minimum read interval for DHT11 is 1 second; for DHT22 is 2 seconds. The
     * settings do not enforce this minimum.
     */
    class DhtSensor: public AbstractTemperaturePressureSensor
    {
        public:
            DhtSensor();

            void setup() override;
            void loop() override;

            /**
             * @brief Get a status report.
             *
             * This is also used for the `device_status` info message, with the exception
             * of various disabled states.
             *
             * When the DhtSensor is disabled or nonoperational,
             * this returns an empty string.
             *
             * @return String containing status report.
             */
            virtual String get_status() const;
        private:
            //!< Default read interval. Chosen such that there should be 5 readings per 30 seconds.
            constexpr static uint32_t statusReadInterval = (30 / 5) * 1000;

            void set_timer();                           //!< Set the timer callback.
            void reset_dht();                           //!< Reset DHT on the first read request following an error.

            std::unique_ptr<DHT> dht;                   //!< Ether DHT11 or DHT22, depending on configuration.
            Ticker ticker;                              //<! Ticker used to schedule readings.
            int last_status = 0;                        //<! Last reported error status.
            uint32_t last_read_millis = 0;              //<! Last read millis().
            uint32_t current_polling_seconds = 0;       //<! Current polling interval.
            bool requested = false;                     //!< Whether a reading was requested.
            uint32_t request_previous_mills = 0;        //!< The time since the last read request.
            NoteSetting title;                          //!< Device tab title.
            ExclusiveOptionSetting dataPin;             //!< Data pin configuration.
            ExclusiveOptionSetting dhtModel;            //!< Choice of DHT11 or DHT22.
            FloatSetting temperatureOffset;             //!< Offset applied to temperature readings.
            FloatSetting temperatureScale;              //!< Scaling applied to temperature readings.
            FloatSetting humidityOffset;                //!< Offset applied to humidity readings.
            FloatSetting humidityScale;                 //!< Scaling applied to humidity readings.
            UnsignedIntegerSetting readInterval;        //!< How often to request a reading.
            InfoSettingHtml device_status;              //!< Last update
    };
}