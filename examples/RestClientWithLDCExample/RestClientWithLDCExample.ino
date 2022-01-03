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
 * This example is a _client_ that connects to a Vindriktning+SHT31 ESP8266, retrieves the
 * state, and displays the values - PM 2.5, temperature, humidity - on an LCD, along with
 * the current uptime. The client must have the Rest API library enabled.
 *
 * The LCD is assumed to be a 2 row, 16 column device.
 *
 * The backlight turns off after 30 seconds. D1 is assumed to be connected to
 * a button (or other trigger) to turn the backlight back on.
 *
 * The demo code from WebSettings has been adapted to provide the settings
 * for entering a WiFi access point name and password, and the IP address
 * (or host name) of your ESP8266-modified Vindriktning.
 *
 * If the target does not have a Vindriktning, or does not have a SHT31-D, the
 * associated entries will show F:404 (i.e. not found).
 *
 * Additional libraries required:
 *  - LiquidCrystal_I2C
 */
#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <Ticker.h>
#include <DNSServer.h>
#include <LittleFS.h>

#include <esp8266_web_settings.h>

// Forward declarations
static void wifi_setup();
static std::optional<DynamicJsonDocument> LoadConfig();
static void on_save(::grmcdorman::WebSettings &);

#define DECLARE_SETTING(name, text, type) \
static const char * PROGMEM name##_text = text; \
static const char * PROGMEM name##_id = #name; \
static ::grmcdorman::type name(FPSTR(name##_text), FPSTR(name##_id));

#define DECLARE_INFO_SETTING(name, text) DECLARE_SETTING(name, text, InfoSettingHtml)
#define DECLARE_STRING_SETTING(name, text) DECLARE_SETTING(name, text, StringSetting)
#define DECLARE_PASSWORD_SETTING(name, text) DECLARE_SETTING(name, text, PasswordSetting)
#define DECLARE_TOGGLE_SETTING(name, text) DECLARE_SETTING(name, text, ToggleSetting)
#define DECLARE_UNSIGNED_SETTING(name, text) DECLARE_SETTING(name, text, UnsignedIntegerSetting)

DECLARE_INFO_SETTING(info, "ESP8266 Device REST Client Example");
DECLARE_STRING_SETTING(ap_name, "WiFi Access Point");
DECLARE_PASSWORD_SETTING(ap_password, "WiFi Password");
DECLARE_STRING_SETTING(server_address, "Vindriktning Server Address");
static ::grmcdorman::SettingInterface::settings_list_t
        settings{&info, &ap_name, &ap_password, &server_address};

static const char config_path[] PROGMEM = "/config.json";

static LiquidCrystal_I2C lcd(0x27, 16, 2);
static WiFiClient wifi_client;
static HTTPClient http_client;
static Ticker update_pm25_timer;
static Ticker update_temperature_timer;
static Ticker update_time_timer;
static Ticker backlight_timer;
static std::unique_ptr<DNSServer> dns_server;   //!< The DNS server for SoftAP mode.
static grmcdorman::WebSettings web_settings;    //!< The settings web server. Default port (80).

// Define some custom characters.
static byte superscript3[] = {
  0x18,
  0x04,
  0x18,
  0x04,
  0x18,
  0x00,
  0x00,
  0x00
};

static byte mu[] = {
  0x00,
  0x00,
  0x11,
  0x11,
  0x11,
  0x13,
  0x1D,
  0x10
};

static byte degree[] = {
  0x04,
  0x0A,
  0x04,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00
};

static void update_pm25();
static void update_temperature();
static void update_time();
static void backlight_off();

static constexpr auto expectedTemperaturePrintLength = sizeof("99d 99%") - 1;

void setup()
{
    Serial.begin(115200);
    Serial.println("Begin LCD REST Client Demo");
    // Begin Wire on D2, D3
    Wire.begin(4, 0);
    lcd.init();
    lcd.backlight();

    lcd.setCursor(0, 0);
    lcd.print("Initializing");
    lcd.createChar(1, superscript3);
    lcd.createChar(2, mu);
    lcd.createChar(3, degree);
    pinMode(5, INPUT);

    auto config = LoadConfig();
    if (config)
    {
        for (auto &setting: settings)
        {
            if (setting->is_persistable() && !(*config)[setting->name()].isNull())
            {
                setting->set_from_string((*config)[setting->name()]);
            }
        }
    }

    // Apply loaded WiFi settings
    wifi_setup();

    // Add our one tab to the web server.
    web_settings.add_setting_set(F("WiFi"), F("wifi_settings"), settings);
    web_settings.setup(on_save, nullptr, nullptr);


    update_pm25_timer.attach_scheduled(23, update_pm25);
    update_temperature_timer.attach_scheduled(11, update_temperature);
    update_time_timer.attach_scheduled(30, update_time);
    backlight_timer.attach_scheduled(30, backlight_off);
    http_client.setTimeout(5000);   // Timeout is in milliseconds.
}

void loop()
{
    // Handle DNS requests. This will make our captive portal work.
    if (dns_server)
    {
        dns_server->processNextRequest();
    }

    // Not presently essential; might be needed in future.
    web_settings.loop();


    if (digitalRead(D1) == HIGH)
    {
        lcd.backlight();
        backlight_timer.attach_scheduled(30, backlight_off);
    }
    static auto lastStatus = static_cast<decltype(WiFi.status())>(-1);

    if (lastStatus != WiFi.status())
    {
        lcd.setCursor(0, 0);
        update_time();
        lastStatus = WiFi.status();
        if (lastStatus == WL_CONNECTED)
        {
            //lcd.print( WiFi.localIP().toString());
            update_temperature();
            update_pm25();
        }
        else if (lastStatus == WL_CONNECT_FAILED)
        {
            lcd.print("Connection failed");
        }
    }
}

static void backlight_off()
{
    lcd.noBacklight();
}

static void update_time()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        // If you use the NTPClient library or similar,
        // this could be the actual time. Do not print seconds,
        // that looks ugly on most LCD displays (too much refreshing).
        lcd.setCursor(0, 0);
        auto total_seconds = millis() / 1000;
        auto minutes = (total_seconds / 60) % 60;
        auto hours = (total_seconds / 60) / 60;
        int col = 0;
        if (hours < 10)
        {
            col += lcd.print(' ');
        }
        col += lcd.print(hours);
        col += lcd.print(':');
        if (minutes < 10)
        {
            col += lcd.print('0');
        }
        col += lcd.print(minutes);
        while (col < 16)
        {
            col += lcd.print(' ');
        }
    }
}

static void update_temperature()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        if (http_client.begin(wifi_client, "http://" + server_address.get() + "/rest/device/sht31_d/get"))
        {
            constexpr int startCol = 0;
            constexpr int row = 1;
            lcd.setCursor(startCol, row);
            lcd.print("Get T");
            auto status = http_client.GET();
            lcd.setCursor(startCol, row);
            size_t col = startCol;
            if (status == 200)
            {
                DynamicJsonDocument doc(1024);
                deserializeJson(doc, http_client.getStream());
                auto temperature = doc["sht31_d"]["temperature"]["average"].as<float>();
                auto humidity = doc["sht31_d"]["humidity"]["average"].as<float>();
                col = lcd.print(static_cast<int>(temperature + 0.5));
                col += lcd.print('\03');
                col += lcd.print(' ');
                col += lcd.print(static_cast<int>(humidity + 0.5));
                col += lcd.print('%');
            }
            else if (status == HTTPC_ERROR_READ_TIMEOUT)
            {
                col = lcd.print("T/O");
            }
            else
            {
                col = lcd.print("F: ");
                col += lcd.print(status);
            }
            http_client.end();
            while (col < expectedTemperaturePrintLength + 1)
            {
                col += lcd.print(' ');
            }
        }
        else
        {
            Serial.println("http_client begin failed");
        }
    }
}


static void update_pm25()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        // Leave space for an actual time.
        constexpr auto startCol = expectedTemperaturePrintLength + 1;
        constexpr int row = 1;
        if (http_client.begin(wifi_client, "http://" + server_address.get() + "/rest/device/vindriktning/get"))
        {
            lcd.setCursor(startCol, row);
            size_t col = startCol;
            lcd.print("PM25");
            lcd.setCursor(startCol, row);
            auto status = http_client.GET();
            if (status == 200)
            {
                DynamicJsonDocument doc(1024);
                deserializeJson(doc, http_client.getStream());
                auto pm25 = doc["vindriktning"]["pm25"]["average"].as<float>();
                col = startCol + lcd.print(static_cast<int>(pm25 + 0.5));
                col += lcd.print("\02g/m\01");
                while (col < 16)
                {
                    col += lcd.print(' ');
                }
            }
            else if (status == HTTPC_ERROR_READ_TIMEOUT)
            {
                col = lcd.print("T/O");
            }
            else
            {
                col = lcd.print("F: ");
                col += lcd.print(status);
            }
            http_client.end();
            while (col < 16)
            {
                col += lcd.print(' ');
            }
        }
        else
        {
            Serial.println("pm25 http_client begin failed");
        }
    }
}

// Set up WiFi.
static void wifi_setup()
{

    if (!ap_name.get().isEmpty())
    {
        lcd.backlight();
        lcd.setCursor(0, 0);
        lcd.print("Connecting: ");
        lcd.setCursor(0, 1);
        lcd.print(ap_name.get().substring(0, 16));
        // Do not use persistent WiFi settings, we manage those ourselves.
        WiFi.persistent(false);
        if (!WiFi.mode(WIFI_STA))
        {
            Serial.println(F("Warning: Unable to set STA mode"));
        }

        auto begin_status = WiFi.begin(ap_name.get(), ap_password.get());

        if (!WiFi.config(0u, 0u, 0u))
        {
            Serial.println(F("Warning: Config for DHCP failed"));
        }

        auto status = WiFi.waitForConnectResult(30 * 1000);
        if (status != WL_CONNECTED)
        {
            Serial.println(F("Unable to connect to the access point, status =") + String(status));
        }
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        // No SSID. Start in AP.
        Serial.println(F("Starting in AP mode"));
        lcd.setCursor(0, 0);
        lcd.print("AP mode; SSID: ");
#ifdef ESP8266
// @bug workaround for bug #4372 https://github.com/esp8266/Arduino/issues/4372
        WiFi.enableAP(true);
        delay(500); // workaround delay
#endif
        WiFi.mode(WIFI_AP);
        // "target_hostname" is the SSID.
        WiFi.softAP(F("RestDemo") + String(ESP.getChipId(), 16));
        delay(500);
        Serial.print(F("Soft AP started at address "));
        Serial.println(WiFi.softAPIP().toString());
        dns_server = std::make_unique<DNSServer>();
        dns_server->setErrorReplyCode(DNSReplyCode::NoError);
        dns_server->start(53, "*", WiFi.softAPIP());
        lcd.setCursor(0, 1);
        lcd.print(WiFi.softAPSSID());
    }

    backlight_timer.attach_scheduled(30, backlight_off);
}

static std::optional<DynamicJsonDocument> LoadConfig()
{

    if (!LittleFS.begin()) {
        return std::nullopt;
    }

    if (!LittleFS.exists(FPSTR(config_path))) {
        return std::nullopt;
    }

    File configFile = LittleFS.open(FPSTR(config_path), "r");

    if (!configFile) {
        return std::nullopt;
    }

    DynamicJsonDocument json(configFile.size() * 10);

    auto status = deserializeJson(json, configFile);
    if (DeserializationError::Ok != status) {
        Serial.print(F("Deserialization error: "));
        Serial.println(status.c_str());
        return std::nullopt;
    }
    return std::move(json);
}

static void on_save(::grmcdorman::WebSettings &)
{
    Serial.println("Saving settings");
    DynamicJsonDocument json(4096);
    for (auto &setting: settings)
    {
        if (strlen_P(reinterpret_cast<const char *>(setting->name())) != 0 && setting->is_persistable())
        {
            json[setting->name()] = setting->as_string();
        }
    }

    File configFile = LittleFS.open(FPSTR(config_path), "w");
    if (!configFile) {
        return;
    }

    serializeJson(json, configFile);
    configFile.close();

    schedule_function([] {
        if (!WiFi.isConnected())
        {
            WiFi.softAPdisconnect();
            WiFi.enableAP(false);
            wifi_setup();
        }
    });
}
