
#pragma once
#include "button_factory.hpp"
#include "button_interface.hpp"
#include "common.hpp"
#include "config.hpp"
#include "gpio.hpp"
#include "xyz/openbmc_project/Chassis/Buttons/HostSelector/server.hpp"
#include "xyz/openbmc_project/Chassis/Common/error.hpp"
#include "xyz/openbmc_project/Inventory/Item/server.hpp"

#include <unistd.h>

#include <phosphor-logging/elog-errors.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
static constexpr auto DEBUG_CARD_PRESENT_GPIO = "debug_card_present";
static constexpr auto SERIAL_CONSOLE_SWITCH = "SERIAL_UART_MUX";

class SerialUartMux final : public ButtonIface
{
  public:
    SerialUartMux(sdbusplus::bus_t& bus, [[maybe_unused]] const char* path,
                  EventPtr& event, ButtonConfig& buttonCfg) :
        ButtonIface(bus, event, buttonCfg)
    {
        init();

        // read the platform specific config of host number to uart mux map
        std::unordered_map<std::string, size_t> uartMuxMapJson =
            buttonCfg.extraJsonInfo.at("serial_uart_mux_map")
                .get<decltype(uartMuxMapJson)>();
        for (auto& [key, value] : uartMuxMapJson)
        {
            auto index = std::stoi(key);
            serialUartMuxMap[index] = value;
        }
        if (buttonCfg.gpios.size() < 3)
        {
            throw std::runtime_error("not enough gpio configs found");
        }

        for (auto& gpio : buttonCfg.gpios)
        {
            if (gpio.name == DEBUG_CARD_PRESENT_GPIO)
            {
                debugCardPresentGpio = gpio;
                break;
            }
        }

        gpioLineCount = buttonCfg.gpios.size() - 1;
    }

    ~SerialUartMux()
    {
        deInit();
    }
    void init() override;
    static constexpr std::string getFormFactorName()
    {
        return SERIAL_CONSOLE_SWITCH;
    }
    static constexpr std::string getDbusObjectPath()
    {
        return "NO_DBUS_OBJECT";
    }

    void hostSelectorPositionChanged(sdbusplus::message_t& msg);
    void configSerialConsoleMux(size_t position);
    bool isOCPDebugCardPresent();

    void handleEvent(sd_event_source*, int, uint32_t) override {}

  protected:
    size_t gpioLineCount;
    std::unique_ptr<sdbusplus::match> hostPositionChanged;
    GpioInfo debugCardPresentGpio;
    std::unordered_map<size_t, size_t> serialUartMuxMap;
};
