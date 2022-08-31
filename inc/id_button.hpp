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
#include "config.h"

#include "button_factory.hpp"
#include "button_interface.hpp"
#include "common.hpp"
#include "gpio.hpp"
#include "xyz/openbmc_project/Chassis/Buttons/ID/server.hpp"
#include "xyz/openbmc_project/Chassis/Common/error.hpp"

#include <unistd.h>

#include <boost/asio/io_service.hpp>
#include <phosphor-logging/elog-errors.hpp>

static constexpr std::string_view ID_BUTTON = "ID_BTN";

class IDButton :
    public sdbusplus::server::object::object<
        sdbusplus::xyz::openbmc_project::Chassis::Buttons::server::ID>,
    public ButtonIface
{
  public:
    IDButton(sdbusplus::bus::bus& bus, const char* path,
             buttonConfig& buttonCfg, boost::asio::io_service& io) :
        sdbusplus::server::object::object<
            sdbusplus::xyz::openbmc_project::Chassis::Buttons::server::ID>(
            bus, path),
        ButtonIface(bus, buttonCfg, io)
    {
        init();
    }

    ~IDButton()
    {
        deInit();
    }

    void simPress() override;

    void handleEvent(bool asserted, std::string /* gpio_name */) override;

    static constexpr std::string_view getFormFactorName()
    {
        return ID_BUTTON;
    }
    static constexpr const char* getDbusObjectPath()
    {
        return ID_DBUS_OBJECT_NAME;
    }
};
