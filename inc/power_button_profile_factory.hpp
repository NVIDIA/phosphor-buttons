#pragma once

#include "config.hpp"
#include "power_button_profile.hpp"

#include <memory>
#include <unordered_map>

namespace phosphor::button
{

using powerButtonProfileCreator =
    std::function<std::unique_ptr<PowerButtonProfile>(sdbusplus::bus_t& bus)>;

/**
 * @class PowerButtonProfileFactory
 *
 * Creates the custom power button profile class if one is set with
 * the 'power-button-profile' meson option.
 *
 * The createProfile() method will return a nullptr if no custom
 * profile is enabled.
 */
class PowerButtonProfileFactory
{
  public:
    static PowerButtonProfileFactory& instance()
    {
        static PowerButtonProfileFactory factory;
        return factory;
    }

    template <typename T>
    void addToRegistry()
    {
        profileRegistry[T::getName()] = [](sdbusplus::bus_t& bus) {
            return std::make_unique<T>(bus);
        };
    }

    std::unique_ptr<PowerButtonProfile> createProfile(sdbusplus::bus_t& bus)
    {
        // Find the creator method named after the
        // 'power-button-profile' option value.
        auto objectIter = profileRegistry.find(POWER_BUTTON_PROFILE);
        if (objectIter != profileRegistry.end())
        {
            return objectIter->second(bus);
        }
        else
        {
            return nullptr;
        }
    }

  private:
    PowerButtonProfileFactory() = default;

    std::unordered_map<std::string, powerButtonProfileCreator> profileRegistry;
};

/**
 * @brief Registers a power button profile with the factory.
 *
 * Declare a static instance of this at the top of the profile
 * .cpp file like:
 *    static PowerButtonProfileRegister<MyClass> register;
 */
template <class T>
class PowerButtonProfileRegister
{
  public:
    PowerButtonProfileRegister()
    {
        PowerButtonProfileFactory::instance().addToRegistry<T>();
    }
};
} // namespace phosphor::button
