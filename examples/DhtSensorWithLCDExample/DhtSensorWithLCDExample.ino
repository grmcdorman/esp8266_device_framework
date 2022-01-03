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

/*
 * This example uses the DhtSensor to collect readings from a DHT11 or DHT22 sensor. As usual,
 * the readings can be published to a MQTT server. However, the readings are also displayed
 * on an attached LCD display of at least 2 rows and 16 columns.
 *
 * The LCD backlight is never turned off; you may want to add a button to control this.
 *
 * In addition to the basic libraries required by the device framework, this requires
 * marcoschwartz/LiquidCrystal_I2C to be installed with your library manager.
 */

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <Wire.h>

#include <LiquidCrystal_I2C.h>

#include <esp8266_web_settings.h>

#include <esp8266_device_framework.h>       // Required by the ESP compiler framework.

#include <grmcdorman/device/ConfigFile.h>

#include <grmcdorman/device/MqttPublisher.h>
#include <grmcdorman/device/DhtSensor.h>
#include <grmcdorman/device/WifiSetup.h>

// Global constant strings.
static const char firmware_name[] PROGMEM = "esp8266-dht-sensor";
static const char identifier_prefix[] PROGMEM = "ESP8266-";
static const char manufacturer[] PROGMEM = "grmcdorman";
static const char model[] PROGMEM = "device_framework_example_dht";
static const char software_version[] PROGMEM = "1.1.0";

// A degree symbol for the LCD.
static const byte degree[] PROGMEM = {
  0x04,
  0x0A,
  0x04,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00
};


// The default identifier string.
static char identifier[sizeof ("ESP8266-") + 12];

// Our config file load/save and the web settings server.
static grmcdorman::device::ConfigFile config;
static grmcdorman::WebSettings webServer;

// Device declarations. Order is not important.
static ::grmcdorman::device::WifiSetup wifi_setup;
static ::grmcdorman::device::DhtSensor dht_sensor;
// This uses the default WiFiClient for communications.
static ::grmcdorman::device::MqttPublisher mqtt_publisher(FPSTR(manufacturer), FPSTR(model), FPSTR(software_version));

// This should be adjusted as per your LCD, or you can create configurable settings.
static LiquidCrystal_I2C lcd(0x27, 16, 2);

static Ticker lcd_update_ticker;

// Device list. Order _is_ important; this is the order they're presented on the web page.
static std::vector<grmcdorman::device::Device *> devices
{
    &dht_sensor,
    &wifi_setup,
    &mqtt_publisher
};

// State.
static bool wifi_did_connect = false;

static bool factory_reset_next_loop = false;    //!< Set to true when a factory reset has been requested.
static bool restart_next_loop = false;          //!< Set to true when a simple reset has been requested.
static uint32_t restart_reset_when = 0;         //!< The time the factory reset/reset was requested.
static constexpr uint32_t restart_reset_delay = 500;    //!< How long after the request for factory reset/reset to actually perform the function

// Forward declarations for the three web_settings callbacks.

static void on_factory_reset(::grmcdorman::WebSettings &);
static void on_restart(::grmcdorman::WebSettings &);
static void on_save(::grmcdorman::WebSettings &);

static void lcd_report_wifi();

void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.print(firmware_name);
    Serial.println(" is starting");
    Serial.flush();

    strcpy_P(identifier, identifier_prefix);
    itoa(ESP.getChipId(), identifier + strlen(identifier), 16);
    Serial.print("My default identifier is ");
    Serial.println(identifier);
    Serial.flush();

    // Set up the LCD, I2C on D1/D2.
    Wire.begin(5, 4);
    lcd.init();

    // While it is not essential to have this in PROGMEM, this
    // code shows how to do so if you are tight on space or have
    // lots of characters to set into the LCD. It also avoids
    // the issue that the second argument to `createChar`
    // is *not* const, and in theory could be overwritten.
    byte char_matrix[sizeof(degree) / sizeof(degree[0])];
    memcpy_P(char_matrix, degree, sizeof (degree));
    lcd.createChar(1, char_matrix);

    // Create characters for 'bars', from 1 to 4 at indices 2 to 5.
    // There are 8 rows, so each level adds two rows.
    memset(char_matrix, 0, sizeof(char_matrix));

    for (int barLevel = 1; barLevel <= 4 ;++barLevel)
    {
        for (int row = 0; row < barLevel * 2; ++row)
        {
            char_matrix[7 - row] =   B11111;
        }
        lcd.createChar(barLevel + 1, char_matrix);
    }

    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connecting");

    // At the moment there is no 'read' callback for the device; instead this will simply use a
    // Ticker to update the LCD every so often.
    lcd_update_ticker.attach_scheduled(5, [] {
        lcd.setCursor(0, 1);
        size_t col = 0;
        col += lcd.print(dht_sensor.get_temperature());
        col += lcd.print('\01');
        col += lcd.print(' ');
        col += lcd.print(dht_sensor.get_humidity());
        col += lcd.print('%');
        // Fill with spaces to the end of the 16 columns.
        while (col < 16)
        {
            col += lcd.print(' ');
        }
        lcd_report_wifi();
    });

    // Set some defaults.
    // Remember, these are *defaults*; loading the configuration
    // will set them to the saved values, if the configuration file is present.
    // This sets the DHT model as a DHT22. Obviously, if you have a DHT11
    // set that, or don't make this call (the default is DHT11).
    dht_sensor.set("dht_model", "DHT22");
    Serial.println(dht_sensor.get("dht_model"));

    for (auto &device: devices)
    {
        device->set_system_identifiers(FPSTR(firmware_name), identifier);
        device->set_defaults();
    }

    // N.B. This will override the model set above if the settings are ever saved.
    config.load(devices);

    Serial.print("Configured DHT model is: ");
    Serial.println(dht_sensor.get("dht_model"));

    // If you wanted to override settings, you could call device 'set' methods here.

    // Print some device settings.
    Serial.print("WiFi SSID is ");
    Serial.println(wifi_setup.get("ssid"));

    for (auto &device : devices)
    {
        device->setup();
        device->set_devices(devices);
        webServer.add_setting_set(device->name(), device->identifier(), device->get_settings());
    }

    webServer.setup(on_save, on_restart, on_factory_reset);

    Serial.print(F("IP: "));
    Serial.println(WiFi.localIP().toString());
}

void loop()
{

    webServer.loop();

    for (auto &device : devices)
    {
        if (device->is_enabled())
        {
            device->loop();
        }
    }

    if (factory_reset_next_loop && millis() - restart_reset_when > restart_reset_delay)
    {
        // Clear file system.
        LittleFS.format();
        // Erase configuration
        ESP.eraseConfig();
        // Reset (not reboot, that may save current state)
        ESP.reset();
    }

    if (restart_next_loop && millis() - restart_reset_when > restart_reset_delay)
    {
        ESP.restart();
    }
}

static void on_factory_reset(::grmcdorman::WebSettings &)
{
    factory_reset_next_loop = true;
    restart_reset_when = millis();
}

static void on_restart(::grmcdorman::WebSettings &)
{
    restart_next_loop =-true;
    restart_reset_when = millis();
}

static void on_save(::grmcdorman::WebSettings &)
{
    config.save(devices);
}

static void lcd_report_wifi()
{
    if (WiFi.isConnected())
    {
        auto signal = WiFi.RSSI();
        if (signal != 0)
        {
            int bars = 0;
            static int lastBars = -1;
            if (signal < -89 || signal == 0)
            {
                bars = 0;
            }
            else if (signal < -78)  // -78 to -88
            {
                bars = 1;
            }
            else if (signal < -67) // -67 to -77
            {
                bars = 2;
            }
            else if (signal < -56) // -56 to -66
            {
                bars = 3;
            }
            else // -55 or higher
            {
                bars = 4;
            }
            if (bars != lastBars)
            {
                lastBars = bars;
                lcd.setCursor(0, 0);
                if (bars == 0)
                {
                    lcd.print(' ');
                }
                else
                {
                    lcd.print(static_cast<char>(bars + 1));
                }
            }
        }
    }
    if (!wifi_did_connect && WiFi.isConnected())
    {
        // Erase message.
        lcd.setCursor(1, 0);
        auto col = 1 + lcd.print(WiFi.localIP().toString());
        while (col < 16)
        {
            lcd.print(' ');
            ++col;
        }
        wifi_did_connect = true;
    }

}