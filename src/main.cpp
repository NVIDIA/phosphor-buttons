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

#include "button_factory.hpp"
#include "gpio.hpp"
#include "xyz/openbmc_project/Chassis/Buttons/Reset/server.hpp"

#include <boost/asio/io_service.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <gpiod.hpp>
#include <nlohmann/json.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <fstream>
static constexpr auto gpioDefFile = "/etc/default/obmc/gpio/gpio_defs.json";

nlohmann::json gpioDefs;
boost::asio::io_service io;

int main(void)
{
    int ret = 0;

    phosphor::logging::log<phosphor::logging::level::INFO>(
        "Start Phosphor buttons service...");

    std::shared_ptr<sdbusplus::asio::connection> conn =
        std::make_shared<sdbusplus::asio::connection>(io);

    sdbusplus::bus::bus bus = sdbusplus::bus::new_default();
    sdbusplus::server::manager::manager objManager{
        bus, "/xyz/openbmc_project/Chassis/Buttons"};

    bus.request_name("xyz.openbmc_project.Chassis.Buttons");
    std::vector<std::unique_ptr<ButtonIface>> buttonInterfaces;
    std::vector<buttonConfig> allBtnCfgs;

    std::ifstream gpios{gpioDefFile};
    auto gpioDefJson = nlohmann::json::parse(gpios, nullptr, true);
    gpioDefs = gpioDefJson["gpio_definitions"];

    // load gpio config from gpio defs json file and create button interface
    // objects based on the button form factor type
    for (const auto& gpioConfig : gpioDefs)
    {
        std::string formFactorName = gpioConfig["name"];
        buttonConfig buttonCfg;
        buttonCfg.formFactorName = formFactorName;
        buttonCfg.extraJsonInfo = gpioConfig;

        /* The folloing code checks if the gpio config read
        from json file is single gpio config or group gpio config,
        based on that further data is processed. */
        if (gpioConfig.contains("group_gpio_config"))
        {
            const auto& groupGpio = gpioConfig["group_gpio_config"];

            for (const auto& config : groupGpio)
            {
                gpioInfo gpioCfg = gpioInfo{config["name"], config["gpio_name"],
                                            config["direction"]};
                buttonCfg.gpios.push_back(gpioCfg);
            }
        }
        else
        {
            gpioInfo gpioCfg = gpioInfo{gpioConfig["name"],
                                        gpioConfig["gpio_name"],
                                        gpioConfig["direction"]};
            buttonCfg.gpios.push_back(gpioCfg);
        }

        // Call button factory to create instances of this button
        auto tempButtonIf = ButtonFactory::instance().createInstance(
            formFactorName, bus, buttonCfg, io);

        /* There are additional gpio configs present in some platforms
         that are not supported in phosphor-buttons.
        But they may be used by other applications. so skipping such configs
        if present in gpio_defs.json file*/
        if (tempButtonIf)
        {
            buttonInterfaces.emplace_back(std::move(tempButtonIf));
        }
        allBtnCfgs.push_back(std::move(buttonCfg));
    }
    phosphor::logging::log<phosphor::logging::level::INFO>(
        "Finished configuring buttons.");

    try
    {
        // Start asynchronous processes (blocking function)
        io.run();
    }
    catch (const std::exception& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(e.what());
        ret = -1;
    }
    // Close all potential gpio lines
    std::for_each(allBtnCfgs.begin(), allBtnCfgs.end(),
                  [](auto& cfg) { cfg.gpios.clear(); });
    return ret;
}
