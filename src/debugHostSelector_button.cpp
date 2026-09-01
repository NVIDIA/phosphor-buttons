#include "debugHostSelector_button.hpp"
// add the button iface class to registry
static ButtonIFRegister<DebugHostSelector> buttonRegister;
using namespace phosphor::logging;

void DebugHostSelector::simPress()
{
    pressed();
}

void DebugHostSelector::simRelease()
{
    released();
}

void DebugHostSelector::simLongPress()
{
    pressedLong();
}

/**
 * @brief This method is called from sd-event provided callback function
 * callbackHandler if platform specific event handling is needed then a
 * derived class instance with its specific event handling logic along with
 * init() function can be created to override the default event handling
 */

void DebugHostSelector::handleEvent(sd_event_source* /* es */, int fd,
                                    uint32_t /* revents*/)
{
    int n = -1;
    char buf = '0';

    n = ::lseek(fd, 0, SEEK_SET);

    if (n < 0)
    {
        lg2::error("GPIO fd lseek error!  : {FORM_FACTOR_TYPE}",
                   "FORM_FACTOR_TYPE", getFormFactorType());
        return;
    }

    n = ::read(fd, &buf, sizeof(buf));
    if (n < 0)
    {
        lg2::error("GPIO fd read error!  : {FORM_FACTOR_TYPE}",
                   "FORM_FACTOR_TYPE", getFormFactorType());
        throw sdbusplus::xyz::openbmc_project::Chassis::Common::Error::
            IOError();
    }

    if (buf == '0')
    {
        lg2::info("Button pressed : {FORM_FACTOR_TYPE}", "FORM_FACTOR_TYPE",
                  getFormFactorType());
        // emit pressed signal
        pressed();
    }
    else
    {
        lg2::info("Button released{FORM_FACTOR_TYPE}", "FORM_FACTOR_TYPE",
                  getFormFactorType());
        // emit released signal
        released();
    }
}
