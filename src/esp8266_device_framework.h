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

    /**
     * @mainpage esp8266_device_framework Detailed Documentation
     *
     * @section intro_sec Introduction
     * A generic device framework using the esp8266_web_settings library
     *
     * This framework supports settings, control, and publishing of multiple ESP8266-attached devices
     * in concert with the `WebSettings` class from esp_web_settings, https://github.com/grmcdorman/esp8266_web_settings.
     *
     * @section overview_sec Overview
     *
     * The base class for Devices is @ref grmcdorman::device::Device "Device".
     * The framework contains seven Devices:
     * - @ref grmcdorman::device::InfoDisplay "InfoDisplay"
     * - @ref grmcdorman::device::MqttPublisher "MqttPublisher"
     * - @ref grmcdorman::device::Sht31Sensor "Sht31Sensor"
     * - @ref grmcdorman::device::SystemDetailsDisplay "SystemDetailsDisplay"
     * - @ref grmcdorman::device::VindriktningAirQuality "VindriktningAirQuality"
     * - @ref grmcdorman::device::WifiDisplay "WifiDisplay"
     * - @ref grmcdorman::device::WifiSetup "WifiSetup"
     *
     * The VindriktningAirQuality class is derived from work by Hypfer's GitHub project, https://github.com/Hypfer/esp8266-vindriktning-particle-sensor.
     * All work on message deciphering comes from that project.
     *
     * An additional helper class, @ref grmcdorman::device::ConfigFile "ConfigFile", provides basic JSON configuration
     * file loading and saving for the list of devices.
     *
     * The sketch using the devices must have two strings, a firmware name, and a system identifier.
     * The former is a static `PROGMEM` string identifying the name or purpose of the sketch; the
     * latter should be a unique name for the specific ESP device; typically this can be a derivative
     * of the firmware name and the ESP chip ID, in hex.
     *
     *
     * In the sketch `setup()` function, the following operations should be performed, in sequence,
     * on the list of devices; this assumes you a global `std::vector<Device *> devices`, a global
     * configuration object `ConfigFile config`, and a global `WebSettings web_settings`.
     *
     * - Initialization: @code{.cpp}
     * for (auto &device: devices)
     * {
     *     device->set_system_identifiers(FPSTR(firmware_name), identifier);
     *     device->set_defaults();
     * }
     * @endcode
     * - Optionally, set some defaults different from the built-in ones: @code{.cpp}
     * // Device index # 0 is InfoDisplay. It has no relevant settings.
     * // Device index # 1 is System Details Display. It has no relevant settings.
     * // Device index # 2 is WiFi display. It has no relevant settings.
     * // Device index # 3 is the WiFi setup. A default AP and password could be set:
     * devices[3]->set("ssid", "my access point");
     * // Note that this will *not* be shown in the web page UI.
     * devices[3]->set("password", "my password");
     *
     * // Device index # 4 is the SHT31-D. Set the default sda to D2, scl to D3.
     * devices[4]->set("sda", "D2");
     * devices[4]->set("scl", "D3");
     * @endcode
     * - load settings: @code{.cpp}
     * config.load(devices);
     * @endcode
     * - Setup devices and add to the web settings: @code{.cpp}
     * for (auto &device : devices)
     * {
     *     device->setup();
     *     device->set_devices(devices);
     *     webServer.add_setting_set(device->name(), device->identifier(), device->get_settings());
     * }
     * @endcode
     *
     * In the sketch `loop()` function, each device's `loop()` method must be called: @code{.cpp}
     * for (auto &device : devices)
     * {
     *     device->loop();
     * }
     * @endcode
     *
     * Finally, in the `on_save` callback from the `WebSettings` class, the device configurations must be saved: @code{.cpp}
     * void on_save(::grmcdorman::WebSettings &)
     * {
     *     config.save(devices);
     * }
     * @endcode
     */

// This file only includes the basic Device headers. Include other Devices as per your needs.

#include "grmcdorman/device/Device.h"