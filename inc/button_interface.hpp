#pragma once

#include "common.hpp"
#include "gpio.hpp"
#include "xyz/openbmc_project/Chassis/Common/error.hpp"

#include <boost/asio/io_service.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/lg2.hpp>

// This is the base class for all the button interface types
//
class ButtonIface
{
  public:
    ButtonIface(sdbusplus::bus::bus& bus, buttonConfig& buttonCfg,
                boost::asio::io_service& io,
                const std::function<void(void*, bool, std::string)> handler =
                    ButtonIface::EventHandler) :
        bus(bus), config(buttonCfg), callbackHandler(handler)
    {
        int ret = -1;

        // Create a new stream descriptor for each gpio
        // Also give each gpio a callback handler and userdata pointer
        for (auto& config : config.gpios)
        {
            config.streamDesc =
                std::make_shared<boost::asio::posix::stream_descriptor>(io);
            config.handler = callbackHandler;
            config.userdata = (void*)this;
        }
        // config group gpio based on the gpio defs read from the json file
        ret = configGroupGpio(config);

        if (ret < 0)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                (getFormFactorType() + " : failed to config GPIO").c_str());
            throw sdbusplus::xyz::openbmc_project::Chassis::Common::Error::
                IOError();
        }
    }
    virtual ~ButtonIface() {}

    /**
     * @brief This method is called from sd-event provided callback function
     * callbackHandler if platform specific event handling is needed then a
     * derived class instance with its specific evend handling logic along with
     * init() function can be created to override the default event handling.
     */

    virtual void handleEvent(bool asserted, std::string gpio_name) = 0;
    static int EventHandler(void* userdata, bool asserted,
                            std::string gpio_name)
    {
        if (userdata)
        {
            ButtonIface* buttonIface = static_cast<ButtonIface*>(userdata);
            buttonIface->handleEvent(asserted, gpio_name);
        }

        return 0;
    }

    std::string getFormFactorType() const
    {
        return config.formFactorName;
    }

  protected:
    /**
     * @brief oem specific initialization can be done under init function.
     * if platform specific initialization is needed then
     * a derived class instance with its own init function to override the
     * default init() method can be added.
     */

    virtual void init() {}

    /**
     * @brief similar to init() oem specific deinitialization can be done under
     * deInit function. if platform specific deinitialization is needed then a
     * derived class instance with its own init function to override the default
     * deinit() method can be added.
     */
    virtual void deInit()
    {
        for (auto& gpioCfg : config.gpios)
        {
            ::closeGpio(gpioCfg.button_name);
        }
    }

    sdbusplus::bus::bus& bus;
    buttonConfig config;
    const std::function<void(void*, bool, std::string)> callbackHandler;
};
