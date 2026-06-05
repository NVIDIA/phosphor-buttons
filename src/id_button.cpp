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

#include "id_button.hpp"

// add the button iface class to registry
static ButtonIFRegister<IDButton> buttonRegister;

void IDButton::simPress()
{
    pressed();
}

#if UID_BUTTON_FUNCTION
void IDButton::updatePressedTime()
{
    pressedTime = std::chrono::steady_clock::now();
}

auto IDButton::getPressTime() const
{
    return pressedTime;
}
#endif

void IDButton::handleEvent(bool asserted, std::string /* gpio_name */)
{
    if (asserted)
    {
        phosphor::logging::log<phosphor::logging::level::DEBUG>(
            (getFormFactorType() + " : pressed").c_str());
#if UID_BUTTON_FUNCTION
        updatePressedTime();
#endif
        // emit pressed signal
        pressed();
    }
    else
    {
        phosphor::logging::log<phosphor::logging::level::DEBUG>(
            (getFormFactorType() + " : released").c_str());
#if UID_BUTTON_FUNCTION
        // A long press (>= password reset threshold) is reported as a separate
        // signal carrying the press duration, so button-handler can pick the
        // password vs factory reset action. Shorter presses keep the existing
        // behaviour (toggling the ID LED group).
        auto d = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - getPressTime());
        if (d >= std::chrono::milliseconds(UID_PASSWORD_RESET_TIME_MSEC))
        {
            pressedLong(static_cast<uint64_t>(d.count()));
        }
        else
        {
            released();
        }
#else
        // released
        released();
#endif
    }
}
