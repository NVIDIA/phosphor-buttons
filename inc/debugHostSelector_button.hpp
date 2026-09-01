
#pragma once
#include "button_factory.hpp"
#include "button_interface.hpp"
#include "common.hpp"
#include "config.hpp"
#include "gpio.hpp"
#include "xyz/openbmc_project/Chassis/Buttons/Button/server.hpp"
#include "xyz/openbmc_project/Chassis/Common/error.hpp"

#include <unistd.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/lg2.hpp>

static constexpr auto DEBUG_SELECTOR_BUTTON = "DEBUG_SELECTOR_BUTTON";

class DebugHostSelector final :
    public sdbusplus::server::object_t<
        sdbusplus::xyz::openbmc_project::Chassis::Buttons::server::Button>,
    public ButtonIface

{
  public:
    DebugHostSelector(sdbusplus::bus_t& bus, const char* path, EventPtr& event,
                      ButtonConfig& buttonCfg) :
        sdbusplus::server::object_t<
            sdbusplus::xyz::openbmc_project::Chassis::Buttons::server::Button>(
            bus, path, action::defer_emit),
        ButtonIface(bus, event, buttonCfg)
    {
        init();
        emit_object_added();
    }

    ~DebugHostSelector()
    {
        deInit();
    }

    void simPress() override;
    void simRelease() override;
    void simLongPress() override;
    void handleEvent(sd_event_source* es, int fd, uint32_t revents) override;

    static constexpr std::string getFormFactorName()
    {
        return DEBUG_SELECTOR_BUTTON;
    }

    static constexpr std::string getDbusObjectPath()
    {
        return DBG_HS_DBUS_OBJECT_NAME;
    }
};
