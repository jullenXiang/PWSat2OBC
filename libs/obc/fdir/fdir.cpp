#include "fdir.hpp"
#include <type_traits>
#include "logger/logger.h"
#include "power/power.h"

namespace obc
{
    static_assert(
        std::is_same<std::uint8_t, error_counter::CounterValue>::value, "Mismatch between counter value type and configuration value type");

    static constexpr std::uint32_t Config(
        error_counter::CounterValue limit, error_counter::CounterValue increment, error_counter::CounterValue decrement)
    {
        return ((limit & 0xFF) << 0)    //
            | ((increment & 0xFF) << 8) //
            | ((decrement & 0xFF) << 16);
    }

    static constexpr error_counter::CounterValue GetLimit(std::uint32_t config)
    {
        return (config & 0x0000FF) >> 0;
    }

    static constexpr error_counter::CounterValue GetIncrement(std::uint32_t config)
    {
        return (config & 0x00FF00) >> 8;
    }

    static constexpr error_counter::CounterValue GetDecrement(std::uint32_t config)
    {
        return (config & 0xFF0000) >> 16;
    }

    FDIR::FDIR(services::power::IPowerControl& powerControl, std::uint16_t maskedDevices)
        : _errorCounting(*this), _powerControl(powerControl), _maskedDevices(maskedDevices)
    {
        std::fill(this->_configuration.begin(), this->_configuration.end(), Config(128, 5, 2));
    }

    void FDIR::Initalize()
    {
        this->_errorCounting.Handler(*this);
    }

    void FDIR::LoadConfig(std::array<std::uint32_t, error_counter::ErrorCounting::MaxDevices>& config)
    {
        for (std::size_t i = 0; i < error_counter::ErrorCounting::MaxDevices; i++)
        {
            this->_configuration[i] = config[i];
        }
    }

    error_counter::ErrorCounting& FDIR::ErrorCounting()
    {
        return this->_errorCounting;
    }

    void FDIR::LimitReached(error_counter::Device device, error_counter::CounterValue errorsCount)
    {
        if (has_flag(this->_maskedDevices, 1 << device))
        {
            return;
        }

        LOGF(LOG_LEVEL_FATAL, "Device %d reach error limit of %d", device, errorsCount);
        this->_powerControl.PowerCycle();
    }

    error_counter::CounterValue FDIR::Limit(error_counter::Device device)
    {
        return GetLimit(this->_configuration[device]);
    }

    error_counter::CounterValue FDIR::Increment(error_counter::Device device)
    {
        return GetIncrement(this->_configuration[device]);
    }

    error_counter::CounterValue FDIR::Decrement(error_counter::Device device)
    {
        return GetDecrement(this->_configuration[device]);
    }

    void FDIR::Set(error_counter::Device device,
        error_counter::CounterValue limit,
        error_counter::CounterValue increment,
        error_counter::CounterValue decrement)
    {
        this->_configuration[device] = Config(limit, increment, decrement);
    }

    std::array<std::uint32_t, error_counter::ErrorCounting::MaxDevices> FDIR::GetConfig()
    {
        std::array<std::uint32_t, error_counter::ErrorCounting::MaxDevices> buf;
        for (std::size_t i = 0; i < error_counter::ErrorCounting::MaxDevices; i++)
        {
            buf[i] = this->_configuration[i];
        }
        return buf;
    }
}
