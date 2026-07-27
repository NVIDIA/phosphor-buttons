
#pragma once
#include "config.hpp"

#include <cstdint>
#include <string>

struct ButtonConfig;

struct CpldInfo
{
    std::string registerName;
    uint32_t i2cAddress;
    uint32_t i2cBus;
    int cpldMappedFd; // io fd mapped with the cpld
};

int configCpld(ButtonConfig& buttonCfg);
