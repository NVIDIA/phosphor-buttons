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

#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>

#include <string>
#include <vector>

struct ButtonConfig;

// enum to represent gpio states
enum class GpioState
{
    assert,
    deassert,
    invalid
};
// enum to represent gpio polarity
enum class GpioPolarity
{
    activeLow,
    activeHigh
};

struct GPIOBufferValue
{
    char assert;
    char deassert;
};

// this struct has the gpio config for single gpio
struct GpioInfo
{
    int fd; // io fd mapped with the gpio
    uint32_t number;
    std::string name;
    std::string direction;
    GpioPolarity polarity;
};

/**
 * @brief iterates over the list of gpios and configures gpios them
 * config which is set from gpio defs json file.
 * The fd of the configured gpio is stored in ButtonConfig.gpios container
 * @return int returns 0 on successful config of all gpios
 */

int configGroupGpio(ButtonConfig& buttonCfg);

/**
 * @brief  configures and initializes the single gpio
 * @return int returns 0 on successful config of all gpios
 */

int configGpio(GpioInfo& gpioConfig, ButtonConfig& buttonIFConfig);

uint32_t getGpioNum(const std::string& gpioPin);

uint32_t getGlobalGpioNumberFromName(const std::string& lineName);
// Set gpio state based on polarity
void setGpioState(int fd, GpioPolarity polarity, GpioState state);
// Get gpio state based on polarity
GpioState getGpioState(int fd, GpioPolarity polarity);

// global json object which holds gpio_defs.json configs
extern nlohmann::json gpioDefs;
