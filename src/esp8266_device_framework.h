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
     * An additional helper class, @ref grmcdorman::device::ConfigFile "ConfigFile", provides basic JSON configuration
     * file loading and saving.
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
     * - load settings: @code{.cpp}
     * config.load(devices);
     * @endcode
     * - Setup devices and add to the web settings: @code{.cpp}
     * for (auto &device : devices)
     * {
     *     device->setup();
     *     device->set_devices(devices);
     *     webServer.add_setting_set(device->name(), device->get_settings());
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