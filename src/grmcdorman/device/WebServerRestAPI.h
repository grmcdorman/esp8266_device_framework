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
         * @brief Set up with the web server.
         *
         * This must be called after the devices have been added. The
         * URIs for the devices, and the device list URI, will be registered with
         * the server.
         *
         * The URIs added are:
         * - `/rest/devices/get` to return all devices
         * - `/rest/device/`_device-id_`/get` to return values for one device
         *
         * @param server    Web server to install API on.
         * @param devices   List of devices to install end-points for. A reference is held to this; this list must exist for the lifetime of this object.
         */
        void setup(AsyncWebServer &server, const std::vector<Device *> &devices);

    private:
        /**
         * @brief Handle a device-specific GET.
         *
         * This handles the URI `/rest/device/`_device-id_`/get`.
         *
         * @param request   Incoming web request.
         * @param device    Device associated with URI.
         */
        void handle_on_device_get(AsyncWebServerRequest *request, const Device *device);
    };
}