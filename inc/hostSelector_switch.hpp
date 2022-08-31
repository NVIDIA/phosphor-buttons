#pragma once
#include "config.h"

#include "button_factory.hpp"
#include "button_interface.hpp"
#include "common.hpp"
#include "gpio.hpp"
#include "xyz/openbmc_project/Chassis/Buttons/HostSelector/server.hpp"
#include "xyz/openbmc_project/Chassis/Common/error.hpp"

#include <unistd.h>

#include <nlohmann/json.hpp>
#include <phosphor-logging/elog-errors.hpp>

#include <fstream>
#include <iostream>

static constexpr std::string_view HOST_SELECTOR = "HOST_SELECTOR";

static constexpr auto INVALID_INDEX = std::numeric_limits<size_t>::max();

enum class GpioState
{
    low,
    high
};

class HostSelector final :
    public sdbusplus::server::object_t<
        sdbusplus::xyz::openbmc_project::Chassis::Buttons::server::
            HostSelector>,
    public ButtonIface
{
  public:
    HostSelector(sdbusplus::bus::bus& bus, const char* path,
                 buttonConfig& buttonCfg, boost::asio::io_service& io) :
        sdbusplus::server::object_t<sdbusplus::xyz::openbmc_project::Chassis::
                                        Buttons::server::HostSelector>(
            bus, path, action::defer_emit),
        ButtonIface(bus, buttonCfg, io)
    {
        init();
        // read and store the host selector position Map
        hsPosMap = buttonCfg.extraJsonInfo.at("host_selector_map")
                       .get<std::map<std::string, int>>();
        maxPosition(buttonCfg.extraJsonInfo["max_position"], true);
        gpioLineCount = buttonCfg.gpios.size();
        setInitialHostSelectorValue();
        emit_object_added();
    }

    ~HostSelector()
    {
        deInit();
    }

    static constexpr std::string_view getFormFactorName()
    {
        return HOST_SELECTOR;
    }

    static const char* getDbusObjectPath()
    {
        return HS_DBUS_OBJECT_NAME;
    }
    void handleEvent(bool asserted, std::string gpio_name) override;
    size_t getMappedHSConfig(size_t hsPosition);
    size_t getGpioIndex(std::string gpio_name);
    void setInitialHostSelectorValue(void);
    void setHostSelectorValue(std::string gpio_name, GpioState state);

  protected:
    size_t hostSelectorPosition = 0;
    size_t gpioLineCount;

    // map of read Host selector switch value and corresponding host number
    // value.
    std::map<std::string, int> hsPosMap;
};
