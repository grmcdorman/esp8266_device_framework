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

#include <memory>
#include <PubSubClient.h>
#include <Ticker.h>

#include "grmcdorman/device/Device.h"
#include "grmcdorman/Setting.h"

class Client;

namespace grmcdorman::device
{
    /**
     * @brief This class supports generic MQTT publishing.
     *
     * SSL connections are not currently supported.
     *
     * It does not support subscriptions.
     *
     * If the connection is lost, an immediate attempt to connect is made on the next publish attempt.
     *
     * When a connection is attempted, the device will attempt `CONNECTION_TRIES` at an interval of `CONNECTION_RETRY_INTERVAL`. If this
     * fails, it will not attempt a connection again until the configured reconnection interval elapses.
     */
    class MqttPublisher: public Device
    {
        public:
            static constexpr uint16_t CONNECTION_TRIES = 5;             //!< Number of times to try establishing a connection.
            static constexpr uint32_t CONNECTION_RETRY_INTERVAL = 5;    //!< Second between attempts to retry establishing a connection.
            /**
             * @brief Construct a new Mqtt Device object.
             *
             * The manufacturer, model, and software version are used when publishing auto-discovery to
             * Home Assistant. For example, if your device is a home-built weather station, then you could
             * supply your name as the manufacturer and the model as "Weather Station".
             *
             * The software version is not restricted to standard naming conventions; one simple solution
             * is to use the predefined __DATE__ and __TIME__ macros. Doing so would give a simple unique
             * string to every build.
             *
             * The communication client, if provided, should be an appropriate client to be used for
             * network communications. If not provided, a WiFiClient is created.
             *
             * @param manufacturer      The IoT device manufacturer.
             * @param model             The IoT device model.
             * @param software_version  The firmware version.
             * @param client            The communication client for the PubSubClient class.
             */
            MqttPublisher(const __FlashStringHelper *manufacturer, const __FlashStringHelper *model, const __FlashStringHelper *software_version, Client *client = nullptr);

            /**
             * @brief Set defaults.
             *
             * This sets the MQTT identifier to the system identifier,
             * and the MQTT topic prefix to the firmware identifier.
             *
             */
            void set_defaults() override;

            void setup() override;
            void loop() override;

            DynamicJsonDocument as_json() const override;

            /**
             * @brief Add a list of devices that will publish.
             *
             * When the MQTT device publishes, per the timing set in its configuration,
             * it will poll each of these devices for publish data. If any device returns
             * data, a publish operation will be performed.
             *
             * No attempt is made to segregate device data; the devices must be
             * configured such that each has unique keys in the JSON to be published.
             *
             * If no devices are added, no publishing occurs.
             *
             * @param list      List of devices to manage.
             */
            virtual void set_devices(const std::vector<Device *> &list) override
            {
                devices = &list;
            }

            /**
             * @brief Get a status report.
             *
             * This is also used for the `device_status` info message, with the exception
             * of various disabled states.
             *
             * When the MQTT publisher is disabled or nonoperational (e.g. no MQTT server configured),
             * this returns an empty string.
             *
             * @return String containing status report.
             */
            virtual String get_status() const;

        private:
            /**
             * @brief Reconnect to MQTT.
             *
             * This is used at startup, and when an MQTT disconnect is detected and
             * the reconnect interval has been exceeded.
             */
            void reconnect();
            /**
             * @brief Publish Home Assistant automatic configuration.
             *
             * This uses data from each attached device to construct a series
             * of publish notifications that describe the device's sensors to
             * Home Assistant.
             */
            void publish_auto_config();
            /**
             * @brief Publish.
             *
             * This will poll each attached device for publish data;
             * if any device indicates it has data to publish, the
             * complete data from all devices must be published.
             */
            void publish();

            /**
             * @brief Get the MQTT string for the state.
             *
             * This will return an appropriate string
             * based on the error state supplied.
             *
             * @param state     The PubSubClient state.
             */
            String get_error_state_message(int state) const;

            /**
             * @brief Set up the publish timer.
             *
             * This does not set the connect timer; that's done when it's noticed
             * that the connection needs to be established.
             */
            void set_timer();

            const __FlashStringHelper *publish_manufacturer;    //<! The manufacturer from the constructor.
            const __FlashStringHelper *publish_model;           //<! The model from the constructor.
            const __FlashStringHelper *publish_software_version;//!< The sofware version from the constructor.

            const std::vector<Device *> *devices = nullptr; //!< The list of attached devices that will be published.

            std::unique_ptr<Client> default_client;         //!< The default communication client, when no client is provided. Created on demand if needed.
            Client                 *client;                 //!< The in-use client.
            std::unique_ptr<PubSubClient> mqttClient;       //!< The MQTT client.
            uint16_t retry_count = 0;                       //!< When connecting, the number of retries.
            uint32_t previous_connection_attempt_ms = 0;    //!< The last connection attempt, system time.
            uint32_t previous_publish_ms = 0;               //!< The last publication, system time.
            bool tried_publish = false;                     //!< Set to true on the first publish attempt.
            bool last_publish_failed = false;               //!< Success/fail for last data publish.
            int last_state = -1;                            //!< Last known MQTT state.
            uint32_t current_publish_seconds = 0;           //<! Current publish interval.

            String topicAvailability;                       //!< The availability topic string. Used when connecting.
            String topicState;                              //!< The state topic string. Used when connecting.
            String topicCommand;                            //!< The command - i.e. data publish - topic string.

            Ticker connect_ticker;                          //!< Timer for checking connection & reconnecting.
            Ticker publish_ticker;                          //!< Timer for publishing.
            NoteSetting notes;                              //!< A note setting with a description of the MQTT device.
            StringSetting server_address;                   //!< The user-configured server address.
            UnsignedIntegerSetting server_port;             //!< The user-configured server port.
            UnsignedIntegerSetting update_interval;         //!< How often to update (i.e. publish), in seconds.
            UnsignedIntegerSetting reconnect_interval;      //!< How often to try reconnecting, in seconds.
            UnsignedIntegerSetting keepalive_interval;      //!< The keep-alive interval.
            UnsignedIntegerSetting buffer_size;             //!< The buffer size in MQTT.
            StringSetting username;                         //!< If applicable, the MQTT user name.
            PasswordSetting password;                       //!< If applicable, the MQTT password.
            StringSetting prefix;                           //!< The prefix for topics.
            StringSetting identifier;                       //!< The unique identifier for topics.
            InfoSettingHtml device_status;                    //!< Output only; last update information.
    };
}