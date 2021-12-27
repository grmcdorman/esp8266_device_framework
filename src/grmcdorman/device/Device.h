#pragma once

#include <WString.h>
#include <vector>
#include <ArduinoJson.h>

#include "grmcdorman/Setting.h"

namespace grmcdorman::device
{
    class SettingInterface;

    /**
     * @brief The generic device interface.
     *
     * A Device, generally, is something that needs to perform non-blocking operations in the main
     * system loop, and can optionally publish to MQTT.
     *
     * The device contains zero or more Descriptions, which are the individual sensors published to MQTT.
     * It also contains zero or more Settings from the esp8266_web_settings library; these provide
     * descriptions, configuration inputs, and status outputs.
     */
    class Device
    {
        public:
            /**
             * @brief Construct a new Device object.
             *
             * @param device_name       Device name.
             * @param device_identifier Device identifier.
             */
            Device(const __FlashStringHelper *device_name, const __FlashStringHelper *device_identifier);
            /**
             * @brief Destroy the Device object.
             *
             */
            virtual ~Device() {}

            static constexpr int D0 = 16; //!< D0 is GPIO16, HIGH at boot, not suitable for most usages
            static constexpr int D1 =  5; //!< D1 is GPIO5; often used as SCL
            static constexpr int D2 =  4; //!< D2 is GPIO4; often used as SDA
            static constexpr int D3 =  0; //!< D3 is GPIO0; pulled up; connected to FLASH button; not for input
            static constexpr int D4 =  2; //!< D4 is GPIO2; pulled up; HIGH at boot; on-board LED; not for input
            static constexpr int D5 = 14; //!< D5 is GPIO14; SPI (SCLK)
            static constexpr int D6 = 12; //!< D6 is GPIO12; SPI (MISO)
            static constexpr int D7 = 13; //!< D7 is GPIO13; SPI (MOSI)
            static constexpr int D8 = 15; //!< D8 is GPIO15; pulled to GND; SPI (CS); not recommended

            /**
             * @brief A sensor definition to be published to MQTT.
             *
             * This allows MQTT listeners, such as Home Assistant, to automatically discover and
             * recognize the sensor. It also provides the definitions to be used for publishing
             * the device data.
             */
            class Definition
            {
                public:
                    /**
                     * @brief Get the name suffix.
                     *
                     * The unique ESP identifer is suffixed with this to generate a unique
                     * human-readable device name.
                     *
                     * @return Device name.
                     */
                    virtual const __FlashStringHelper *get_name_suffix() const = 0;
                    /**
                     * @brief Get the value template.
                     *
                     * The value template describes which JSON attribute corresponds to
                     * the sensor's measurement value.
                     *
                     * @return Value template.
                     */
                    virtual const __FlashStringHelper *get_value_template() const = 0;

                    /**
                     * @brief Get the unique id suffix.
                     *
                     * The unique ESP identifier is suffixed with this to generate a unique
                     * system-wide sensor identifier. Unlike the `name` value, this is not
                     * human-readable and should follow identifier rules.
                     *
                     * @return Unique id suffix.
                     */
                    virtual const __FlashStringHelper *get_unique_id_suffix() const = 0;
                    /**
                     * @brief Get the unit of measurement.
                     *
                     * The units for the value, e.g. dBm. Most systems support UTF-8,
                     * allowing characters like the degree symbol.
                     *
                     * @return Units string.
                     */
                    virtual const __FlashStringHelper *get_unit_of_measurement() const = 0;
                    /**
                     * @brief Get the json attributes template.
                     *
                     * Attributes are additional values other than the primary value;
                     * this describes where they are in the JSON. This should be a null
                     * (`nullptr`) if there are no attributes.
                     *
                     * Examples include SSID for the WiFi RSSI value.
                     *
                     * @return Template for attributes.
                     */
                    virtual const __FlashStringHelper *get_json_attributes_template() const = 0;
                    /**
                     * @brief Get the icon.
                     *
                     * This is the icon to be used by applications like Home Assistant.
                     * Examples include "mdi:wifi".
                     *
                     * @return Icon string.
                     */
                    virtual const __FlashStringHelper *get_icon() const = 0;
            };

            /**
             * @brief Get the device name; used for UI names and IDs.
             *
             * @return Device name.
             */
            const __FlashStringHelper *name() const
            {
                return device_name;
            }

            /**
             * @brief Get the device identifier.
             *
             * This should be a unique identifier to be used in
             * internal contexts, such as settings files.
             *
             * @return Device identifier.
             */
            virtual const __FlashStringHelper *identifier() const
            {
                return device_identifier;
            }

            /**
             * @brief Set the system identifier values.
             *
             * This must be called before `load`; it sets default values. The
             * system identifier is used for default values for things such as
             * host names and SoftAP SSIDs.
             *
             * The firmware name is a unique identifier for the firmware, or application,
             * using the device.
             *
             * The system identifier is a unique identifier for the specific board running
             * the application. This is typically constructed at least in part from the
             * hex value of `ESP.getChipId()` or equivalent. If empty, an identifier is
             * constructed by combining the firmware prefix, a hypen, and the ESP chip ID in hex.
             *
             * Because these values are system wide, this is a static method.
             *
             * If this is never called, the firmware prefix is `unspecified_firmware` and
             * the system identifier is constructed as described above.
             *
             * @param firmware_name_value   The unique firmware prefix.
             * @param system_identifier_value The system identifier.
             */
            static void set_system_identifiers(const __FlashStringHelper *firmware_name_value, const String &system_identifier_value = String());

            /**
             * @brief Get the firmware prefix.
             *
             * @return Firmware prefix.
             */
            static const __FlashStringHelper *get_firmware_name()
            {
                return firmware_name;
            }

            /**
             * @brief Get the system identifier.
             *
             * @return This is the value set, or generated, when `set_system_identifiers` is called.
             */
            static const String &get_system_identifier()
            {
                return system_identifier;
            }

            /**
             * @brief Set defaults, if necessary.
             *
             * This must be called before loading any settings. It will,
             * if necessary, set any defaults that are dynamic; typically
             * these will be values that need the firmware prefix or
             * system identifier.
             */
            virtual void set_defaults()
            {
            }

            /**
             * @brief For devices that support it, add devices to manage.
             *
             * @param list  List of devices to manage.
             */
            virtual void set_devices(const std::vector<Device *> &list)
            {
            }

            /**
             * @brief Setup the device.
             *
             * Call once when the system boots. This should perform
             * device-specific setup, if the device is enabled, as well
             * as preparing UI settings (notably info settings) for communication.
             *
             * This must be called after initial values are loaded.
             */
            virtual void setup() = 0;

            /**
             * @brief Main loop.
             *
             * This is called in the main application loop. This should avoid blocking
             * as doing so will prevent the other devices from running.
             *
             * An exception is a "device" which requires user interaction, e.g.
             * WiFi association, before the rest of the system can run.
             */
            virtual void loop() = 0;

            /**
             * @brief Publish the value and attributes.
             *
             * The values and attributes for all sensors, i.e. all items
             * listed in the Description instances, should added to the
             * supplied JSON object. If the value is not available, or
             * this device does not publish, return `false`.
             *
             * The value, or values, and attributes, if any, are to be added
             * as a single node to the `json` parameter. This includes the
             * case for multiple sensors.
             *
             * @param[in,out] json JSON to receive the device values and attributes.
             * @return `true` if a value to be published was added; `false` otherwise.
             */
            virtual bool publish(DynamicJsonDocument &json) = 0;

            /**
             * @brief The type containing a list of Definition objects.
             *
             */
            typedef std::vector<const Definition *> definition_list_t;

            /**
             * @brief Get the definitions list.
             *
             * This returns a static definition list of zero or more sensor
             * definitions.
             *
             * @return A reference to the definition list.
             */
            const definition_list_t &get_definitions() const
            {
                return definitions;
            }

            /**
             * @brief Get the settings list.
             *
             * This returns a list of zero or more individual settings for
             * this device.
             *
             * @return A reference to the settings list.
             */
            const ::grmcdorman::SettingInterface::settings_list_t &get_settings() const
            {
                return settings;
            }

            /**
             * @brief Get whether this device is enabled.
             *
             * @return true     The device is enabled.
             * @return false    The device is disabled.
             */
            bool is_enabled() const
            {
                return enabled.get();
            }

            /**
             * @brief Set whether this device is enabled.
             *
             * @param state     The new enabled/disabled state.
             */
            void set_enabled(bool state)
            {
                enabled.set(state);
            }

            static const ExclusiveOptionSetting::names_list_t data_line_names;  //!< Names for each configurable data line; see `settingsMap`.
        protected:
            /**
             * @brief Initialize the definition and setting lists.
             *
             * The lists are moved to the private class attributes.
             *
             * @param definition_list   Device sensor definition list, for MQTT publishing.
             * @param setting_list      Device settings, for use by the WebSettings library.
             */
            void initialize(definition_list_t &&definition_list, ::grmcdorman::SettingInterface::settings_list_t &&setting_list)
            {
                definitions = std::move(definition_list);
                settings = std::move(setting_list);
            }

            /**
             * @brief Convert a data line index to a ESP data line.
             *
             * The index is simply an offset into the settingsMap array,
             * which contains usable data lines from the D0 ... D8 set
             * defined above.
             *
             * @param index     Index into the settings map.
             * @return int      The data line. An out of range index will return D1.
             */
            static int index_to_dataline(int index)
            {
                return settingsMap[index];
            }

            /**
             * @brief Convert a data line to an index.
             *
             * This will convert a data line, such as D1,
             * into an index into the settings map.
             * @param dataLine  The data line.
             * @return int      An index into the settings map. An unknown data line will return an index of 0, corresponding to D1.
             */
            static int dataline_to_index(int dataLine);

            /**
             * @brief The set of data lines usable for communication.
             *
             * This array contains the usable data lines - D1, D2, D3, D5, D6, and D7.
             * It thus is used to translate between indexes in, for example, settings
             * and the internal data line constants.
             */
            static const int settingsMap[6];

            ToggleSetting enabled;                                      //!< Whether this device is enabled.
        private:
            const __FlashStringHelper *device_name;                     //!< The device name, from the constructor.
            const __FlashStringHelper *device_identifier;               //!< The device identifier, from the constructor.

            static const __FlashStringHelper *firmware_name;            //!< The unique firmware prefix.
            static String system_identifier;                            //!< The unique system identifier.


            definition_list_t definitions;                              //!< The list of definitions. Set by `initialize`.
            ::grmcdorman::SettingInterface::settings_list_t settings;   //!< The list of settiongs. Set by `initialize`.
    };
}