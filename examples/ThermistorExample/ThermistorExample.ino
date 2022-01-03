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
 * This example uses the ThermistorSensor device to collect and publish thermistor readings.
 * Correct the thermistor values to correspond to your thermistor.
 */

static constexpr unsigned int THERMISTOR_THERMAL_INDEX = 3950;
static constexpr float THERMISTOR_REFERENCE_TEMPERATURE_CELCIUS = 25;

#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>

#include <esp8266_web_settings.h>

#include <esp8266_device_framework.h>       // Required by the ESP compiler framework.

#include <grmcdorman/device/ConfigFile.h>

#include <grmcdorman/device/MqttPublisher.h>
#include <grmcdorman/device/ThermistorSensor.h>
#include <grmcdorman/device/WifiSetup.h>


// Global constant strings.
static const char firmware_name[] PROGMEM = "esp8266-analog-sensor";
static const char identifier_prefix[] PROGMEM = "ESP8266-";
static const char manufacturer[] PROGMEM = "grmcdorman";
static const char model[] PROGMEM = "device_framework_example";
static const char software_version[] PROGMEM = "1.1.0";

// The default identifier string.
static char identifier[sizeof ("ESP8266-") + 12];

// Our config file load/save and the web settings server.
static grmcdorman::device::ConfigFile config;
static grmcdorman::WebSettings webServer;

// Device declarations. Order is not important.
static ::grmcdorman::device::WifiSetup wifi_setup;
static ::grmcdorman::device::ThermistorSensor thermistor_sensor(THERMISTOR_THERMAL_INDEX, 273.15 + THERMISTOR_REFERENCE_TEMPERATURE_CELCIUS);

// This uses the default WiFiClient for communications.
static ::grmcdorman::device::MqttPublisher mqtt_publisher(FPSTR(manufacturer), FPSTR(model), FPSTR(software_version));


// Device list. Order _is_ important; this is the order they're presented on the web page.
static std::vector<grmcdorman::device::Device *> devices
{
    &thermistor_sensor,
    &wifi_setup,
    &mqtt_publisher
};

// State.
static bool factory_reset_next_loop = false;    //!< Set to true when a factory reset has been requested.
static bool restart_next_loop = false;          //!< Set to true when a simple reset has been requested.
static uint32_t restart_reset_when = 0;         //!< The time the factory reset/reset was requested.
static constexpr uint32_t restart_reset_delay = 500;    //!< How long after the request for factory reset/reset to actually perform the function

// Forward declarations for the three web_settings callbacks.

static void on_factory_reset(::grmcdorman::WebSettings &);
static void on_restart(::grmcdorman::WebSettings &);
static void on_save(::grmcdorman::WebSettings &);

void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.print(firmware_name);
    Serial.println(" is starting");

    strcpy_P(identifier, identifier_prefix);
    itoa(ESP.getChipId(), identifier + strlen(identifier), 16);
    Serial.print("My default identifier is ");
    Serial.println(identifier);

    // Set some defaults.

    wifi_setup.set("ssid", "my access point");
    // Note that this will *not* be shown in the web page UI.
    wifi_setup.set("password", "my password");

    for (auto &device: devices)
    {
        device->set_system_identifiers(FPSTR(firmware_name), identifier);
        device->set_defaults();
    }

    config.load(devices);

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
