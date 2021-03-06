#include "telemetry/ImtqTelemetry.hpp"
#include <algorithm>
#include "base/BitWriter.hpp"

namespace telemetry
{
    using namespace std::chrono_literals;

    ImtqHousekeeping::ImtqHousekeeping() : digitalVoltage(0), analogVoltage(0), digitalCurrent(0), analogCurrent(0), mcuTemperature(0)
    {
    }

    ImtqHousekeeping::ImtqHousekeeping(devices::imtq::VoltageInMiliVolt digitalVoltage_,
        devices::imtq::VoltageInMiliVolt analogVoltage_,
        devices::imtq::Current digitalCurrent_,
        devices::imtq::Current analogCurrent_,
        devices::imtq::TemperatureMeasurement mcuTemperature_)
        : digitalVoltage(digitalVoltage_), //
          analogVoltage(analogVoltage_),   //
          digitalCurrent(digitalCurrent_), //
          analogCurrent(analogCurrent_),   //
          mcuTemperature(mcuTemperature_)
    {
    }

    void ImtqHousekeeping::Write(BitWriter& writer) const
    {
        writer.Write(this->digitalVoltage);
        writer.Write(this->analogVoltage);
        writer.Write(this->digitalCurrent);
        writer.Write(this->analogCurrent);
        writer.Write(this->mcuTemperature);
    }

    ImtqState::ImtqState() : mode(devices::imtq::Mode::Idle), errorCode(0), configurationUpdated(false), uptime(0s)
    {
    }

    ImtqState::ImtqState(devices::imtq::Mode mode_,
        std::uint8_t errorCode_,
        bool configurationUpdated_,
        std::chrono::seconds uptime_)
        : mode(mode_),                                 //
          errorCode(errorCode_),                       //
          configurationUpdated(configurationUpdated_), //
          uptime(uptime_)
    {
    }

    void ImtqState::Write(BitWriter& writer) const
    {
        writer.WriteWord(num(this->mode), 2);
        writer.Write(this->errorCode);
        writer.Write(this->configurationUpdated);
        writer.Write(gsl::narrow_cast<std::uint32_t>(this->uptime.count()));
    }
}
