/*
// Copyright (c) 2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#pragma once

#include <boost/asio/io_service.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <gpiod.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>

#include <string>
#include <vector>

// this struct has the gpio config for single gpio
struct gpioInfo
{
    std::string button_name;
    std::string gpio_name;
    int direction;
    gpiod::line line;
    std::shared_ptr<boost::asio::posix::stream_descriptor> streamDesc;
    void* userdata;
    std::function<void(void*, bool, std::string)> handler;

    gpioInfo(const std::string button_name, const std::string gpio_name,
             const std::string direction) :
        button_name(button_name), gpio_name(gpio_name), streamDesc(nullptr),
        userdata(nullptr), handler(nullptr)
    {
        setDirection(direction);
    }

    gpioInfo(const gpioInfo& src) :
        button_name(src.button_name), gpio_name(src.gpio_name),
        direction(src.direction), line(src.line), streamDesc(src.streamDesc),
        userdata(src.userdata), handler(src.handler)
    {}

    ~gpioInfo()
    {
        if (streamDesc.get() != NULL)
        {
            streamDesc.reset();
        }
    }

    void setDirection(std::string dirStr)
    {
        // Determine edge type. Default is falling edge means asserted.
        auto dir = gpiod::line_event::FALLING_EDGE;
        std::for_each(dirStr.begin(), dirStr.end(),
                      [](char& c) { c = ::tolower(c); });
        if (dirStr == "rising")
        {
            dir = gpiod::line_event::RISING_EDGE;
        }
        this->direction = dir;
    }
};

// this struct represents button interface
struct buttonConfig
{
    std::string formFactorName;   // name of the button interface
    std::vector<gpioInfo> gpios;  // holds single or group gpio config
    nlohmann::json extraJsonInfo; // corresponding to button interface
};

/**
 * @brief iterates over the list of gpios and configures gpios them
 * config which is set from gpio defs json file.
 * The fd of the configured gpio is stored in buttonConfig.gpios container
 * @return int returns 0 on successful config of all gpios
 */

int configGroupGpio(buttonConfig& buttonIFConfig);

/**
 * @brief  configures and initializes the single gpio
 * @return int returns 0 on successful config of all gpios
 */

int configGpio(gpioInfo& gpioConfig);

uint32_t getGpioNum(const std::string& gpioPin);
void closeAllGpio();
void closeGpio(const std::string& buttonName);
// global json object which holds gpio_defs.json configs
extern nlohmann::json gpioDefs;
