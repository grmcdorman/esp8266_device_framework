#include "grmcdorman/device/VindriktningAirQuality.h"

namespace grmcdorman::device
{
    namespace {
        constexpr static const uint8_t DEFAULT_RX_PIN = Device::D2;
        constexpr static const uint32_t UART_SPEED = 9600;
        const char vindriktning_name[] PROGMEM = "Vindriktning";
        const char vindriktning_identifier[] PROGMEM = "vindriktning";
        class Vindriktning_Definition: public Device::Definition
        {
            public:
                virtual const __FlashStringHelper *get_name_suffix() const override
                {
                    return F(" PM 2.5");
                }
                virtual const __FlashStringHelper *get_value_template() const override
                {
                    return F("{{value_json.pm25}}");
                }
                virtual const __FlashStringHelper *get_unique_id_suffix() const override
                {
                    return F("_pm25");
                }
                virtual const __FlashStringHelper *get_unit_of_measurement() const override
                {
                    return F("μg/m³");
                }
                virtual const __FlashStringHelper *get_json_attributes_template() const override
                {
                    return nullptr;
                }
                virtual const __FlashStringHelper *get_icon() const override
                {
                    return F("mdi:air-filter");
                }
        };
    }

    VindriktningAirQuality::VindriktningAirQuality():
        Device(FPSTR(vindriktning_name), FPSTR(vindriktning_name)),
        title(F("<h2>Vindriktning Air Quality Sensor</h2>")),
        serialDataPin(F("Serial In (Data) Connection)"), F("serial_pin"), data_line_names),
        last_status(F("Last status<script>periodicUpdateList.push(\"vindriktning&setting=last_status\");</script>"), F("last_status")),
        sensorSerial()
    {
        static const Vindriktning_Definition vindriktning_definition;
        initialize({&vindriktning_definition}, {&title, &serialDataPin, &last_status, &enabled});

        serialDataPin.set(dataline_to_index(DEFAULT_RX_PIN));

        set_enabled(false);

        last_status.set_request_callback([this] (const InfoSettingHtml &)
        {
            if (!is_enabled())
            {
                last_status.set(F("Vindriktning is disabled"));
                return;
            }

            last_status.set(get_status());
        });
    }

    void VindriktningAirQuality::setup()
    {
        if (enabled.get())
        {
            sensorSerial.begin(UART_SPEED, SWSERIAL_8N1, index_to_dataline(serialDataPin.get()), -1, false, buffer_size);
        }
    }

    void VindriktningAirQuality::loop()
    {
        if (!enabled.get())
        {
            return;
        }

        // read serial; this will read all data available,
        // up to `buffer_reserved_size` bytes. The expected message is 20 bytes,
        // first three bytes are 0x16 0x11 0x0B
        if (!sensorSerial.available()) {
            return;
        }

        if (rxBufIdx >= buffer_size) {
            rxBufIdx = 0;
        }

        rxBufIdx += sensorSerial.read(&buffer[rxBufIdx], sizeof (buffer) - rxBufIdx);

        if (rxBufIdx >= vindriktning_message_size)
        {
            // Find the header, if possible.
            auto ptr = find_message(buffer, rxBufIdx);
            if (ptr != nullptr)
            {
                parse(ptr);
                // Move any later data in the buffer to the start, for the next cycle.
                auto offset = ptr - buffer + vindriktning_message_size;
                memmove(&buffer[0], &buffer[offset], rxBufIdx - offset);
                rxBufIdx -= offset;
            }
            else
            {
                last_read_state = State::NO_HEADER_FOUND;
            }
        }
    }

    const uint8_t *VindriktningAirQuality::find_message(const uint8_t *ptr, uint8_t byte_count) const
    {
        const uint8_t *candidate(ptr - 1);
        do
        {
            candidate = reinterpret_cast<uint8_t *>(memchr(candidate + 1, HEADER_BYTE_0, byte_count));
            if (candidate == nullptr)
            {
                return nullptr;
            }
            auto bytes_left = byte_count - (candidate - ptr);
            if (bytes_left < vindriktning_message_size)
            {
                return nullptr;
            }
        } while (candidate[1] != HEADER_BYTE_1 && candidate[2] != HEADER_BYTE_2 && is_valid_checksum(candidate));

        return candidate;
    }

    bool VindriktningAirQuality::is_valid_checksum(const uint8_t *data) const
    {
        uint8_t checksum = 0;

        for (uint8_t i = 0; i < vindriktning_message_size; i++) {
            checksum += data[i];
        }

        return checksum == 0;
    }

    void VindriktningAirQuality::parse(const uint8_t *data)
    {
        /**
         *         MSB  DF 3     DF 4  LSB
         * uint16_t = xxxxxxxx xxxxxxxx
         */
        last_pm25 = (data[5] << 8) | data[6];
        pm25_sum += last_pm25;
        ++pm25_count;
        last_read_millis = millis();
        last_read_state = State::READ;
    }

    bool VindriktningAirQuality::publish(DynamicJsonDocument &json)
    {
        if (!enabled.get() || pm25_count == 0)
        {
            return false;
        }

        json[F("pm25")] = pm25_sum / pm25_count;
        pm25_sum = 0;
        pm25_count = 0;

        return true;
    }

    String VindriktningAirQuality::get_status() const
    {
        String state_message;
        state_message.reserve(256);
        if (last_read_millis > 0)
        {
            state_message += last_pm25;
            state_message += F("µg/m³, ");
            state_message += (millis() - last_read_millis) / 1000;
            state_message += F(" seconds since last reading. ");
        }

        switch (last_read_state)
        {
            case State::NEVER_READ:
                state_message = F("Never got a reading.");
                break;

            case State::NO_HEADER_FOUND:
                state_message = F("Did not find a header in the last 20 bytes read.");
                break;

            case State::READ:
                // No message.
                break;

            default:
                state_message = F("Something went wrong.");
                break;
        }

        return state_message;
    }
}
