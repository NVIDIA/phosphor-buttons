#pragma once
#include "config.hpp"
#include "cpld.hpp"
#include "gpio.hpp"

#include <nlohmann/json.hpp>

#include <iostream>

enum class ConfigType
{
    gpio,
    cpld
};

// this struct represents button interface
struct ButtonConfig
{
    ConfigType type;
    std::string formFactorName;   // name of the button interface
    std::vector<GpioInfo> gpios;  // holds single or group gpio config
    CpldInfo cpld;                // holds single cpld config
    std::vector<int> fds;         // store all the fds listen io event which
                                  // mapped with the gpio or cpld
    nlohmann::json extraJsonInfo; // corresponding to button interface
};
