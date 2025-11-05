#include "host_then_chassis_poweroff.hpp"

#include "config.hpp"
#include "power_button_profile_factory.hpp"

#include <phosphor-logging/lg2.hpp>
#include <xyz/openbmc_project/State/BMC/server.hpp>
#include <xyz/openbmc_project/State/Chassis/server.hpp>

namespace phosphor::button
{

// Register the profile with the factory.
static PowerButtonProfileRegister<HostThenChassisPowerOff> profileRegister;

namespace service
{
constexpr auto bmcState = "xyz.openbmc_project.State.BMC";
constexpr auto chassisState = "xyz.openbmc_project.State.Chassis0";
constexpr auto hostState = "xyz.openbmc_project.State.Host";
} // namespace service

namespace object_path
{
constexpr auto bmcState = "/xyz/openbmc_project/state/bmc0";
constexpr auto chassisState = "/xyz/openbmc_project/state/chassis0";
constexpr auto hostState = "/xyz/openbmc_project/state/host0";
} // namespace object_path

namespace interface
{
constexpr auto property = "org.freedesktop.DBus.Properties";
constexpr auto bmcState = "xyz.openbmc_project.State.BMC";
constexpr auto chassisState = "xyz.openbmc_project.State.Chassis";
constexpr auto hostState = "xyz.openbmc_project.State.Host";
} // namespace interface

using namespace sdbusplus::xyz::openbmc_project::State::server;

void HostThenChassisPowerOff::pressed()
{
    lg2::info("Power button pressed");

    try
    {
        // If power not on - power on
        if (!isPoweredOn())
        {
            if (!isBmcReady())
            {
                lg2::error("BMC not at ready state yet, cannot power on");
                return;
            }

            state = PowerOpState::powerOnPress;
            powerOn();
            return;
        }
    }
    catch (const sdbusplus::exception_t& e)
    {
        lg2::error(
            "Exception while processing power button press. Cannot continue");
        return;
    }

    // Power is on ...

    if (state == PowerOpState::buttonNotPressed)
    {
        lg2::info("Starting countdown to power off");
        state = PowerOpState::buttonPressed;
        setHostOffTime();
        timer.restart(pollInterval);
    }

    // Button press during host off to chassis off window.
    // Causes a chassis power off.
    else if (state == PowerOpState::buttonReleasedHostToChassisOffWindow)
    {
        lg2::info("Starting chassis power off due to button press");
        state = PowerOpState::chassisOffStarted;
        timer.setEnabled(false);
        chassisPowerOff();
    }
}

void HostThenChassisPowerOff::released(uint64_t /*pressTimeMS*/)
{
    lg2::info("Power button released");

    // Button released in the host to chassis off window.
    // Timer continues to run in case button is pressed again
    // in the window.
    if (state == PowerOpState::buttonPressedHostOffStarted)
    {
        state = PowerOpState::buttonReleasedHostToChassisOffWindow;
        return;
    }

    state = PowerOpState::buttonNotPressed;
    timer.setEnabled(false);
}

void HostThenChassisPowerOff::timerHandler()
{
    const auto now = std::chrono::steady_clock::now();

    if ((state == PowerOpState::buttonPressed) && (now >= hostOffTime))
    {
        // Start the host power off and start the chassis
        // power off countdown.
        state = PowerOpState::buttonPressedHostOffStarted;
        setChassisOffTime();
        hostPowerOff();
    }
    else if ((state == PowerOpState::buttonPressedHostOffStarted) &&
             (now >= chassisOffTime))
    {
        // Button still pressed and it passed the chassis off time.
        // Issue the chassis power off.
        state = PowerOpState::chassisOffStarted;
        timer.setEnabled(false);
        chassisPowerOff();
    }
}

void HostThenChassisPowerOff::hostTransition(Host::Transition transition)
{
    try
    {
        std::variant<std::string> state = convertForMessage(transition);

        lg2::info("Power button action requesting host transition of {TRANS}",
                  "TRANS", std::get<std::string>(state));

        auto method =
            bus.new_method_call(service::hostState, object_path::hostState,
                                interface::property, "Set");
        method.append(interface::hostState, "RequestedHostTransition", state);

        bus.call(method);
    }
    catch (const sdbusplus::exception_t& e)
    {
        lg2::error("Failed requesting host transition {TRANS}: {ERROR}",
                   "TRANS", convertForMessage(transition), "ERROR", e);
    }
}

void HostThenChassisPowerOff::powerOn()
{
    hostTransition(Host::Transition::On);
}

void HostThenChassisPowerOff::hostPowerOff()
{
    hostTransition(Host::Transition::Off);
}

void HostThenChassisPowerOff::chassisPowerOff()
{
    lg2::info("Power button action requesting chassis power off");

    try
    {
        std::variant<std::string> state =
            convertForMessage(Chassis::Transition::Off);

        auto method = bus.new_method_call(service::chassisState,
                                          object_path::chassisState,
                                          interface::property, "Set");
        method.append(interface::chassisState, "RequestedPowerTransition",
                      state);

        bus.call(method);
    }
    catch (const sdbusplus::exception_t& e)
    {
        lg2::error("Failed requesting chassis off: {ERROR}", "ERROR", e);
    }
}

bool HostThenChassisPowerOff::isPoweredOn() const
{
    Chassis::PowerState chassisState;

    try
    {
        auto method = bus.new_method_call(service::chassisState,
                                          object_path::chassisState,
                                          interface::property, "Get");
        method.append(interface::chassisState, "CurrentPowerState");
        auto result = bus.call(method);

        auto state = result.unpack<std::variant<std::string>>();

        chassisState =
            Chassis::convertPowerStateFromString(std::get<std::string>(state));
    }
    catch (const sdbusplus::exception_t& e)
    {
        lg2::error("Failed checking if chassis is on: {ERROR}", "ERROR", e);
        throw;
    }

    return chassisState == Chassis::PowerState::On;
}

bool HostThenChassisPowerOff::isBmcReady() const
{
    BMC::BMCState bmcState;

    try
    {
        auto method =
            bus.new_method_call(service::bmcState, object_path::bmcState,
                                interface::property, "Get");
        method.append(interface::bmcState, "CurrentBMCState");
        auto result = bus.call(method);

        auto state = result.unpack<std::variant<std::string>>();

        bmcState = BMC::convertBMCStateFromString(std::get<std::string>(state));
    }
    catch (const sdbusplus::exception_t& e)
    {
        lg2::error("Failed reading BMC state interface: {}", "ERROR", e);
        throw;
    }

    return bmcState == BMC::BMCState::Ready;
}
} // namespace phosphor::button
