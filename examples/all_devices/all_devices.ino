#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>

#include "grmcdorman/device/ConfigFile.h"

#include "grmcdorman/device/InfoDisplay.h"
#include "grmcdorman/device/MqttPublisher.h"
#include "grmcdorman/device/Sht31Sensor.h"
#include "grmcdorman/device/SystemDetailsDisplay.h"
#include "grmcdorman/device/VindriktningAirQuality.h"
#include "grmcdorman/device/WifiDisplay.h"
#include "grmcdorman/device/WifiSetup.h"

#include <esp8266_web_settings.h>

// Global constant strings.
static const char firmware_name[] PROGMEM = "esp8266-vindriktning-particle-sensor";
static const char identifier_prefix[] PROGMEM = "VINDRIKTNING-";
static const char manufacturer[] PROGMEM = "grmcdorman";
static const char model[] PROGMEM = "device_framework_example";
static const char software_version[] PROGMEM = "1.0.0";

// The default identifier string.
static char identifier[sizeof ("VINDRIKTNING-") + 12];

// Our config file load/save and the web settings server.
static grmcdorman::device::ConfigFile config;
static grmcdorman::WebSettings webServer;

// Device list. Order _is_ important; this is the order they're presented on the web page.
static std::vector<grmcdorman::device::Device *> devices
{
    new ::grmcdorman::device::InfoDisplay,
    new ::grmcdorman::device::SystemDetailsDisplay,
    new ::grmcdorman::device::WifiDisplay,
    new ::grmcdorman::device::WifiSetup,
    new ::grmcdorman::device::Sht31Sensor,
    new ::grmcdorman::device::VindriktningAirQuality,
    new ::grmcdorman::device::MqttPublisher(FPSTR(manufacturer), FPSTR(model), FPSTR(software_version)) // This uses the default WiFiClient for communications.
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

    for (auto &device: devices)
    {
        device->set_system_identifiers(FPSTR(firmware_name), identifier);
        device->set_defaults();
    }

    config.load(devices);

    for (auto &device : devices)
    {
        device->setup();
        device->set_devices(devices);
        // For the present, only the name can be used here. A future update
        // to the WebSettings library will allow both the name and identifier.
        webServer.add_setting_set(device->name(), device->get_settings());
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
