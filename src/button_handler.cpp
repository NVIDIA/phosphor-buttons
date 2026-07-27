#include "config.h"

#include "button_handler.hpp"

#include <phosphor-logging/lg2.hpp>
#include <xyz/openbmc_project/State/Chassis/server.hpp>
#include <xyz/openbmc_project/State/Host/server.hpp>

#if UID_BUTTON_FUNCTION
#include <security/pam_appl.h>

#include <cstdlib>
#include <cstring>
#include <span>
#endif
namespace phosphor
{
namespace button
{
namespace sdbusRule = sdbusplus::bus::match::rules;
using namespace sdbusplus::xyz::openbmc_project::State::server;

constexpr auto chassisIface = "xyz.openbmc_project.State.Chassis";
constexpr auto hostIface = "xyz.openbmc_project.State.Host";
constexpr auto powerButtonIface = "xyz.openbmc_project.Chassis.Buttons.Power";
constexpr auto idButtonIface = "xyz.openbmc_project.Chassis.Buttons.ID";
constexpr auto resetButtonIface = "xyz.openbmc_project.Chassis.Buttons.Reset";
constexpr auto ledGroupIface = "xyz.openbmc_project.Led.Group";
constexpr auto ledGroupBasePath = "/xyz/openbmc_project/led/groups/";
constexpr auto hostSelectorIface =
    "xyz.openbmc_project.Chassis.Buttons.HostSelector";
constexpr auto debugHostSelectorIface =
    "xyz.openbmc_project.Chassis.Buttons.DebugHostSelector";

constexpr auto propertyIface = "org.freedesktop.DBus.Properties";
constexpr auto mapperIface = "xyz.openbmc_project.ObjectMapper";

constexpr auto mapperObjPath = "/xyz/openbmc_project/object_mapper";
constexpr auto mapperService = "xyz.openbmc_project.ObjectMapper";
constexpr auto BMC_POSITION = 0;

Handler::Handler(sdbusplus::bus_t& bus) : bus(bus)
{
    try
    {
        if (!getService(POWER_DBUS_OBJECT_NAME, powerButtonIface).empty())
        {
            lg2::info("Starting power button handler");
            powerButtonReleased = std::make_unique<sdbusplus::bus::match_t>(
                bus,
                sdbusRule::type::signal() + sdbusRule::member("Released") +
                    sdbusRule::path(POWER_DBUS_OBJECT_NAME) +
                    sdbusRule::interface(powerButtonIface),
                std::bind(std::mem_fn(&Handler::powerReleased), this,
                          std::placeholders::_1));

            powerButtonLongPressed = std::make_unique<sdbusplus::bus::match_t>(
                bus,
                sdbusRule::type::signal() + sdbusRule::member("PressedLong") +
                    sdbusRule::path(POWER_DBUS_OBJECT_NAME) +
                    sdbusRule::interface(powerButtonIface),
                std::bind(std::mem_fn(&Handler::longPowerPressed), this,
                          std::placeholders::_1));
        }
    }
    catch (const sdbusplus::exception::exception& e)
    {
        // The button wasn't implemented
    }

    try
    {
        if (!getService(ID_DBUS_OBJECT_NAME, idButtonIface).empty())
        {
            lg2::info("Registering ID button handler");
            idButtonReleased = std::make_unique<sdbusplus::bus::match_t>(
                bus,
                sdbusRule::type::signal() + sdbusRule::member("Released") +
                    sdbusRule::path(ID_DBUS_OBJECT_NAME) +
                    sdbusRule::interface(idButtonIface),
                std::bind(std::mem_fn(&Handler::idReleased), this,
                          std::placeholders::_1));

#if UID_BUTTON_FUNCTION
            idButtonLongPressed = std::make_unique<sdbusplus::bus::match_t>(
                bus,
                sdbusRule::type::signal() + sdbusRule::member("PressedLong") +
                    sdbusRule::path(ID_DBUS_OBJECT_NAME) +
                    sdbusRule::interface(idButtonIface),
                std::bind(std::mem_fn(&Handler::idPressedLong), this,
                          std::placeholders::_1));
#endif
        }
    }
    catch (const sdbusplus::exception::exception& e)
    {
        // The button wasn't implemented
    }

    try
    {
        if (!getService(RESET_DBUS_OBJECT_NAME, resetButtonIface).empty())
        {
            lg2::info("Registering reset button handler");
            resetButtonReleased = std::make_unique<sdbusplus::bus::match_t>(
                bus,
                sdbusRule::type::signal() + sdbusRule::member("Released") +
                    sdbusRule::path(RESET_DBUS_OBJECT_NAME) +
                    sdbusRule::interface(resetButtonIface),
                std::bind(std::mem_fn(&Handler::resetReleased), this,
                          std::placeholders::_1));
        }
    }
    catch (const sdbusplus::exception::exception& e)
    {
        // The button wasn't implemented
    }
}
bool Handler::isMultiHost()
{
    // return true in case host selector object is available
    bool ret = false;
    try
    {
        ret = (!getService(HS_DBUS_OBJECT_NAME, hostSelectorIface).empty());
    }
    // If ResourceNotFound, return false. Rethrow other exceptions
    catch (const sdbusplus::exception::exception& e)
    {
        if (std::string{e.what()}.find("ResourceNotFound") != std::string::npos)
        {
            return false;
        }
        throw;
    }
    return ret;
}
std::string Handler::getService(const std::string& path,
                                const std::string& interface) const
{
    auto method = bus.new_method_call(mapperService, mapperObjPath, mapperIface,
                                      "GetObject");
    method.append(path, std::vector{interface});
    auto result = bus.call(method);

    std::map<std::string, std::vector<std::string>> objectData;
    result.read(objectData);

    return objectData.begin()->first;
}
size_t Handler::getHostSelectorValue()
{
    auto HSService = getService(HS_DBUS_OBJECT_NAME, hostSelectorIface);

    if (HSService.empty())
    {
        lg2::info("Host Selector dbus object not available");
        throw std::invalid_argument("Host selector dbus object not available");
    }

    try
    {
        auto method = bus.new_method_call(
            HSService.c_str(), HS_DBUS_OBJECT_NAME, propertyIface, "Get");
        method.append(hostSelectorIface, "Position");
        auto result = bus.call(method);

        std::variant<size_t> HSPositionVariant;
        result.read(HSPositionVariant);

        auto position = std::get<size_t>(HSPositionVariant);
        return position;
    }
    catch (const sdbusplus::exception::exception& e)
    {
        lg2::error("Error reading Host selector Position: {ERROR}", "ERROR", e);
        throw;
    }
}
bool Handler::poweredOn(size_t hostNumber) const
{
    auto chassisObjectName = CHASSIS_STATE_OBJECT_NAME +
                             std::to_string(hostNumber);
    auto service = getService(chassisObjectName.c_str(), chassisIface);
    auto method = bus.new_method_call(
        service.c_str(), chassisObjectName.c_str(), propertyIface, "Get");
    method.append(chassisIface, "CurrentPowerState");
    auto result = bus.call(method);

    std::variant<std::string> state;
    result.read(state);

    return Chassis::PowerState::On ==
           Chassis::convertPowerStateFromString(std::get<std::string>(state));
}

void Handler::handlePowerEvent(PowerEvent powerEventType)
{
    std::string objPathName;
    std::string dbusIfaceName;
    std::string transitionName;
    std::variant<Host::Transition, Chassis::Transition> transition;

    size_t hostNumber = 0;
    auto isMultiHostSystem = isMultiHost();
    if (isMultiHostSystem)
    {
        hostNumber = getHostSelectorValue();
        lg2::info("Multi host system detected : {POSITION}", "POSITION",
                  hostNumber);
    }

    std::string hostNumStr = std::to_string(hostNumber);

    // ignore  power and reset button events if BMC is selected.
    if (isMultiHostSystem && (hostNumber == BMC_POSITION) &&
        (powerEventType != PowerEvent::longPowerPressed))
    {
        lg2::info(
            "handlePowerEvent : BMC selected on multihost system. ignoring power and reset button events...");
        return;
    }

    switch (powerEventType)
    {
        case PowerEvent::powerReleased:
        {
            objPathName = HOST_STATE_OBJECT_NAME + hostNumStr;
            dbusIfaceName = hostIface;
            transitionName = "RequestedHostTransition";

            transition = Host::Transition::On;

            if (poweredOn(hostNumber))
            {
                transition = Host::Transition::Off;
            }
            lg2::info("handlePowerEvent : handle power button press ");

            break;
        }
        case PowerEvent::longPowerPressed:
        {
            dbusIfaceName = chassisIface;
            transitionName = "RequestedPowerTransition";
            objPathName = CHASSIS_STATE_OBJECT_NAME + hostNumStr;
            transition = Chassis::Transition::Off;

            /*  multi host system :
                    hosts (1 to N) - host shutdown
                    bmc (0) - sled cycle
                single host system :
                    host(0) - host shutdown
            */
            if (isMultiHostSystem && (hostNumber == BMC_POSITION))
            {
#if CHASSIS_SYSTEM_RESET_ENABLED
                objPathName = CHASSISSYSTEM_STATE_OBJECT_NAME + hostNumStr;
                transition = Chassis::Transition::PowerCycle;
#else
                return;
#endif
            }
            else if (!poweredOn(hostNumber))
            {
                lg2::info("Power is off so ignoring long power button press");
                return;
            }
            lg2::info("handlePowerEvent : handle long power button press");

            break;
        }

        case PowerEvent::resetReleased:
        {
            objPathName = HOST_STATE_OBJECT_NAME + hostNumStr;
            dbusIfaceName = hostIface;
            transitionName = "RequestedHostTransition";

            if (!poweredOn(hostNumber))
            {
                lg2::info("Power is off so ignoring reset button press");
                return;
            }

            lg2::info("Handling reset button press");
            transition = Host::Transition::Reboot;
            break;
        }
        default:
        {
            lg2::error("{EVENT} is invalid power event. skipping...", "EVENT",
                       static_cast<std::underlying_type_t<PowerEvent>>(
                           powerEventType));

            return;
        }
    }
    auto service = getService(objPathName.c_str(), dbusIfaceName);
    auto method = bus.new_method_call(service.c_str(), objPathName.c_str(),
                                      propertyIface, "Set");
    method.append(dbusIfaceName, transitionName, transition);
    bus.call(method);
}
void Handler::powerReleased(sdbusplus::message_t& /* msg */)
{
    try
    {
        handlePowerEvent(PowerEvent::powerReleased);
    }
    catch (const sdbusplus::exception::exception& e)
    {
        lg2::error("Failed power state change on a power button press: {ERROR}",
                   "ERROR", e);
    }
}
void Handler::longPowerPressed(sdbusplus::message_t& /* msg */)
{
    try
    {
        handlePowerEvent(PowerEvent::longPowerPressed);
    }
    catch (const sdbusplus::exception::exception& e)
    {
        lg2::error("Failed powering off on long power button press: {ERROR}",
                   "ERROR", e);
    }
}

void Handler::resetReleased(sdbusplus::message_t& /* msg */)
{
    try
    {
        handlePowerEvent(PowerEvent::resetReleased);
    }
    catch (const sdbusplus::exception::exception& e)
    {
        lg2::error("Failed power state change on a reset button press: {ERROR}",
                   "ERROR", e);
    }
}

void Handler::idReleased(sdbusplus::message_t& /* msg */)
{
    std::string groupPath{ledGroupBasePath};
    groupPath += ID_LED_GROUP;

    auto service = getService(groupPath, ledGroupIface);

    if (service.empty())
    {
        lg2::info("No found {GROUP} during ID button press:", "GROUP",
                  groupPath);
        return;
    }

    try
    {
        auto method = bus.new_method_call(service.c_str(), groupPath.c_str(),
                                          propertyIface, "Get");
        method.append(ledGroupIface, "Asserted");
        auto result = bus.call(method);

        std::variant<bool> state;
        result.read(state);

        state = !std::get<bool>(state);

        lg2::info(
            "Changing ID LED group state on ID LED press, GROUP = {GROUP}, STATE = {STATE}",
            "GROUP", groupPath, "STATE", std::get<bool>(state));

        method = bus.new_method_call(service.c_str(), groupPath.c_str(),
                                     propertyIface, "Set");

        method.append(ledGroupIface, "Asserted", state);
        result = bus.call(method);
    }
    catch (const sdbusplus::exception::exception& e)
    {
        lg2::error("Error toggling ID LED group on ID button press: {ERROR}",
                   "ERROR", e);
    }
}

#if UID_BUTTON_FUNCTION
namespace
{

void eventLog(sdbusplus::bus_t& bus, const std::string& message,
              const std::string& severity)
{
    try
    {
        auto method = bus.new_method_call(
            "xyz.openbmc_project.Logging", "/xyz/openbmc_project/logging",
            "xyz.openbmc_project.Logging.Create", "Create");
        std::map<std::string, std::string> additionalData;
        method.append(message, severity, additionalData);
        bus.call(method);
    }
    catch (const std::exception& e)
    {
        lg2::error("Failed to generate event log: {ERROR}", "ERROR", e.what());
    }
}

void factoryReset(sdbusplus::bus_t& bus)
{
    int val = std::system("fw_setenv openbmconce factory-reset");
    if (val == 0)
    {
        lg2::info("Factory Reset successfully using UID button press");
        eventLog(bus, "Factory Reset successfully using UID button press",
                 "xyz.openbmc_project.Logging.Entry.Level.Informational");
        auto ret = std::system("reboot");
        if (ret != 0)
        {
            lg2::error("Reboot failed after factory reset was armed");
            eventLog(bus, "Reboot failed after factory reset was armed",
                     "xyz.openbmc_project.Logging.Entry.Level.Error");
        }
    }
    else
    {
        lg2::error("Error during Factory Reset");
        eventLog(bus, "Error during Factory Reset",
                 "xyz.openbmc_project.Logging.Entry.Level.Error");
    }
}

// PAM conversation function used to feed the new password to the PAM stack.
int pamFunctionConversation(int numMsg, const struct pam_message** msg,
                            struct pam_response** resp, void* appdataPtr)
{
    if ((appdataPtr == nullptr) || (msg == nullptr) || (resp == nullptr))
    {
        return PAM_CONV_ERR;
    }

    if (numMsg <= 0 || numMsg >= PAM_MAX_NUM_MSG)
    {
        return PAM_CONV_ERR;
    }

    auto msgCount = static_cast<size_t>(numMsg);
    auto messages = std::span(msg, msgCount);

    for (size_t i = 0; i < msgCount; ++i)
    {
        /* Ignore all PAM messages except prompting for hidden input */
        if (messages[i]->msg_style != PAM_PROMPT_ECHO_OFF)
        {
            continue;
        }

        /* Assume PAM is only prompting for the password as hidden input */
        /* Allocate memory only when PAM_PROMPT_ECHO_OFF is encountered */

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        char* appPass = reinterpret_cast<char*>(appdataPtr);
        // Buffer size including the trailing NUL terminator.
        size_t appPassSize = std::strlen(appPass) + 1;

        if (appPassSize > PAM_MAX_RESP_SIZE)
        {
            return PAM_CONV_ERR;
        }
        // Ideally we'd like to avoid using malloc here, but because we're
        // passing off ownership of this to a C application, there aren't a lot
        // of sane ways to avoid it.

        // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
        void* passPtr = malloc(appPassSize);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        char* pass = reinterpret_cast<char*>(passPtr);
        if (pass == nullptr)
        {
            return PAM_BUF_ERR;
        }

        std::strncpy(pass, appPass, appPassSize);

        // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
        void* ptr = calloc(msgCount, sizeof(struct pam_response));
        if (ptr == nullptr)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
            free(pass);
            return PAM_BUF_ERR;
        }

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        *resp = reinterpret_cast<pam_response*>(ptr);

        // Create the span only after *resp points at the freshly allocated
        // array, otherwise it would reference unallocated memory.
        auto responses = std::span(*resp, msgCount);
        responses[i].resp = pass;

        return PAM_SUCCESS;
    }

    return PAM_CONV_ERR;
}

int pamUpdatePassword(const std::string& username, const std::string& password)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    char* passStrNoConst = const_cast<char*>(password.c_str());
    const struct pam_conv localConversation = {pamFunctionConversation,
                                               passStrNoConst};
    pam_handle_t* localAuthHandle = nullptr; // this gets set by pam_start

    int retval = pam_start("passwordreset", username.c_str(),
                           &localConversation, &localAuthHandle);
    if (retval != PAM_SUCCESS)
    {
        return retval;
    }

    retval = pam_chauthtok(localAuthHandle, PAM_SILENT);
    if (retval != PAM_SUCCESS)
    {
        pam_end(localAuthHandle, retval);
        return retval;
    }

    return pam_end(localAuthHandle, PAM_SUCCESS);
}

void passwordReset(sdbusplus::bus_t& bus)
{
    auto ret = pamUpdatePassword(USERNAME, PASSWORD);
    if (ret == PAM_SUCCESS)
    {
#ifdef EXPIRE_PASSWORD
        // Enforce password expiration for the user.
        std::string cmd = "passwd --expire " + std::string(USERNAME);
        auto expireRet = std::system(cmd.c_str());
        if (expireRet != 0)
        {
            lg2::error("Failed to expire password after reset.");
        }
#endif // EXPIRE_PASSWORD
        lg2::info("Password reset successfully using UID button press");
        eventLog(bus, "Password reset successfully using UID button press",
                 "xyz.openbmc_project.Logging.Entry.Level.Informational");
    }
    else
    {
        lg2::error("Failed to reset password: {RET}", "RET", ret);
        eventLog(bus, "Failed to reset password using UID button press",
                 "xyz.openbmc_project.Logging.Entry.Level.Error");
    }
}

} // namespace

void Handler::idPressedLong(sdbusplus::message_t& msg)
{
    try
    {
        uint64_t milliseconds = 0;
        msg.read(milliseconds);

        if (milliseconds >= static_cast<uint64_t>(UID_FACTORY_RESET_TIME_MSEC))
        {
            lg2::info("UID button held {MSEC}ms: performing factory reset",
                      "MSEC", milliseconds);
            factoryReset(bus);
        }
        else
        {
            lg2::info("UID button held {MSEC}ms: performing password reset",
                      "MSEC", milliseconds);
            passwordReset(bus);
        }
    }
    catch (const std::exception& e)
    {
        lg2::error("Exception handling UID button long press: {ERROR}", "ERROR",
                   e.what());
    }
}
#endif // UID_BUTTON_FUNCTION
} // namespace button
} // namespace phosphor
