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
 * This example contains a DHT sensor, WifiSetup, InfoDisplay, SystemDetailsDisplay, and WifiDisplay devices.
 * It does *not* contain a MQTT publisher; instead, the REST API server is configured.
 *
 * To query the REST APIs, start with http://_your device ip_/rest/devices/get. This will
 * return a list of the devices available in JSON. In this case, the response will
 * contain only two devices:
 * `["dht","wifi_setup"]`
 *
 * The values for the devices are at http://_your device ip_/rest/device/_device_/get.
 * For example, the DHT response will be like:
 * `{"dht":{"enabled":true,"temperature":{"average":23.6,"last":23,"sample_count":5,"sample_age_ms":4017},"humidity":{"average":41.2,"last":41,"sample_count":5,"sample_age_ms":4017}}}`
 * and the WiFi setup response will be like:
 * `{"wifi_setup":{"enabled":true,"ssid":"myap","ip":"192.168.213.48","rssi":-46}}`.
 *
 * The DHT can be configured as a DHT11 or DTH22; the default is a DHT11.
 */

#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>

#include <esp8266_web_settings.h>

#include <esp8266_device_framework.h>       // Required by the ESP compiler framework.

#include <grmcdorman/device/ConfigFile.h>

#include <grmcdorman/device/SystemDetailsDisplay.h>
#include <grmcdorman/device/InfoDisplay.h>
#include <grmcdorman/device/WifiDisplay.h>
#include <grmcdorman/device/DhtSensor.h>
#include <grmcdorman/device/WifiSetup.h>
#include <grmcdorman/device/WebServerRestAPI.h>

// Global constant strings.
static const char firmware_name[] PROGMEM = "esp8266-dht-sensor";
static const char identifier_prefix[] PROGMEM = "ESP8266-";
static const char manufacturer[] PROGMEM = "grmcdorman";
static const char model[] PROGMEM = "device_framework_example_dht";
static const char software_version[] PROGMEM = "1.1.0";

// The default identifier string.
static char identifier[sizeof ("ESP8266-") + 12];

// Our config file load/save and the web settings server.
static grmcdorman::device::ConfigFile config;
static grmcdorman::WebSettings webServer;

// Device declarations. Order is not important.
static ::grmcdorman::device::WifiSetup wifi_setup;
static ::grmcdorman::device::DhtSensor dht_sensor;
static ::grmcdorman::device::InfoDisplay info;
static ::grmcdorman::device::SystemDetailsDisplay details;
static ::grmcdorman::device::WifiDisplay wifi;
static ::grmcdorman::device::WebServerRestApi rest_api;

// Device list. Order _is_ important; this is the order they're presented on the web page.
static std::vector<grmcdorman::device::Device *> devices
{
    &dht_sensor,
    &wifi_setup, &info, &details, &wifi
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

    dht_sensor.set("dht_model", "DHT11");
    //wifi_setup.set("ssid", "my access point");
    // Note that this will *not* be shown in the web page UI.
    //wifi_setup.set("password", "my password");

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

    rest_api.set_devices(devices);
    rest_api.setup(webServer.get_server());

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
