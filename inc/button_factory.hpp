#pragma once

#include "button_interface.hpp"
#include "gpio.hpp"

#include <phosphor-logging/elog-errors.hpp>

#include <unordered_map>

using buttonIfCreatorMethod = std::function<std::unique_ptr<ButtonIface>(
    sdbusplus::bus::bus& bus, buttonConfig& buttonCfg,
    boost::asio::io_service& io)>;

/**
 * @brief This is abstract factory for the creating phosphor buttons objects
 * based on the button  / formfactor type given.
 */

class ButtonFactory
{
  public:
    static ButtonFactory& instance()
    {
        static ButtonFactory buttonFactoryObj;
        return buttonFactoryObj;
    }

    /**
     * @brief this method creates a key and value pair element
     * for the given button interface where key is the form factor
     * name and the value is lambda method to return
     * the instance of the button interface.
     * This key value pair is stored in the Map buttonIfaceRegistry.
     */

    template <typename T>
    void addToRegistry()
    {
        buttonIfaceRegistry[std::string(T::getFormFactorName())] =
            [](sdbusplus::bus::bus& bus, buttonConfig& buttonCfg,
               boost::asio::io_service& io) {
                return std::make_unique<T>(bus, T::getDbusObjectPath(),
                                           buttonCfg, io);
            };
    }
    /**
     * @brief this method returns the button interface object
     *    corresponding to the button formfactor name provided
     */
    std::unique_ptr<ButtonIface> createInstance(const std::string& name,
                                                sdbusplus::bus::bus& bus,
                                                buttonConfig& buttonCfg,
                                                boost::asio::io_service& io)
    {
        // find matching name in the registry and call factory method.
        auto objectIter = buttonIfaceRegistry.find(name);
        if (objectIter != buttonIfaceRegistry.end())
        {
            return objectIter->second(bus, buttonCfg, io);
        }
        else
        {
            return nullptr;
        }
    }

  private:
    // This map is the registry for keeping supported button interface types.
    std::unordered_map<std::string, buttonIfCreatorMethod> buttonIfaceRegistry;
};

template <class T>
class ButtonIFRegister
{
  public:
    ButtonIFRegister()
    {
        // register the class factory function
        ButtonFactory::instance().addToRegistry<T>();
    }
};