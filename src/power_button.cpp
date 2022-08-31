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

#include "power_button.hpp"

// add the button iface class to registry
static ButtonIFRegister<PowerButton> buttonRegister;

void PowerButton::simPress()
{
    pressed();
}

void PowerButton::simLongPress()
{
    pressedLong();
}

void PowerButton::updatePressedTime()
{
    pressedTime = std::chrono::steady_clock::now();
}

auto PowerButton::getPressTime() const
{
    return pressedTime;
}

void PowerButton::handleEvent(bool asserted, std::string /* gpio_name */)
{
    if (asserted)
    {
        phosphor::logging::log<phosphor::logging::level::DEBUG>(
            "POWER_BUTTON: pressed");

        updatePressedTime();
        // emit pressed signal
        pressed();
    }
    else
    {
        phosphor::logging::log<phosphor::logging::level::DEBUG>(
            "POWER_BUTTON: released");

        auto now = std::chrono::steady_clock::now();
        auto d = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - getPressTime());

        if (d > std::chrono::milliseconds(LONG_PRESS_TIME_MS))
        {
            pressedLong();
        }
        else
        {
            // released
            released();
        }
    }
}
