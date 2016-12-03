#include "miniport.h"
#include <string.h>
#include <cstdint>
#include "base/reader.h"
#include "logger/logger.h"

/**
 * @brief Enumerator of all supported antenna controller commands.
 * @ingroup AntennaMiniport
 */
enum Command
{
    DEPLOY_ANTENNA = 0xa0,
    START_AUTOMATIC_DEPLOYMENT = 0xa5,
    CANCEL_ANTENNA_DEPLOYMENT = 0xa9,
    RESET = 0xaa,
    DISARM = 0xac,
    ARM = 0xad,
    DEPLOY_ANTENNA_OVERRIDE = 0xb9,
    QUERY_ANTENNA_ACTIVATION_COUNT = 0xaf,
    QUERY_ANTENNA_ACTIVATION_TIME = 0xb3,
    QUERY_TEMPERATURE = 0xc0,
    QUERY_DEPLOYMENT_STATUS = 0xc3
};

/**
 * @brief Maps boolean operation status to OSResult::.
 * @param[in] status Operation status.
 * @return Mapped operation status.
 * @ingroup AntennaMiniport
 */
static inline OSResult MapStatus(I2CResult status)
{
    switch (status)
    {
        case I2CResultOK:
            return OSResult::Success;
        case I2CResultNack:
        case I2CResultBusErr:
        case I2CResultArbLost:
        case I2CResultUsageFault:
        case I2CResultSwFault:
        case I2CResultClockLatched:
        case I2CResultFailure:
            return OSResult::IOError;
        case I2CResultTimeout:
            return OSResult::Timeout;
        default:
        case I2CResultClockAlreadyLatched:
            return OSResult::ProtocolError;
    }
}

/**
 * @brief Sends requested command via the passed miniport driver.
 * @param[in] bus Bus that should be used to communicate with hardware.
 * @param[in] channel Current hardware channel.
 * @param[in] command Command to be sent.
 * @return Operation status. True on success, false otherwise.
 * @ingroup AntennaMiniport
 */
static OSResult SendCommand(I2CBus* bus, AntennaChannel channel, Command command)
{
    uint8_t data = (uint8_t)command;
    const I2CResult result = bus->Write(channel, &data, 1);
    const bool status = (result == I2CResultOK);
    if (!status)
    {
        LOGF(LOG_LEVEL_ERROR, "[ant] Unable to send command %d to %d, Reason: %d", command, channel, result);
    }

    return MapStatus(result);
}

/**
 * @brief Sends requested command via the passed miniport driver and reads hardware response
 * @param[in] bus Bus that should be used to communicate with hardware.
 * @param[in] channel Current hardware channel.
 * @param[in] command Command to be sent.
 * @param[out] outBuffer Buffer that should be filled with response from hardware.
 * @param[in] outBufferSize Size in bytes of the outBuffer buffer.
 * @return Operation status. True on success, false otherwise.
 * @ingroup AntennaMiniport
 */
static OSResult SendCommandWithResponse(I2CBus* bus,
    AntennaChannel channel,
    Command command,
    uint8_t* outBuffer,
    uint8_t outBufferSize //
    )
{
    const I2CResult result = bus->WriteRead(channel, reinterpret_cast<std::uint8_t*>(&command), sizeof(command), outBuffer, outBufferSize);
    const bool status = (result == I2CResultOK);
    if (!status)
    {
        LOGF(LOG_LEVEL_ERROR, "[ant] Unable to send command %d to %d, Reason: %d", command, channel, result);
    }

    return MapStatus(result);
}

static OSResult Reset(AntennaMiniportDriver* miniport,
    I2CBus* communicationBus,
    AntennaChannel channel //
    )
{
    UNREFERENCED_PARAMETER(miniport);
    return SendCommand(communicationBus, channel, RESET);
}

static OSResult ArmDeploymentSystem(AntennaMiniportDriver* miniport,
    I2CBus* communicationBus,
    AntennaChannel channel //
    )
{
    UNREFERENCED_PARAMETER(miniport);
    return SendCommand(communicationBus, channel, ARM);
}

static OSResult DisarmDeploymentSystem(AntennaMiniportDriver* miniport,
    I2CBus* communicationBus,
    AntennaChannel channel //
    )
{
    UNREFERENCED_PARAMETER(miniport);
    return SendCommand(communicationBus, channel, DISARM);
}

static inline uint8_t GetOverrideFlag(bool override)
{
    return override ? (DEPLOY_ANTENNA_OVERRIDE - DEPLOY_ANTENNA) : 0;
}

static OSResult DeployAntenna(struct AntennaMiniportDriver* miniport,
    I2CBus* communicationBus,
    AntennaChannel channel,
    AntennaId antennaId,
    TimeSpan timeout,
    bool override //
    )
{
    UNREFERENCED_PARAMETER(miniport);
    uint8_t buffer[2];
    buffer[0] = (uint8_t)(DEPLOY_ANTENNA + antennaId + GetOverrideFlag(override));
    buffer[1] = (uint8_t)TimeSpanToSeconds(timeout);
    return MapStatus(communicationBus->Write(channel,
        buffer,
        sizeof(buffer) //
        ));
}

static OSResult InitializeAutomaticDeployment(AntennaMiniportDriver* miniport,
    I2CBus* communicationBus,
    AntennaChannel channel,
    TimeSpan timeout //
    )
{
    UNREFERENCED_PARAMETER(miniport);
    uint8_t buffer[2];
    buffer[0] = (uint8_t)(START_AUTOMATIC_DEPLOYMENT);
    buffer[1] = (uint8_t)TimeSpanToSeconds(timeout);
    return MapStatus(communicationBus->Write(channel,
        buffer,
        sizeof(buffer) //
        ));
}

static OSResult CancelAntennaDeployment(AntennaMiniportDriver* miniport,
    I2CBus* communicationBus,
    AntennaChannel channel //
    )
{
    UNREFERENCED_PARAMETER(miniport);
    return SendCommand(communicationBus, channel, CANCEL_ANTENNA_DEPLOYMENT);
}

static OSResult GetDeploymentStatus(AntennaMiniportDriver* miniport,
    I2CBus* communicationBus,
    AntennaChannel channel,
    AntennaDeploymentStatus* telemetry //
    )
{
    UNREFERENCED_PARAMETER(miniport);
    uint8_t output[2];
    memset(telemetry, 0, sizeof(*telemetry));
    const OSResult result = SendCommandWithResponse(communicationBus,
        channel,
        QUERY_DEPLOYMENT_STATUS,
        output,
        sizeof(output) //
        );

    if (OS_RESULT_FAILED(result))
    {
        return result;
    }

    Reader reader;
    ReaderInitialize(&reader, output, sizeof(output));
    const uint16_t value = ReaderReadWordLE(&reader);
    if ((value & 0x1000) != 0)
    {
        LOGF(LOG_LEVEL_WARNING,
            "[ant] Antenna %d deployment status out of range: %d.",
            channel,
            value //
            );

        return OSResult::OutOfRange;
    }

    telemetry->DeploymentStatus[0] = IS_BIT_CLEAR(value, 15); // (value & 0x8000) == 0;
    telemetry->DeploymentStatus[1] = IS_BIT_CLEAR(value, 11); // (value & 0x0800) == 0;
    telemetry->DeploymentStatus[2] = IS_BIT_CLEAR(value, 7);  // (value & 0x0080) == 0;
    telemetry->DeploymentStatus[3] = IS_BIT_CLEAR(value, 3);  // (value & 0x0008) == 0;

    telemetry->IsDeploymentActive[0] = IS_BIT_SET(value, 13);     //(value & 0x2000) != 0;
    telemetry->IsDeploymentActive[1] = IS_BIT_SET(value, 9);      //(value & 0x0200) != 0;
    telemetry->IsDeploymentActive[2] = IS_BIT_SET(value, 5);      //(value & 0x0020) != 0;
    telemetry->IsDeploymentActive[3] = IS_BIT_SET(value, 1);      //(value & 0x0002) != 0;
    telemetry->IgnoringDeploymentSwitches = IS_BIT_SET(value, 8); //(value & 0x0100) != 0;
    telemetry->DeploymentSystemArmed = IS_BIT_SET(value, 0);      //(value & 0x0001) != 0;
    return OSResult::Success;
}

static OSResult GetAntennaActivationCount(AntennaMiniportDriver* miniport,
    I2CBus* communicationBus,
    AntennaChannel channel,
    AntennaId antennaId,
    uint16_t* count //
    )
{
    UNREFERENCED_PARAMETER(miniport);
    uint8_t output;
    const OSResult result = SendCommandWithResponse(communicationBus,
        channel,
        (Command)(QUERY_ANTENNA_ACTIVATION_COUNT + antennaId),
        &output,
        sizeof(output) //
        );

    if (OS_RESULT_FAILED(result))
    {
        *count = 0;
        return result;
    }

    *count = output;
    return OSResult::Success;
}

static OSResult GetAntennaActivationTime(AntennaMiniportDriver* miniport,
    I2CBus* communicationBus,
    AntennaChannel channel,
    AntennaId antennaId,
    TimeSpan* span //
    )
{
    UNREFERENCED_PARAMETER(miniport);
    uint8_t output[2];

    const OSResult result = SendCommandWithResponse(communicationBus,
        channel,
        (Command)(QUERY_ANTENNA_ACTIVATION_TIME + antennaId),
        output,
        sizeof(output) //
        );

    if (OS_RESULT_FAILED(result))
    {
        *span = TimeSpanFromMilliseconds(0);
        return OSResult::IOError;
    }

    Reader reader;
    ReaderInitialize(&reader, output, sizeof(output));
    const uint16_t value = ReaderReadWordBE(&reader);
    *span = TimeSpanFromMilliseconds(value * 50);
    return OSResult::Success;
}

static OSResult GetTemperature(AntennaMiniportDriver* miniport,
    I2CBus* communicationBus,
    AntennaChannel channel,
    uint16_t* temperature //
    )
{
    UNREFERENCED_PARAMETER(miniport);
    uint8_t output[2];
    *temperature = 0;
    const OSResult result = SendCommandWithResponse(communicationBus,
        channel,
        QUERY_TEMPERATURE,
        output,
        sizeof(output) //
        );

    if (OS_RESULT_FAILED(result))
    {
        return OSResult::IOError;
    }

    Reader reader;
    ReaderInitialize(&reader, output, sizeof(output));
    const uint16_t value = ReaderReadWordBE(&reader);
    if ((value & 0xfc00) != 0)
    {
        LOGF(LOG_LEVEL_WARNING,
            "[ant] Antenna %d temperature is out of range: %d.",
            channel,
            value //
            );

        return OSResult::OutOfRange;
    }

    *temperature = value & 0x3ff;
    return OSResult::Success;
}

void AntennaMiniportInitialize(AntennaMiniportDriver* driver)
{
    memset(driver, 0, sizeof(*driver));
    driver->Reset = Reset;
    driver->ArmDeploymentSystem = ArmDeploymentSystem;
    driver->DisarmDeploymentSystem = DisarmDeploymentSystem;
    driver->DeployAntenna = DeployAntenna;
    driver->InitializeAutomaticDeployment = InitializeAutomaticDeployment;
    driver->CancelAntennaDeployment = CancelAntennaDeployment;
    driver->GetDeploymentStatus = GetDeploymentStatus;
    driver->GetAntennaActivationCount = GetAntennaActivationCount;
    driver->GetAntennaActivationTime = GetAntennaActivationTime;
    driver->GetTemperature = GetTemperature;
}
