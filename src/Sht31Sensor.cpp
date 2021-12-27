
#include <Arduino.h>
#include <Wire.h>
#include <SHT31.h>

#include <algorithm>

#include "grmcdorman/device/Sht31Sensor.h"
#include "grmcdorman/Setting.h"

namespace grmcdorman::device
{
    namespace {
        // Defaults.
        constexpr int DEFAULT_ADDRESS = 0x44;       /// Default address. The device can be changed to 0x45 by a jumper.
        constexpr int DEFAULT_SDA = Device::D5;     /// Default SDA connection.
        constexpr int DEFAULT_SCL = Device::D6;     /// Default SCL connection.
        constexpr int address_map[2] = { 0x44, 0x45 };
        const char sht31_name[] PROGMEM = "SHT31-D";
        const ExclusiveOptionSetting::names_list_t address_names{ FPSTR("0x44"), FPSTR("0x45")};

        class Sht31Device_Temperature_Definition: public Device::Definition
        {
            public:
                virtual const __FlashStringHelper *get_name_suffix() const override
                {
                    return F(" SHT31-D Temperature");
                }
                virtual const __FlashStringHelper *get_value_template() const override
                {
                    return F("{{value_json.sht31.temperature}}");
                }
                virtual const __FlashStringHelper *get_unique_id_suffix() const override
                {
                    return F("_sht31d_temperature");
                }
                virtual const __FlashStringHelper *get_unit_of_measurement() const override
                {
                    return F("°C");
                }
                virtual const __FlashStringHelper *get_json_attributes_template() const override
                {
                    return F("{{value_json.sht31.temperature}}");
                }
                virtual const __FlashStringHelper *get_icon() const override
                {
                    return F("mdi:thermometer");
                }
        };
        class Sht31Device_Humidity_Definition: public Device::Definition
        {
            public:
                virtual const __FlashStringHelper *get_name_suffix() const override
                {
                    return F(" SHT31-D Humidity");
                }
                virtual const __FlashStringHelper *get_value_template() const override
                {
                    return F("{{value_json.sht31.humidity}}");
                }
                virtual const __FlashStringHelper *get_unique_id_suffix() const override
                {
                    return F("_sht31d_humidity");
                }
                virtual const __FlashStringHelper *get_unit_of_measurement() const override
                {
                    return F("%");
                }
                virtual const __FlashStringHelper *get_json_attributes_template() const override
                {
                    return F("{{value_json.sht31.humidity}}");
                }
                virtual const __FlashStringHelper *get_icon() const override
                {
                    return F("mdi:water-percent");
                }
        };
    }

    Sht31Sensor::Sht31Sensor():
        Device(FPSTR(sht31_name), FPSTR(sht31_name)),
        title(F("<h2>SHT31-D Temperature and Humidity Sensor</h2>")),
        dataPin(F("SDA (Data) Connection"), F("sda"), data_line_names),
        clockPin(F("SCL (Clock) Connection"), F("scl"), data_line_names),
        address(F("Sensor address"), F("address"), address_names),
        temperatureOffset(F("Temperature offset"), F("temperature_offset")),
        temperatureScale(F("Temperature Scale Factor"), F("temperature_scale")),
        humidityOffset(F("Humidity Offset"), F("humidity_offset")),
        humidityScale(F("Humidity Scale Factor"), F("humidity_scale")),
        readInterval(F("Polling interval (seconds)"), F("poll_interval")),
        last_update(F("Last reading status<script>window.addEventListener(\"load\", () => { periodicUpdateList.push(\"SHT31-D&setting=Last_reading\"); });</script>"), F("Last_reading"))
    {
        static const Sht31Device_Temperature_Definition temperature_definition;
        static const Sht31Device_Humidity_Definition humidity_definition;

        initialize({&temperature_definition, &humidity_definition}, {&title, &dataPin, &clockPin, &address, &temperatureOffset, &temperatureScale, &humidityOffset, &humidityScale,
            &readInterval, &last_update, &enabled});

        dataPin.set(dataline_to_index(DEFAULT_SDA));
        clockPin.set(dataline_to_index(DEFAULT_SCL));
        address.set(0);
        temperatureOffset.set(0);
        temperatureScale.set(1);
        humidityOffset.set(0);
        humidityScale.set(1);
        readInterval.set(statusReadInterval / 1000);

        last_update.set_request_callback([this] (const InfoSettingHtml &)
        {
            if (!is_enabled())
            {
                last_update.set(F("Sensor is disabled"));
                return;
            }

            if (!available)
            {
                last_update.set(F("(SHT31-D failed to start or is not connected, or was disabled at boot."));
            }

            String message;
            if (reading_count > 0)
            {
                message += F("Accumulated ");
                message += reading_count;
                message += F(" readings; average temperature ");
                message += temperature_sum / reading_count;
                message += F(", average humidity ");
                message += humidity_sum /reading_count;
            }
            else
            {
                auto since = millis() - sht.lastRead();
                message = F("No readings have been performed; ");
                message += since / 1000;
                message += F(" seconds since last reading");
            }
            if (requested)
            {
                message = "; a reading is requested";
            }
            message += '.';
            last_update.set(message);
        });
    }

    void Sht31Sensor::setup()
    {
        if (!is_enabled())
        {
            return;
        }

        Wire.begin();
        if (!sht.begin(address_map[address.get()], index_to_dataline(dataPin.get()), index_to_dataline(clockPin.get())))
        {
            return;
        }
        Wire.setClock(100000);
        if (!sht.isConnected())
        {
            return;
        }

        sht.requestData();
        available = true;
        requested = true;
    }

    void Sht31Sensor::loop()
    {
        if (!available || !is_enabled())
        {
            return;
        }

        if (requested && sht.dataReady())
        {
            bool success  = sht.readData();   // default = true = fast
            if (success)
            {
                temperature_sum += sht.getTemperature() * temperatureScale.get() + temperatureOffset.get();
                humidity_sum += sht.getHumidity() * humidityScale.get() + humidityOffset.get();
                ++reading_count;
            }

            requested = false;
        }

        const uint32_t currentMillis = millis();
        if (!requested && currentMillis - sht.lastRead() >= readInterval.get() * 1000)
        {
            statusReadPreviousMillis = currentMillis;
            sht.requestData();                // request for next sample
            requested = true;
        }
    }

    bool Sht31Sensor::publish(DynamicJsonDocument &json)
    {
        if (reading_count == 0 || !is_enabled())
        {
            return false;
        }

        DynamicJsonDocument sht31Json(1024);

        sht31Json[F("temperature")] = temperature_sum / reading_count;
        sht31Json[F("humidity")] = humidity_sum / reading_count;

        json[F("sht31")] = sht31Json.as<JsonObject>();

        temperature_sum = 0;
        humidity_sum = 0;
        reading_count = 0;

        return true;
    }
}