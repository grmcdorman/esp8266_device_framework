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
 * This example includes the original seven devices:
 * - The InfoDisplay
 * - The MQTT publisher
 * - The SHT31-D sensor
 * - The System details display
 * - The Vindriktning Air Quality sensor
 * - The WiFi Display
 * - The WiFi setup
 */

#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>

#include <esp8266_web_settings.h>

#include <esp8266_device_framework.h>       // Required by the ESP compiler framework.

#include <grmcdorman/device/ConfigFile.h>

#include <grmcdorman/device/InfoDisplay.h>
#include <grmcdorman/device/MqttPublisher.h>
#include <grmcdorman/device/Sht31Sensor.h>
#include <grmcdorman/device/SystemDetailsDisplay.h>
#include <grmcdorman/device/VindriktningAirQuality.h>
#include <grmcdorman/device/WifiDisplay.h>
#include <grmcdorman/device/WifiSetup.h>


// Global constant strings.
static const char firmware_name[] PROGMEM = "esp8266-vindriktning-particle-sensor";
static const char identifier_prefix[] PROGMEM = "VINDRIKTNING-";
static const char manufacturer[] PROGMEM = "grmcdorman";
static const char model[] PROGMEM = "device_framework_example";
static const char software_version[] PROGMEM = "1.1.0";

// The default identifier string.
static char identifier[sizeof ("VINDRIKTNING-") + 12];

// Our config file load/save and the web settings server.
static grmcdorman::device::ConfigFile config;
static grmcdorman::WebSettings webServer;

// Device declarations. Order is not important.
static ::grmcdorman::device::InfoDisplay info_display;
static ::grmcdorman::device::SystemDetailsDisplay system_details_display;
static ::grmcdorman::device::WifiDisplay wifi_display;
static ::grmcdorman::device::WifiSetup wifi_setup;
static ::grmcdorman::device::Sht31Sensor sht31_sensor;
static ::grmcdorman::device::VindriktningAirQuality vindriktning_air_quality;

// This uses the default WiFiClient for communications.
static ::grmcdorman::device::MqttPublisher mqtt_publisher(FPSTR(manufacturer), FPSTR(model), FPSTR(software_version));


// Device list. Order _is_ important; this is the order they're presented on the web page.
static std::vector<grmcdorman::device::Device *> devices
{
    &info_display,
    &system_details_display,
    &wifi_display,
    &wifi_setup,
    &sht31_sensor,
    &vindriktning_air_quality,
    &mqtt_publisher
};

// State.
static bool set_save_credentials = false;       //!< Set to true when credentials for save/restart etc. have been configured.
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

    // Device index # 0 is InfoDisplay. It has no relevant settings.
    // Device index # 1 is System Details Display. It has no relevant settings.
    // Device index # 2 is WiFi display. It has no relevant settings.
    // Device index # 3 is the WiFi setup. A default AP and password could be set:
    devices[3]->set("ssid", "my access point");
    // Note that this will *not* be shown in the web page UI.
    devices[3]->set("password", "my password");

    // Device index # 4 is the SHT31-D. Set the default sda to D2, scl to D3.
    devices[4]->set("sda", "D2");
    devices[4]->set("scl", "D3");

    for (auto &device: devices)
    {
        device->set_system_identifiers(FPSTR(firmware_name), identifier);
        device->set_defaults();
    }

    config.load(devices);

    // If you wanted to override settings, you could call device 'set' methods here.

    // Print some device settings.
    Serial.print("WiFi SSID is ");
    Serial.println(devices[3]->get("ssid"));
    Serial.print("SHT31-D SDA is on pin ");
    Serial.println(devices[4]->get("sda"));

    for (auto &device : devices)
    {
        device->setup();
        device->set_devices(devices);
        webServer.add_setting_set(device->name(), device->identifier(), device->get_settings());
    }

    if (WiFi.isConnected())
    {
        // SoftAP capture portal clients are typically not happy about authentication.
        webServer.set_credentials("admin", identifier);
        set_save_credentials = true;
        setupOTA();
    }

    webServer.setup(on_save, on_restart, on_factory_reset);

    Serial.print(F("IP: "));
    Serial.println(WiFi.localIP().toString());
}

void setupOTA() {
    ArduinoOTA.onStart([]() { Serial.println("Start"); });
    ArduinoOTA.onEnd([]() { Serial.println("\nEnd"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) {
            Serial.println("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
            Serial.println("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
            Serial.println("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
            Serial.println("Receive Failed");
        } else if (error == OTA_END_ERROR) {
            Serial.println("End Failed");
        }
    });

    // This needs a regular string.
    ArduinoOTA.setHostname(String(WiFi.getHostname()).c_str());

    // This could also be a setting
    ArduinoOTA.setPassword(identifier);
    ArduinoOTA.begin();
}

void loop()
{
    if (WiFi.isConnected())
    {
        if (!set_save_credentials)
        {
            // Connected to WiFi, but credentials for save/reset etc. were not set
            webServer.set_credentials("admin", identifier);
            set_save_credentials = true;
        }
    }
    else if (WiFi.softAPgetStationNum() != 0)
    {
        if (set_save_credentials)
        {
            // In Soft AP mode with at least one client connected, and a password was set.
            webServer.set_credentials(String(), String());
        }
    }

    ArduinoOTA.handle();
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
