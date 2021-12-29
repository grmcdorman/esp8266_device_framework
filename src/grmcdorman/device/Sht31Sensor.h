#pragma once

#include <SHT31.h>

#include "grmcdorman/device/Device.h"
#include "grmcdorman/Setting.h"

namespace grmcdorman::device
{
    /**
     * @brief This class supports the SHT31-D sensor as a device.
     *
     * The device provides humidity and temperature. Readings are published
     * as the average of all readings made since the last publish.
     */
    class Sht31Sensor: public Device
    {
        public:
            Sht31Sensor();

            void setup() override;
            void loop() override;
            bool publish(DynamicJsonDocument &json) override;

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
            SHT31 sht;
            float temperature_sum = 0.0;        //!< The sum of all temperature readings since the last publish.
            float humidity_sum = 0.0;           //!< The sum of all humidity readings since the last publish.
            uint32_t reading_count = 0;         //!< The number of reads performed since the last publish.
            float last_temperature = -273;      //!< Last read temperature. Initialized to 0 Kelvin, which we are rather unlikely to see.
            float last_humidity = 0.0;          //!< Last read humidity.
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