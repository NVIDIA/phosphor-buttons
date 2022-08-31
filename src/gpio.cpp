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

#include "config.h"

#include "gpio.hpp"

#include "xyz/openbmc_project/Chassis/Common/error.hpp"

#include <error.h>
#include <fcntl.h>
#include <unistd.h>

#include <gpiod.hpp>
#include <gpioplus/utility/aspeed.hpp>
#include <nlohmann/json.hpp>
#include <phosphor-logging/lg2.hpp>

#include <filesystem>
#include <fstream>

const std::string gpioDev = "/sys/class/gpio";
std::map<std::string, gpiod::line> allGpios;

namespace fs = std::filesystem;

void closeAllGpio()
{
    // Loop through all gpios and close them
    std::string msg = "Closing all button gpio lines.";
    lg2::info(msg.c_str());
    while (allGpios.size() != 0)
    {
        auto it = allGpios.begin();
        closeGpio(it->first);
    }
}

void closeGpio(const std::string& buttonName)
{
    // Find the GPIO line
    gpiod::line gpioLine;
    try
    {
        gpioLine = allGpios.at(buttonName);
    }
    catch (const std::exception& e)
    {
        std::string errMsg = "Button name not found: " + buttonName;
        lg2::error(errMsg.c_str());
        return;
    }
    if (!gpioLine)
    {
        std::string errMsg = "Failed to find the " + buttonName + " line";
        lg2::error(errMsg.c_str());
        return;
    }
    try
    {
        // Try to release the line
        if (gpioLine.is_used())
        {
            gpioLine.release();
        }
        // Erase the line from allGpios
        allGpios.erase(buttonName);
    }
    catch (const std::exception& e)
    {
        std::string errMsg = "Failed to release the " + buttonName + " line";
        lg2::error(errMsg.c_str());
        return;
    }
    std::string msg = "Released the " + buttonName + " line";
    lg2::info(msg.c_str());
}

uint32_t getGpioBase()
{
    // Look for a /sys/class/gpio/gpiochip*/label file
    // with a value of GPIO_BASE_LABEL_NAME.  Then read
    // the base value from the 'base' file in that directory.
#ifdef LOOKUP_GPIO_BASE
    for (auto& f : fs::directory_iterator(gpioDev))
    {
        std::string path{f.path()};
        if (path.find("gpiochip") == std::string::npos)
        {
            continue;
        }

        std::ifstream labelStream{path + "/label"};
        std::string label;
        labelStream >> label;

        if (label == GPIO_BASE_LABEL_NAME)
        {
            uint32_t base;
            std::ifstream baseStream{path + "/base"};
            baseStream >> base;
            return base;
        }
    }

    lg2::error("Could not find GPIO base");
    throw std::runtime_error("Could not find GPIO base!");
#else
    return 0;
#endif
}

int configGroupGpio(buttonConfig& buttonIFConfig)
{
    int result = 0;
    // iterate the list of gpios from the button interface config
    // and initialize them
    for (auto& gpioCfg : buttonIFConfig.gpios)
    {
        result = configGpio(gpioCfg);
        if (result < 0)
        {
            lg2::error("{NAME}: Error configuring gpio-{NUM}: {RESULT}", "NAME",
                       buttonIFConfig.formFactorName, "NUM", gpioCfg.gpio_name,
                       "RESULT", result);

            break;
        }
    }

    return result;
}

static void waitForGPIOEvent(gpioInfo& gpioConfig)
{
    // This is an async function that is called upon a change
    // to the gpio that is being watched
    gpioConfig.streamDesc.get()->async_wait(
        boost::asio::posix::stream_descriptor::wait_read,
        [&gpioConfig](const boost::system::error_code ec) {
            if (gpioConfig.button_name == "")
            {
                lg2::error("Name for button is an empty string");
                throw sdbusplus::xyz::openbmc_project::Chassis::Common::Error::
                    IOError();
            }
            if (ec)
            {
                closeAllGpio();
                std::string errMsg = gpioConfig.button_name +
                                     " fd handler error: " + ec.message();
                lg2::error(errMsg.c_str());
                // TODO: throw here to force power-control to restart?
                throw sdbusplus::xyz::openbmc_project::Chassis::Common::Error::
                    IOError();
            }
            if (!gpioConfig.userdata)
            {
                closeAllGpio();
                std::string errMsg = "Failed to find the " +
                                     gpioConfig.button_name + " userdata";
                lg2::error(errMsg.c_str());
                throw sdbusplus::xyz::openbmc_project::Chassis::Common::Error::
                    IOError();
            }
            if (!gpioConfig.handler)
            {
                closeAllGpio();
                std::string errMsg =
                    "Failed to find the " + gpioConfig.button_name + " handler";
                lg2::error(errMsg.c_str());
                throw sdbusplus::xyz::openbmc_project::Chassis::Common::Error::
                    IOError();
            }
            gpiod::line_event line_event = gpioConfig.line.event_read();
            gpioConfig.handler(gpioConfig.userdata,
                               line_event.event_type == gpioConfig.direction,
                               gpioConfig.gpio_name);
            waitForGPIOEvent(gpioConfig);
        });
}

int configGpio(gpioInfo& gpioConfig)
{
    // Find the GPIO line
    gpioConfig.line = gpiod::find_line(gpioConfig.gpio_name);
    if (!gpioConfig.line)
    {
        std::string errMsg = "Failed to find the " + gpioConfig.gpio_name +
                             " line for " + gpioConfig.button_name;
        lg2::error(errMsg.c_str());
        return -1;
    }

    // Add this gpio to map of all gpios
    allGpios[gpioConfig.button_name] = gpioConfig.line;
    try
    {
        gpioConfig.line.request(
            {"button-handler", gpiod::line_request::EVENT_BOTH_EDGES, {}});
    }
    catch (const std::exception&)
    {
        closeAllGpio();
        std::string errMsg =
            "Failed to request events for " + gpioConfig.button_name;
        lg2::error(errMsg.c_str());
        return -1;
    }

    // Get a file descriptor to be used for this event
    int gpioLineFd = gpioConfig.line.event_get_fd();
    if (gpioLineFd < 0)
    {
        closeAllGpio();
        std::string errMsg = "Failed to name " + gpioConfig.button_name + " fd";
        lg2::error(errMsg.c_str());
        return -1;
    }
    try
    {
        // Assign the fd to this stream descriptor
        gpioConfig.streamDesc.get()->assign(gpioLineFd);
    }
    catch (const std::exception&)
    {
        closeAllGpio();
        std::string errMsg = "Failed assign line to stream descriptor for " +
                             gpioConfig.button_name;
        lg2::error(errMsg.c_str());
        return -1;
    }

    std::string msg = "Button GPIO configured: " + gpioConfig.button_name;
    lg2::info(msg.c_str());
    waitForGPIOEvent(gpioConfig);
    return 0;
}
