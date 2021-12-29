
#include "grmcdorman/device/MqttPublisher.h"
#include "grmcdorman/Setting.h"

#include <ESP8266WiFi.h>

namespace grmcdorman::device
{
    namespace
    {
        const char * AVAILABILITY_ONLINE = "online";
        const char * AVAILABILITY_OFFLINE = "offline";
        const char mqtt_name[] PROGMEM = "MQTT";
        const char mqtt_identifier[] PROGMEM = "mqtt_publisher";
    }
    MqttPublisher::MqttPublisher(const __FlashStringHelper *manufacturer, const __FlashStringHelper *model, const __FlashStringHelper *software_version, Client *client):
        Device(FPSTR(mqtt_name), FPSTR(mqtt_identifier)),
        publish_manufacturer(manufacturer),
        publish_model(model),
        publish_software_version(software_version),
        default_client(client == nullptr ? new WiFiClient() : nullptr),
        client(client == nullptr ? default_client.get() : nullptr),
        notes(F("These settings configure the MQTT server.<br>"
        "A blank server name disables MQTT operations.<br>"
        "MQTT topics will be:"
        "<ul>"
        "<li><em>prefix</em>/<em>identifier</em>/status"
        "<li><em>prefix</em>/<em>identifier</em>/state"
        "<li><em>prefix</em>/<em>identifier</em>/command"
        "</ul>"
        "Corresponding Home Assistant configurations will be published.")),
        server_address(F("MQTT server name or IP"), F("server")),
        server_port(F("MQTT server port (standard 1883)"), F("port")),
        update_interval(F("Interval to publish data (seconds)"), F("update")),
        reconnect_interval(F("How often to try reconnecting (seconds)"), F("reconnect")),
        keepalive_interval(F("Keep alive timer (seconds)"), F("keepalive")),
        buffer_size(F("MQTT buffer size (bytes)"), F("buffer_size")),
        username(F("The MQTT username (blank for anonymous)"), F("username")),
        password(F("The MQTT password"), F("password")),
        prefix(F("MQTT topic prefix"), F("prefix")),
        identifier(F("MQTT client ID and topic identifier"), F("identifier")),
        last_update(F("Last update<script>periodicUpdateList.push(\"mqtt_publisher&setting=last_update\");</script>"), F("last_update"))
    {
        initialize({}, {&notes, &server_address, &server_port, &update_interval, &reconnect_interval,
            &keepalive_interval, &buffer_size,
            &username, &password, &prefix, &identifier, &last_update, &enabled});
        server_port.set(1883);
        update_interval.set(30);
        reconnect_interval.set(60);
        keepalive_interval.set(30);
        buffer_size.set(2048);

        // Unlike other devices, this is disabled by default.
        set_enabled(false);

        last_update.set_request_callback([this] (const InfoSettingHtml &)
        {
            if (!is_enabled())
            {
                last_update.set(F("MQTT is disabled"));
                return;
            }

            if (mqttClient == nullptr)
            {
                last_update.set(F("MQTT was disabled at boot; reboot to enable"));
                return;
            }

            if (server_address.get().isEmpty())
            {
                last_update.set(F("No server is configured"));
                return;
            }

            if (devices == nullptr)
            {
                last_update.set(F("No devices attached for publishing"));
                return;
            }

            last_update.set(get_status());
        });
    }

    void MqttPublisher::set_defaults()
    {
        identifier.set(get_system_identifier());
        prefix.set(get_firmware_name());
    }

    bool MqttPublisher::publish(DynamicJsonDocument &)
    {
        // This device does not publish.
        return false;
    }

    void MqttPublisher::setup()
    {
        // Arrange to publish & connect immediately.
        previous_publish_ms = millis() - update_interval.get() * 1000;
        previous_connection_attempt_ms = millis() - reconnect_interval.get() * 1000;
        topicAvailability.reserve(prefix.get().length() + 1 + identifier.get().length() + sizeof("/status"));
        topicAvailability = prefix.get();
        topicAvailability += '/';
        topicAvailability += identifier.get();
        topicAvailability += F("/status");

        topicState.reserve(prefix.get().length() + 1 + identifier.get().length() + sizeof("/state"));
        topicState = prefix.get();
        topicState += '/';
        topicState += identifier.get();
        topicState += F("/state");

        if (is_enabled() && !server_address.get().isEmpty())
        {
            mqttClient.reset(new PubSubClient(*client));
            mqttClient->setServer(server_address.get().c_str(), server_port.get());
            mqttClient->setKeepAlive(keepalive_interval.get());
            mqttClient->setBufferSize(buffer_size.get());
        }
    }

    void MqttPublisher::loop()
    {
        // While it is possible to detect that the settings have changed such
        // that it's possible to connect, other settings are not applied when changed;
        // it's more consistent to require reboot for any change at the moment.
        if (!is_enabled() || server_address.get().isEmpty() || !mqttClient)
        {
            return;
        }

        mqttClient->loop();

        const uint32_t currentMillis = millis();

        // Try to connect before publishing.
        if (!mqttClient->connected() && currentMillis - previous_connection_attempt_ms >= reconnect_interval.get() * 1000) {
            previous_connection_attempt_ms = currentMillis;
            reconnect();
        }

        last_state = mqttClient->state();

        if (currentMillis - previous_publish_ms >= update_interval.get() * 1000) {
            // This does not check 'connected'.
            previous_publish_ms = currentMillis;
            publish();
        }

    }
    void MqttPublisher::reconnect()
    {
        if (!is_enabled() && !server_address.get().isEmpty() && !mqttClient)
        {
            setup();
        }

        if (!is_enabled() || server_address.get().isEmpty() || !mqttClient)
        {
            return;
        }

        bool connected = false;
        if (username.get().isEmpty()) {
            connected = mqttClient->connect(identifier.get().c_str(), topicAvailability.c_str(), 1, true, AVAILABILITY_OFFLINE);
        } else {
            connected = mqttClient->connect(identifier.get().c_str(), username.get().c_str(), password.get().c_str(), topicAvailability.c_str(), 1, true, AVAILABILITY_OFFLINE);
        }

        if (connected) {
            mqttClient->publish(topicAvailability.c_str(), AVAILABILITY_ONLINE, true);
            publish_auto_config();
        }
    }

    void MqttPublisher::publish_auto_config()
    {
        if (devices == nullptr)
        {
            return;
        }

        DynamicJsonDocument device_json(256);
        StaticJsonDocument<64> identifiersDoc;
        JsonArray identifiers = identifiersDoc.to<JsonArray>();

        identifiers.add(identifier.get());

        device_json["identifiers"] = identifiers;
        device_json["manufacturer"] =  publish_manufacturer;
        device_json["model"] = publish_model;
        device_json["name"] = identifier.get();
        device_json["sw_version"] =  publish_software_version;

        for (auto &device: *devices)
        {
            if (device->is_enabled())
            {
                for (auto &definition: device->get_definitions())
                {
                    DynamicJsonDocument autoconfPayload(1024);
                    autoconfPayload["device"] = device_json.as<JsonObject>();
                    autoconfPayload["availability_topic"] = topicAvailability;
                    autoconfPayload["state_topic"] = topicState;
                    autoconfPayload["name"] = identifier.get() + definition->get_name_suffix();
                    autoconfPayload["value_template"] = definition->get_value_template();
                    autoconfPayload["unique_id"] = identifier.get() + definition->get_unique_id_suffix();
                    autoconfPayload["unit_of_measurement"] =  definition->get_unit_of_measurement();
                    if (definition->get_json_attributes_template() != nullptr)
                    {
                        autoconfPayload["json_attributes_topic"] = topicState;
                        autoconfPayload["json_attributes_template"] = definition->get_json_attributes_template();
                    }
                    autoconfPayload["icon"] = definition->get_icon();
                    auto size = measureJson(autoconfPayload) + 1;
                    std::unique_ptr<char[]> buffer(new char[size]);
                    buffer.get()[0] = '\0';

                    ::serializeJson(autoconfPayload, buffer.get(), size);
                    String topic;
                    topic.reserve(sizeof ("homeassistant/sensor/") + prefix.get().length() +
                        1 +     // slash
                        identifier.get().length() +
                        strlen_P(reinterpret_cast<const char *>(definition->get_unique_id_suffix())) +
                        sizeof("/config"));

                    topic += "homeassistant/sensor/";
                    topic += prefix.get();
                    topic += '/';
                    topic += identifier.get();
                    topic += definition->get_unique_id_suffix();
                    topic += "/config";
                    mqttClient->publish(topic.c_str(), buffer.get(), true);
                    autoconfPayload.clear();
                }
            }
        }
    }

    void MqttPublisher::publish()
    {
        if (devices == nullptr)
        {
            return;
        }

        tried_publish = true;
        DynamicJsonDocument state_json(1024 * devices->size());
        for (auto &device : *devices)
        {
            device->publish(state_json);
        }

        auto size = measureJson(state_json) + 1;
        std::unique_ptr<char[]> buffer(new char[size]);
        buffer.get()[0] = '\0';

        ::serializeJson(state_json, buffer.get(), size);
        last_publish_failed = !mqttClient->publish(topicState.c_str(), buffer.get(), true);
   }

   String MqttPublisher::get_error_state_message(int state) const
   {
        if (mqttClient == nullptr)
        {
            return F("Disabled at boot");
        }

        String state_message;
        switch (state)
        {
            case MQTT_CONNECTION_TIMEOUT:
                state_message = F("server didn't respond within the keepalive time");
                break;

            case MQTT_CONNECTION_LOST:
                state_message = F("network connection was broken");
                break;

            case MQTT_CONNECT_FAILED:
                state_message = F("network connection failed");
                break;

            case MQTT_DISCONNECTED:
                state_message = F("client is disconnected cleanly");
                break;

            case MQTT_CONNECTED:
                state_message = F("connected");
                break;

            case MQTT_CONNECT_BAD_PROTOCOL:
                state_message = F("MQTT server doesn't support the requested version of MQTT");
                break;

            case MQTT_CONNECT_BAD_CLIENT_ID:
                state_message = F("server rejected the client identifier");
                break;

            case MQTT_CONNECT_UNAVAILABLE:
                state_message = F("server was unable to accept the connection");
                break;

            case MQTT_CONNECT_BAD_CREDENTIALS:
                state_message = F("the username and password were rejected");
                break;

            case MQTT_CONNECT_UNAUTHORIZED:
                state_message = F("client was not authorized to connect");
                break;

            default:
                state_message = F("Unknown MQTT PubSubClient error: ") + String(state);
                break;
        }

        return state_message;
    }

    String MqttPublisher::get_status() const
    {
        if (!is_enabled() ||
            mqttClient == nullptr ||
            server_address.get().isEmpty() ||
            devices == nullptr)
        {
            return String();
        }

        if (last_state != MQTT_CONNECTED)
        {
            auto timeSinceConnect = millis() - previous_connection_attempt_ms;
            String message("Last connection attempt ");
            message += timeSinceConnect / 1000;
            message += " seconds ago: ";
            message += get_error_state_message(last_state);
            return message;
        }

        if (tried_publish)
        {
            auto timeSincePublish = millis() - previous_publish_ms;
            String message(F("Last publish "));
            message += last_publish_failed ? F("failed ") : F("succeeded ");
            message += timeSincePublish / 1000;
            message += F(" seconds ago.");
            return message;
        }
        else
        {
            return F("Never published.");
        }

        // NOT REACHED
    }

}