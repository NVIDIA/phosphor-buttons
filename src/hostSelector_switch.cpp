
#include "hostSelector_switch.hpp"

#include <error.h>

#include <phosphor-logging/lg2.hpp>

// add the button iface class to registry
static ButtonIFRegister<HostSelector> buttonRegister;

size_t HostSelector::getMappedHSConfig(size_t hsPosition)
{
    size_t adjustedPosition = INVALID_INDEX; // set bmc as default value
    std::string hsPosStr;
    hsPosStr = std::to_string(hsPosition);

    if (hsPosMap.find(hsPosStr) != hsPosMap.end())
    {
        adjustedPosition = hsPosMap[hsPosStr];
    }
    else
    {
        lg2::debug("getMappedHSConfig : {TYPE}: no valid value in map.", "TYPE",
                   getFormFactorType());
    }
    return adjustedPosition;
}

size_t HostSelector::getGpioIndex(std::string gpio_name)
{
    for (size_t index = 0; index < gpioLineCount; index++)
    {
        if (config.gpios[index].gpio_name == gpio_name)
        {
            return index;
        }
    }
    return INVALID_INDEX;
}

void HostSelector::setInitialHostSelectorValue()
{
    for (size_t index = 0; index < gpioLineCount; index++)
    {
        auto value = config.gpios[index].line.get_value();
        GpioState gpioState = (value == 0) ? (GpioState::low)
                                           : (GpioState::high);
        setHostSelectorValue(config.gpios[index].gpio_name, gpioState);
        size_t hsPosMapped = getMappedHSConfig(hostSelectorPosition);
        if (hsPosMapped != INVALID_INDEX)
        {
            position(hsPosMapped, true);
        }
    }
}

void HostSelector::setHostSelectorValue(std::string gpio_name, GpioState state)
{
    size_t pos = getGpioIndex(gpio_name);

    if (pos == INVALID_INDEX)
    {
        return;
    }
    auto set_bit = [](size_t& val, size_t n) { val |= 0xff & (1 << n); };

    auto clr_bit = [](size_t& val, size_t n) { val &= ~(0xff & (1 << n)); };

    auto bit_op = (state == GpioState::low) ? set_bit : clr_bit;

    bit_op(hostSelectorPosition, pos);
    return;
}
/**
 * @brief This method is called from sd-event provided callback function
 * callbackHandler if platform specific event handling is needed then a
 * derived class instance with its specific event handling logic along with
 * init() function can be created to override the default event handling
 */

void HostSelector::handleEvent(bool asserted, std::string gpio_name)
{
    // read the gpio state for the io event received
    GpioState gpioState = (asserted) ? (GpioState::low) : (GpioState::high);

    setHostSelectorValue(gpio_name, gpioState);

    size_t hsPosMapped = getMappedHSConfig(hostSelectorPosition);

    if (hsPosMapped != INVALID_INDEX)
    {
        position(hsPosMapped);
    }
}
