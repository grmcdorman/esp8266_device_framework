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

#include <SoftwareSerial.h>

#include "grmcdorman/device/Device.h"
#include "grmcdorman/device/Accumulator.h"

namespace grmcdorman::device
{
    /**
     * @brief This class supports the Ikea Vindriktning air quality sensor as a device.
     *
     * The Ikea device provides +5V power; in addition, one of the pads can be connected to an
     * appropriate data pin (default D2) to monitor readings. This class will automatically
     * monitor the connection for reading messages and accumulating readings, and will publish the
     * average reading.
     *
     * The serial message sent by the Vindriktning is 20 bytes in length; the first three bytes
     * are 0x16, 0x11, and 0x0B. Bytes 5 and 6 contain a 16 bit sensor reading. The buffer also
     * contains checksum bytes such that the sum of all bytes in the buffer is zero. Other bytes
     * in the buffer are not currently documented here.
     *
     * The maximum reading is around 500 ug/m^3; that means for the 32-bit sum used, which
     * can hold up to 4,294,967,295, the maximum number of readings at that level is approximately 8,589,934.
     * (If you do get readings that high, run for the hills.)
     *
     * Readings appear to occur every 20 to 30 seconds; that means the summation counter (`pm25_sum`)
     * will not overflow in any reasonable publishing interval (8,589,934 readings times 20 seconds
     * is 171,798,691 seconds, over a year, which is 31,557,988 seconds).
     *
     * When a valid reading is discovered in the received Vindriktning data, the reading is added to the sum
     * and all buffered data up to and including the processed message is discarded. Any following
     * data is retained.
     *
     * Derived from work by Hypfer's GitHub project, https://github.com/Hypfer/esp8266-vindriktning-particle-sensor.
     * All work on message deciphering comes from that project.
     */
    class VindriktningAirQuality: public Device
    {
        public:
            VindriktningAirQuality();

            void setup() override;
            void loop() override;
            bool publish(DynamicJsonDocument &json) const override;
            DynamicJsonDocument as_json() const override;

            /**
             * @brief Get a status report.
             *
             * This is also used for the `last_update` info message, with the exception
             * of various disabled states.
             *
             * When the VindriktningAirQuality is disabled this returns an empty string.
             *
             * @return String containing status report.
             */
            virtual String get_status() const;

            /**
             * @brief Get the last PM 2.5 reading.
             *
             * Note that this is not the value published to MQTT.
             *
             * This may be zero if never read.
             * @return PM 2.5 reading.
             */
            uint16_t get_pm25() const
            {
                return pm25.get_last_reading();
            }
        private:
            static constexpr uint16_t vindriktning_message_size = 20;
            /**
             * @brief Parse the serial data and extract the reading.
             *
             * This extracts the reading from the serial data and adds it to the sum.
             *
             * The provided data may be at some offset in the serial buffer.
             * @param data  Data from the Vindriktning; should be 20 bytes in length.
             */
            void parse(const uint8_t *data);
            /**
             * @brief Verify the data checksum.
             *
             * Verify that the checksum in the 20-byte data is valid (i.e. sums to zero)
             * @param data  Data from the Vindriktning; should be 20 bytes in length.
             * @return true if the checksum is valid.
             */
            bool is_valid_checksum(const uint8_t *data) const;
            /**
             * @brief Find the message in read data.
             *
             * This searches for a valid message - correct header and checksum - in
             * the raw data read.
             * @param data              The raw data.
             * @param byte_count        The number of bytes read.
             * @return A pointer to valid 20-byte message, or `nullptr` if no message can be located.
             */
            const uint8_t *find_message(const uint8_t *data, uint8_t byte_count) const;

            /**
             * @brief The current device state.
             *
             */
            enum class State
            {
                NEVER_READ,         //!< Never read anything.
                NO_HEADER_FOUND,    //!< Last read didn't find a header.
                READ                //!< Last read succeeded.
            };
            NoteSetting title;                        //!< The title for the device tab.
            ExclusiveOptionSetting serialDataPin;     //!< The serial data pin configuration.
            InfoSettingHtml device_status;             //!< The last read status.

            ::grmcdorman::SettingInterface::settings_list_t settings;   //!< The settings list

            SoftwareSerial sensorSerial;                    //!< The software serial reader, used to read from the device.
            uint32_t last_read_millis = 0;                  //!< The time of the last read.
            State last_read_state = State::NEVER_READ;      //!< The last read state.
            static constexpr uint16_t buffer_size = 2 * vindriktning_message_size;    //!< The buffer size. Do not set this high; high values need a lot of heap.
            uint8_t buffer[buffer_size];                    //!< Read buffer.
            uint32_t rxBufIdx = 0;                          //!< The current read index in the buffer.

            static constexpr uint8_t HEADER_BYTE_0 = 0x16;  //!< The value in the first byte of the message header.
            static constexpr uint8_t HEADER_BYTE_1 = 0x11;  //!< The value in the second byte of the message header.
            static constexpr uint8_t HEADER_BYTE_2 = 0x0B;  //!< The value in the third byte of the message header.

            Accumulator<uint32_t, 5> pm25;                  //!< The PM 2.5 readings.
    };
}