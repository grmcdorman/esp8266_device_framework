# esp8266_device_framework
<h1>Generic ESP8266 Device Framework Library</h1>

This repository contains a generic framework to manage the sensors and other devices, including virtual "devices", for your ESP8266.

All devices can have a collection of _settings_ as defined in the [Web Settings](https://grmcdorman.github.io/esp8266_web_settings) library; by simply adding the device's settings to a `WebSetting` instance you will automatically have a page with the settings and status for the device.

In addition, devices can easily publish to a MQTT server, provided the `MqttPublisher` device is included, configured, and enabled.

**Warning** The standard [ESP Async Web Server](https://github.com/me-no-dev/ESPAsyncWebServer) adds substantial overhead to your sketch; the ESP fork of that project, [ESP Home ESP Async Web Server](https://github.com/esphome/ESPAsyncWebServer) both compiles to substantially smaller code and has a smaller memory footprint. It is not clear what ESP Home has done in their fork; presumably they have removed functionality. Testing with the ESP Home fork and this code has, as yet, shown no issues. If you have sufficient storage on your device, I recommend you use the primary repository; if not, you can use the ESP Home fork. The D1 Mini I am using, however, does not have sufficient storage; I am using the ESP Home fork in my own projects.

The example provided in this repository is the actual working sketch I use for my own Ikea Vindriktning air quality sensor; I have added a SHT31-D to the setup. You may notice from the screen shots that a temperature offset is applied; it appears that either the sensor reads high by two to three degrees Celcius or the heat generated inside the Vindriktning case throws off the reading by that amount.

At the moment, there are seven devices:
* [`InfoDisplay`](https://grmcdorman.github.io/esp8266_device_framework/classgrmcdorman_1_1device_1_1_info_display.html): When connected to a `WebSetting` instance, displays and updates basic system information:
  * Host name and IP address.
  * Connected access point
  * Signal strength
  * Soft API SSID.
  * Heap information (allocatable memory), with fragmentation
  * Uptime (from the `millis()` system call; this will wrap around at about 50 days)
  * `LitteLFS` file system free space and used space
* [`MqttPublisher`](https://grmcdorman.github.io/esp8266_device_framework/classgrmcdorman_1_1device_1_1_mqtt_publisher.html): This controls message publishing to a MQTT server.
* [`Sht31Sensor`](https://grmcdorman.github.io/esp8266_device_framework/classgrmcdorman_1_1device_1_1_sht31_sensor.html): This polls a SHT31-D temperature and humidity sensor. The default I2C lines are SDA on D5 and SCL on D6, but this can be configured.
* [`SystemDetailsDisplay`](https://grmcdorman.github.io/esp8266_device_framework/classgrmcdorman_1_1device_1_1_system_details_display.html): This displays static system details:
  * Installed Firmware (the firmware string passed to `set_system_identifiers`)
  * Firmware Built (the date and time the compile was done)
  * Architecture (currently hard-coded to esp8266)
  * Device Chip ID (the unique device chip ID, from `ESP.getChipId()`)
  * Flash Chip ID (from `ESP.getFlashChipId()`)
  * Last reset reason
  * Flash memory size
  * Real flash size
  * Sketch space (used and total)
  * Vendor Chip ID (from `ESP.getFlashChipVendorId()`)
  * Core version
  * Boot version
  * SDK version
  * CPU frequencey
* [`VindriktningAirQuality`](https://grmcdorman.github.io/esp8266_device_framework/classgrmcdorman_1_1device_1_1_vindriktning_air_quality.html): This monitors an Ikea Vindriktning air quality sensor. This class is derived from work by Hypfer's GitHub project, https://github.com/Hypfer/esp8266-vindriktning-particle-sensor. All work on message deciphering comes from that project.
* [`WifiDisplay`](https://grmcdorman.github.io/esp8266_device_framework/classgrmcdorman_1_1device_1_1_wifi_display.html):  When connected to a `WebSetting` instance, displays and updates basic WiFi information:
  * Soft IP Address (if applicable)
  * Soft AP MAC address
  * BSSID
  * IP Address
  * Gateway IP Address
  * Subnet Mask
  * DNS Server Address
  * MAC Address
  * Connected
  * Auto Connect
* [`WifiSetup`](https://grmcdorman.github.io/esp8266_device_framework/classgrmcdorman_1_1device_1_1_wifi_setup.html): Provides settings for WiFi configuration. Patterned in part after the Windows TCP/IP configuration dialog. This will start a Soft Access Point for configuration if no WiFi configuration exists, or the WiFi connection cannot be established. **Warning**: The Soft AP, at the moment, cannot be password protected.
  * Host name
  * Access point SSID
  * Access point password
  * "Obtain an IP address automatically" (i.e. use DHCP)
  * IP Address (static IP address, if applicable)
  * Subnet mask
  * Default Gateway
  * "Obtain DNS server address automatically"
  * Preferred DNS server
  * Alternative DNS server
  * Connection timeout (seconds)
  * Publish WiFi signal strength (when enabled, the WiFi signal will be published via the MqttPublish class, if connected).

An additional simple utility class to load and save device settings to `LittleFS` storage is provided.

Usage is generally pretty simple:

* Create a vector (`std::vector<Device *>`) of devices, containing each of the devices you want to use, and declare a `WebSettings` and a `Config`:
```
static std::vector<::grmcdorman::device::Device *> devices{
    new ::grmcdorman::device::InfoDisplay,
    new ::grmcdorman::device::SystemDetailsDisplay,
    new ::grmcdorman::device::WifiDisplay,
    new ::grmcdorman::device::WifiSetup,
    new ::grmcdorman::device::Sht31Sensor,
    new ::grmcdorman::device::VindriktningAirQuality,
    new ::grmcdorman::device::MqttPublisher(FPSTR(manufacturer), FPSTR(model), FPSTR(software_version))
};
static grmcdorman::device::ConfigFile config;
static grmcdorman::WebSettings webServer;
```
* In your `setup()` function, provide your default identifiers and set device defaults:
```
    for (auto &device: devices)
    {
        device->set_system_identifiers(FPSTR(firmware_name), identifier);
        device->set_defaults();
    }
```
* Optionally, set some defaults different from the built-in ones:
```
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
```
* Load and apply settings:
```
    confg.load(devices);
```
* Set up devices and add to the `WebSettings`:
```
    for (auto &device : devices)
    {
        device->setup();
        device->set_devices(devices);
        // For the present, only the name can be used here. A future update
        // to the WebSettings library will allow both the name and identifier.
        webServer.add_setting_set(device->name(), device->identifier(), device->get_settings());
    }
```
* Call the device `loop` methods in your `loop` function:
```
void loop()
{
    // Other code as required ...
    for (auto &device : devices)
    {
        if (device->is_enabled())
        {
            device->loop();
        }
    }
}
```
* In the `WebSettings` save callback, save the device settings:
```
static void on_save(::grmcdorman::WebSettings &)
{
    config.save(devices);
}
```

There will be some other management around the `WebSettings` class, for things like reset and factory defaults callbacks. See the example for all the details. The example also includes OTA support (which, in theory, could also be a device, but it's simple enough that it's not needed).

<h2>XHR/JavaScript requests</h2>
The web settings server can be queried for device status via the `/settings/get` URL. The URL requires a query parameter that is the setting set identifier (which will be the device identifier if you follow the code layout above). The response is in JSON.

So, for example, the URL `/settings/get/?tab=system_overview` will return all the settings for the system overview tab:
```
{"system_overview":
    [
        {"name":"host","value":"VINDRIKTNING-4DB7B3 [192.168.213.45]"},
        {"name":"station_ssid","value":"mySSID"},
        {"name":"rssi","value":"◾◾ -74 dBm"},
        {"name":"softap","value":"VINDRIKTNING-4DB7B3"},
        {"name":"heap_status","value":"26104 bytes (fragmentation: 8)"},
        {"name":"uptime","value":"1:06:54"},
        {"name":"filesystem","value":"LittleFS: total bytes 1024000, used bytes: 24576"},
        {"name":"device_status","value":"SHT31-D: 19.6 °C, 49.8% R.H.; 0 seconds since last reading.<br>Vindriktning: 8µg/m³, 1 seconds since last reading. <br>MQTT: Last publish succeeded 12 seconds ago."}
    ]
}
```
You can also request a subset of the items by adding one or more `setting=_name_` parameters. Using the same system overview again, to get just the uptime and system status: `/settings/get/?tab=system_overview&setting=uptime&setting=device_status` would return:
```
{"system_overview":
    [
        {"name":"uptime","value":"1:06:54"},
        {"name":"device_status","value":"SHT31-D: 19.6 °C, 49.8% R.H.; 0 seconds since last reading.<br>Vindriktning: 8µg/m³, 1 seconds since last reading. <br>MQTT: Last publish succeeded 12 seconds ago."}
    ]
}
```
While none of the fields, at the moment, contain _just_ the raw sensor values, it should be easy to modify the code to provide those if needed by your client code.

For example, if you want to consume these values in a PowerShell script:
```
$json = (Invoke-WebRequest 'http://my-esp-device/settings/get?tab=system_overview&setting=device_status' ).content | ConvertFrom-Json
$status = $json.system_overview[0].value
# Do something with $status
```

Note that the encoding is UTF-8. However, at the moment the settings server is not including a character set in the response headers.

Screenshots:

* `InfoDisplay`: ![Info display](images/Screenshot-info-panel.png?raw=true "Info Panel")
* `MqttPublisher`: ![MQTT publisher](images/Screenshot-mqtt-publisher-panel.png?raw=true "Info Panel")
* `Sht31Sensor`: ![SHT31-D sensor](images/Screenshot-sht31-sensor-panel.png?raw=true "Info Panel")
* `SystemDetailsDisplay`: ![Details display](images/Screenshot-details-panel.png?raw=true "Info Panel")
* `VindriktningAirQuality`: ![Details display](images/Screenshot-vindriktning-panel.png?raw=true "Info Panel")
* `WifiDisplay`: ![Details display](images/Screenshot-wifi-status-panel.png?raw=true "Info Panel")
* `WifiSetup`:  ![Details display](images/Screenshot-wifi-setup-panel.png?raw=true "Info Panel")
