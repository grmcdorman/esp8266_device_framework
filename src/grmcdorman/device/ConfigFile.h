#pragma once

#include <optional>
#include <vector>

#include <ArduinoJson.h>

namespace grmcdorman::device
{
    class Device;

    /**
     * @brief This class provides simple JSON configuration file save/load.
     *
     *
     */
    class ConfigFile
    {
    public:
        /**
         * @brief Construct a new Config File object
         *
         * This will have the default file path, `/config.json`.
         *
         */
        ConfigFile(): ConfigFile("/config.json")
        {
        }

        /**
         * @brief Construct a new Config File object.
         *
         * @param explicit_path     The explicit path to the config file. Must be a persisent pointer; do not pass a `.c_str()` value.
         *
         */
        explicit ConfigFile(const char *explicit_path): path(explicit_path)
        {
        }

        /**
         * @brief Get the path.
         *
         * @return Configuration file path.
         */
        const char *get_path() const
        {
            return path;
        }

        /**
         * @brief Save all device settings.
         *
         * This saves all applicable device settings. If
         * there are no devices with savable settings,
         * no config file is saved.
         *
         * @param devices   The set of devices to save.
         */
        void save(const std::vector<Device *> &devices);

        /**
         * @brief Load all device settings.
         *
         * This loads all applicable device settings.
         *
         * @param devices   The set of devices to save.
         * @return `true` if settings were loaded.
         */
        bool load(const std::vector<Device *> &devices);

        /**
         * @brief Save the JSON settings to the file system.
         *
         * @param json  JSON to save.
         */
        void save(DynamicJsonDocument &json);

        /**
         * @brief Load the JSON settings from the file system.
         *
         * If the settings cannot be retrieved, or an error occurred,
         * an unset value is returned.
         *
         * @return The loaded settings, or unset if no settings can be loaded.
         */
        std::optional<DynamicJsonDocument> load();

    private:
        const char *path;       //!< The configuration file path.
    };
}
