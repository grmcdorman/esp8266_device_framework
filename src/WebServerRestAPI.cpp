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

#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <ESPAsyncWebServer.h>

#include "grmcdorman/device/WebServerRestAPI.h"

namespace grmcdorman::device
{
    WebServerRestApi::WebServerRestApi()
    {
    }

    void WebServerRestApi::setup(AsyncWebServer &server, const std::vector<Device *> &devices)
    {

        // The filters are required because the AsyncWebServer accepts any path
        // that *starts* with the URI.
        for (const auto &device: devices)
        {
            String path(F("/rest/device/"));
            path += device->identifier();
            path += F("/get");
            server.on(path.c_str(), HTTP_GET, [this, device] (AsyncWebServerRequest *request)
            {
                handle_on_device_get(request, device);
            }).setFilter([path] (AsyncWebServerRequest *request)
                {
                    return request->url() == path;
                }
            );
        }

        server.on("/rest/devices/get", HTTP_GET, [&devices]  (AsyncWebServerRequest *request)
        {
            auto response = new AsyncJsonResponse(true);
            auto & root = response->getRoot();
            for (const auto &device: devices)
            {
                root.add(device->identifier());
            }
            response->setLength();
            response->addHeader("Cache-Control", "no-cache");
            request->send(response);
        }).setFilter([] (AsyncWebServerRequest *request)
            {
                return request->url() == F("/rest/devices/get");
            }
        );
    }

    void WebServerRestApi::handle_on_device_get(AsyncWebServerRequest *request, const Device *device)
    {
        auto response = new AsyncJsonResponse(false);
        auto & root = response->getRoot();
        root[device->identifier()] =  device->as_json();
        response->setLength();
        response->addHeader("Cache-Control", "no-cache");
        request->send(response);
    }
}