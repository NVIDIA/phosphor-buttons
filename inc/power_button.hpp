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
#include "button_factory.hpp"
#include "button_interface.hpp"
#include "common.hpp"
#include "gpio.hpp"
#include "xyz/openbmc_project/Chassis/Buttons/Power/server.hpp"
#include "xyz/openbmc_project/Chassis/Common/error.hpp"

#include <unistd.h>

#include <chrono>
#include <phosphor-logging/elog-errors.hpp>

static constexpr std::string_view POWER_BUTTON = "POWER_BUTTON";

class PowerButton
    : public sdbusplus::server::object::object<
          sdbusplus::xyz::openbmc_project::Chassis::Buttons::server::Power>,
      public ButtonIface
{
  public:
    PowerButton(sdbusplus::bus::bus& bus, const char* path, EventPtr& event,
                buttonConfig& buttonCfg) :
        sdbusplus::server::object::object<
            sdbusplus::xyz::openbmc_project::Chassis::Buttons::server::Power>(
            bus, path),
        ButtonIface(bus, event, buttonCfg)
    {
        init();
    }

    ~PowerButton()
    {

        deInit();
    }

    void simPress() override;
    void simLongPress() override;

    static constexpr std::string_view getFormFactorName()
    {
        return POWER_BUTTON;
    }
    static constexpr const char* getDbusObjectPath()
    {
        return POWER_DBUS_OBJECT_NAME;
    }
    void updatePressedTime();
    auto getPressTime() const;
    void handleEvent(sd_event_source* es, int fd, uint32_t revents) override;

  protected:
    decltype(std::chrono::steady_clock::now()) pressedTime;
};
