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

#include <WString.h>
#include <ArduinoJson.h>

#include <algorithm>
#include <numeric>
#include <vector>

namespace grmcdorman::device
{

        /**
         * @brief A class to handle accumulating values.
         *
         * This provides tracking for a reading, giving both the
         * most recent reading and the current running average.
         *
         * The number of readings required for a running average is configurable,
         * and is not time-based. It defaults to 10 readings.
         *
         * The intent is that readings are accumulated until a
         * reading is published, at which time `reset` is called
         * to begin accumulating anew.
         *
         * @tparam T        The type being accumulated; must be an arithmetic type, e.g. int32_t or float.
         * @tparam unset    The unset value. Defaults to `T()`, which is likely to be zero.
         * @tparam zero     The zero value; defaults to 0.
         */
        template<typename T, uint8_t N, int unset = 0, int zero = 0>
        class Accumulator
        {
        public:
            static constexpr T unset_value = unset;     //!< The unset value.
            static constexpr T zero_value = zero;       //!< The zero value.
            static constexpr uint8_t average_points = N;//!< The number of readings for the rolling average.
            typedef T value_type;                       //!< The value type.

            /**
             * @brief Get the current value.
             *
             * This is the last value supplied to `new_reading`. If
             * a value has never been read, the returned value will be
             * `unset_value`.
             *
             * @return Last reading
             */
            T get_last_reading() const
            {
                return last_reading;
            }
            /**
             * @brief Get the current rolling average.
             *
             * This is the rolling average for the last N readings;
             * if fewer than N have been accumulated, it is the average
             * of those.
             *
             * @return Current average.
             */
            float get_current_average() const
            {
                uint8_t count = std::min(data_read_first, average_points);
                return count == 0 ? unset_value : std::accumulate(&last_reading_set[0], &last_reading_set[count], zero_value) / static_cast<float>(count);
            }
            /**
             * @brief Record a new reading.
             *
             * The `last_reading` is set to this value,
             * and the reading is accumulated in the rolling average set.
             *
             * @param new_value New reading.
             */
            void new_reading(T new_value)
            {
                last_reading = new_value;
                last_reading_set[current_index] = new_value;
                current_index = (current_index + 1) % N;
                if (data_read_first < N)
                {
                    ++data_read_first;
                }
                last_sample_time = millis();
            }
            /**
             * @brief Return a value indicate whether data has been accumulated.
             *
             * @return true     At least one reading has been accumulated.
             * @return false    No readings have been accumulated.
             */
            bool has_accumulation() const
            {
                return data_read_first > 0;
            }

            /**
             * @brief Get the number of samples used for the average.
             *
             * At first this will be less than the configured maximum until
             * sufficient samples have been collected.
             * @return Sample count.
             */
            uint8_t get_sample_count() const
            {
                return data_read_first;
            }

            /**
             * @brief Get the last sample age, in milliseconds.
             *
             * @return Last sample age.
             */
            uint32_t get_last_sample_age() const
            {
                return millis() - last_sample_time;
            }

            /**
             * @brief Get the values in standard JSON.
             *
             * @return DynamicJsonDocument containing values.
             */
            DynamicJsonDocument as_json() const
            {
                static const char average_string[] PROGMEM = "average";
                static const char last_string[] PROGMEM = "last";
                static const char sample_count_string[] PROGMEM = "sample_count";
                static const char sample_age_string[] PROGMEM = "sample_age_ms";

                DynamicJsonDocument json(128);
                json[FPSTR(average_string)] = get_current_average();
                json[FPSTR(last_string)] = get_last_reading();
                json[FPSTR(sample_count_string)] = get_sample_count();
                json[FPSTR(sample_age_string)] = get_last_sample_age();
                return json;
            }

        private:
            T last_reading = unset_value;           //!< The last reading.
            T last_reading_set[N];                  //!< The last <N> readings.
            uint32_t current_index = 0;             //!< The number of readings since the last reset.
            uint8_t data_read_first = 0;            //!< The number of points in the first set; no more than N.
            uint32_t last_sample_time = 0;          //!< The last sample time.
        };
}
