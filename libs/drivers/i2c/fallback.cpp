#include <stddef.h>
#include "i2c.h"
#include "logger/logger.h"

I2CResult I2CFallbackBus::Write(const I2CAddress address, gsl::span<const uint8_t> inData)
{
    I2CInterface* buses = this->InnerBuses;

    const I2CResult systemBusResult = buses->Bus->Write(address, inData);

    if (systemBusResult == I2CResult::OK)
    {
        return systemBusResult;
    }

    LOGF(LOG_LEVEL_WARNING, "Fallbacking to payload bus. System bus error %d. Transfer to %X", num(systemBusResult), address);

    const I2CResult payloadBusResult = buses->Payload->Write(address, inData);

    return payloadBusResult;
}

I2CResult I2CFallbackBus::WriteRead(const I2CAddress address, gsl::span<const uint8_t> inData, gsl::span<uint8_t> outData)
{
    I2CInterface* buses = this->InnerBuses;

    const I2CResult systemBusResult = buses->Bus->WriteRead(address, inData, outData);

    if (systemBusResult == I2CResult::OK)
    {
        return systemBusResult;
    }

    LOGF(LOG_LEVEL_WARNING, "Fallbacking to payload bus. System bus error %d. Transfer to %X", num(systemBusResult), address);

    const I2CResult payloadBusResult = buses->Payload->WriteRead(address, inData, outData);

    return payloadBusResult;
}

I2CFallbackBus::I2CFallbackBus(I2CInterface* buses)
{
    this->InnerBuses = buses;
}
