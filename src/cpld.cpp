
#include "button_config.hpp"
#include "config.hpp"

#include <error.h>
#include <fcntl.h>
#include <unistd.h>

#include <phosphor-logging/lg2.hpp>

const std::string cpldDev = "/sys/bus/i2c/devices/";

std::string getCpldDevPath(const CpldInfo& info)
{
    std::stringstream devPath;
    devPath << cpldDev << info.i2cBus << "-" << std::hex << std::setw(4)
            << std::setfill('0') << info.i2cAddress << "/" << info.registerName;
    return devPath.str();
}

int configCpld(ButtonConfig& buttonIFConfig)
{
    std::string devPath = getCpldDevPath(buttonIFConfig.cpld);

    auto fd = ::open(devPath.c_str(), O_RDONLY | O_NONBLOCK);

    if (fd < 0)
    {
        lg2::error("Open {PATH} error: {ERROR}", "PATH", devPath, "ERROR",
                   errno);
        return -1;
    }

    buttonIFConfig.cpld.cpldMappedFd = fd;
    buttonIFConfig.fds.push_back(fd);
    return 0;
}
