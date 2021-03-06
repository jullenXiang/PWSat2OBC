#include "params.hpp"

namespace boot
{
    __attribute__((section(".boot_param.0"))) decltype(MagicNumber) MagicNumber;
    __attribute__((section(".boot_param.1"))) decltype(BootReason) BootReason;
    __attribute__((section(".boot_param.2"))) decltype(Index) Index;
    __attribute__((section(".boot_param.3"))) decltype(RequestedRunlevel) RequestedRunlevel;
    __attribute__((section(".boot_param.4"))) decltype(ClearStateOnStartup) ClearStateOnStartup;

    bool IsBootInformationAvailable()
    {
        return MagicNumber == BootloaderMagicNumber;
    }
}
