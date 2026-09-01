#pragma once
#include "power_button_profile.hpp"

#include <sdbusplus/bus/match.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>
#include <xyz/openbmc_project/State/Host/server.hpp>

#include <chrono>

namespace phosphor::button
{

/**
 * @class HostThenChassisPowerOff
 *
 * A custom power button handler that will do the following:
 *
 * If power is off:
 *  - A button press will power on as long as the BMC is
 *    in the ready state.
 *
 * If power is on:
 *  - A button press less than 4s won't do anything.
 *  - At 4s, issue a host power off and start a 10s timer.
 *    - If the button is released within that 10s and not pressed
 *      again, continue with the host power off.
 *    - If the button is released within that 10s and also
 *      pressed again in that 10s, do a hard power (chassis)
 *      off.
 *    - If the button is pressed throughout that 10s
 *      issue a hard power off.
 */
class HostThenChassisPowerOff : public PowerButtonProfile
{
  public:
    enum class PowerOpState
    {
        powerOnPress,
        buttonNotPressed,
        buttonPressed,
        buttonPressedHostOffStarted,
        buttonReleasedHostToChassisOffWindow,
        chassisOffStarted
    };

    /**
     * @brief Constructor
     * @param[in] bus - The sdbusplus bus object
     */
    explicit HostThenChassisPowerOff(sdbusplus::bus_t& bus) :
        PowerButtonProfile(bus), state(PowerOpState::buttonNotPressed),
        timer(bus.get_event(),
              std::bind(&HostThenChassisPowerOff::timerHandler, this),
              pollInterval)
    {
        timer.setEnabled(false);
    }

    /**
     * @brief Returns the name that matches the value in
     *        meson_options.txt.
     */
    static constexpr std::string getName()
    {
        return "host_then_chassis_poweroff";
    }

    HostThenChassisPowerOff() = delete;
    ~HostThenChassisPowerOff() = default;

    /**
     * @brief Called when the power button is pressed.
     */
    virtual void pressed() override;

    /**
     * @brief Called when the power button is released.
     *
     * @param[in] pressTimeMS - How long the button was pressed
     *                          in milliseconds.
     */
    virtual void released(uint64_t pressTimeMS) override;

  private:
    /**
     * @brief Determines if the BMC is in the ready state.
     * @return bool If the BMC is in the ready state
     */
    bool isBmcReady() const;

    /**
     * @brief Determines if system (chassis) is powered on.
     *
     * @return bool - If power is on
     */
    bool isPoweredOn() const;

    /**
     * @brief Requests a host state transition
     * @param[in] transition - The transition (like On or Off)
     */
    void hostTransition(
        sdbusplus::xyz::openbmc_project::State::server::Host::Transition
            transition);

    /**
     * @brief Powers on the system
     */
    void powerOn();

    /**
     * @brief Requests a host power off
     */
    void hostPowerOff();

    /**
     * @brief Requests a chassis power off
     */
    void chassisPowerOff();

    /**
     * @brief The handler for the 1s timer that runs when determining
     *        how to power off.
     *
     * A 1 second timer is used so that there is the ability to emit
     * a power off countdown if necessary.
     */
    void timerHandler();

    /**
     * @brief Sets the time the host will be powered off if the
     *        button is still pressed - 4 seconds in the future.
     */
    inline void setHostOffTime()
    {
        hostOffTime = std::chrono::steady_clock::now() + hostOffInterval;
    }

    /**
     * @brief Sets the time the chassis will be powered off if the
     *        button is still pressed or pressed again - 10 seconds
     *        in the future.
     */
    inline void setChassisOffTime()
    {
        chassisOffTime = std::chrono::steady_clock::now() + chassisOffInterval;
    }

    /**
     * @brief The interval the timer handler is called at.
     */
    static constexpr std::chrono::milliseconds pollInterval{1000};

    /**
     * @brief Default button hold down interval constant
     */
    static constexpr std::chrono::milliseconds hostOffInterval{4000};

    /**
     * @brief The time between a host power off and chassis power off.
     */
    static constexpr std::chrono::milliseconds chassisOffInterval{10000};

    /**
     * @brief The current state of the handler.
     */
    PowerOpState state;

    /**
     * @brief When the host will be powered off.
     */
    std::chrono::time_point<std::chrono::steady_clock> hostOffTime;

    /**
     * @brief When the chassis will be powered off.
     */
    std::chrono::time_point<std::chrono::steady_clock> chassisOffTime;

    /**
     * @brief The timer object.
     */
    sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic> timer;
};
} // namespace phosphor::button
