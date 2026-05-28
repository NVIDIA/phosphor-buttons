#include "button_handler.hpp"

#include <phosphor-logging/lg2.hpp>

#include <cstdlib>
#include <exception>

int main(void)
try
{
    auto bus = sdbusplus::bus::new_default();

    phosphor::button::Handler handler{bus};

    while (true)
    {
        bus.process_discard();
        bus.wait();
    }
    return 0;
}
catch (const std::exception& e)
{
    lg2::error("button-handler terminated by exception: {ERR}", "ERR",
               e.what());
    return EXIT_FAILURE;
}
