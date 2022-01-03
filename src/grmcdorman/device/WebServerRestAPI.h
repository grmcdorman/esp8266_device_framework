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

#include "grmcdorman/device/Device.h"

class AsyncWebServer;
class AsyncWebServerRequest;

namespace grmcdorman::device
{
    class Device;

    /**
     * @brief This class provides a REST API for the attached devices.
     *
     * This leverages the `Definitions` list in the Devices to create the API end points,
     * and the `publish` method to serve requests.
     */
    class WebServerRestApi
    {
    public:
        WebServerRestApi();
        /**
         * @brief Set defaults.
         *
         *
         */
        void set_defaults();

        void setup(AsyncWebServer &server);

        /**
         * @brief Add the devices to serve
         *
         * This must be called before `setup`, so the appropriate APIs
         * can be constructed.
         *
         * @param list  List of devices to manage.
         */
        void set_devices(const std::vector<Device *> &list)
        {
            devices = &list;
        }
    private:
        void handle_on_device_get(AsyncWebServerRequest *request, const Device *device);

        const std::vector<Device *> *devices;
    };
}