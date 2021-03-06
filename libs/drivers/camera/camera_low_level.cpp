#include "camera_low_level.h"
#include <array>
#include <gsl/span>
#include "logger/logger.h"
#include "system.h"

using namespace devices::camera;
using namespace drivers::uart;
using namespace std::chrono_literals;

constexpr std::chrono::milliseconds LowLevelCameraDriver::DefaultTimeout;

constexpr std::chrono::milliseconds LowLevelCameraDriver::ResetTimeout;

gsl::span<uint8_t> SkipAck(gsl::span<uint8_t> buffer)
{
    return buffer.last(buffer.length() - CommandFrameSize);
}

LowLevelCameraDriver::LowLevelCameraDriver(error_counter::ErrorCounting& errorCounting, ILineIO& lineIO)
    : _lineIO(lineIO), _error(errorCounting)
{
}

bool LowLevelCameraDriver::SendCommand( //
    gsl::span<uint8_t> commandBuffer,   //
    gsl::span<uint8_t> receiveBuffer,   //
    uint8_t additionalBytes,            //
    std::chrono::milliseconds timeout)
{
    ErrorReporter errorContext(_error);
    uint8_t commandCode = commandBuffer[1];

    LogSendCommand(commandBuffer);

    const auto readSucceeded = _lineIO.ExchangeBuffers(commandBuffer, receiveBuffer, timeout);
    if (!readSucceeded)
    {
#ifdef CAMERA_DEBUG
        LOG(LOG_LEVEL_ERROR, "[cam] LineIO read timeout");
#endif
        errorContext.Counter().Failure();
        return false;
    }

    LogReceivedCommand(receiveBuffer);

    if (receiveBuffer.length() < CommandFrameSize + additionalBytes)
    {
        LOG(LOG_LEVEL_ERROR, "[cam] Too less data received.");
        return false;
    }

    const auto status = receiveBuffer[2] == commandCode;
#ifdef CAMERA_DEBUG
    if (!status)
    {
        LOGF(LOG_LEVEL_ERROR, "[cam] Invalid ack command received. Expected %02X was %02X", commandCode, receiveBuffer[2]);
    }
#endif

    return status;
}

bool LowLevelCameraDriver::SendCommand(gsl::span<uint8_t> command, std::chrono::milliseconds timeout)
{
    std::array<uint8_t, CommandFrameSize> receiveBuffer;
    receiveBuffer.fill(0);

    return SendCommand(command, receiveBuffer, 0, timeout);
}

bool LowLevelCameraDriver::SendAckWithResponse( //
    CameraCmd ackedCommand,                     //
    uint16_t packageId,                         //
    gsl::span<uint8_t> receiveBuffer,           //
    std::chrono::milliseconds timeout)
{
    ErrorReporter errorContext(_error);
    std::array<uint8_t, CommandFrameSize> commandBuffer;

    uint8_t lowerPackageIdByte = packageId & 0xff;
    uint8_t higherPackageIdByte = (uint8_t)(packageId >> 8);

    _commandFactory.BuildAck(commandBuffer, ackedCommand, lowerPackageIdByte, higherPackageIdByte);
    LogSendCommand(commandBuffer);

    const auto readSucceeded = _lineIO.ExchangeBuffers(gsl::span<uint8_t>(commandBuffer), receiveBuffer, timeout);

    if (!readSucceeded)
    {
#ifdef CAMERA_DEBUG
        LOG(LOG_LEVEL_ERROR, "[cam] LineIO read timeout");
#endif
        errorContext.Counter().Failure();
        return false;
    }

    return true;
}

void LowLevelCameraDriver::SendAck(CameraCmd ackedCommand, uint8_t packageIdLow, uint8_t packageIdHigh)
{
    std::array<uint8_t, CommandFrameSize> commandBuffer;

    _commandFactory.BuildAck(commandBuffer, ackedCommand, packageIdLow, packageIdHigh);

    LogSendCommand(commandBuffer);
    _lineIO.PrintBuffer(gsl::span<uint8_t>(commandBuffer));
}

bool LowLevelCameraDriver::SendSync(std::chrono::milliseconds timeout)
{
#ifdef CAMERA_DEBUG
    LOG(LOG_LEVEL_INFO, "[cam] Sending Sync command.");
#endif
    std::array<uint8_t, CommandFrameSize> commandBuffer;
    std::array<uint8_t, 2 * CommandFrameSize> receiveBuffer;
    receiveBuffer.fill(0);

    _commandFactory.BuildSync(commandBuffer);

    if (!SendCommand(commandBuffer, receiveBuffer, CommandFrameSize, timeout))
    {
        return false;
    }

    if (!IsValidSyncResponse(SkipAck(receiveBuffer)))
    {
        LOG(LOG_LEVEL_INFO, "[cam] Expected Sync response.");
        return false;
    }

    return true;
}

bool LowLevelCameraDriver::SendReset()
{
    LOG(LOG_LEVEL_INFO, "[cam] Sending Reset command.");
    std::array<uint8_t, CommandFrameSize> commandBuffer;

    _commandFactory.BuildReset(commandBuffer, CameraResetType::Reboot);
    return SendCommand(commandBuffer, ResetTimeout);
}

bool LowLevelCameraDriver::SendJPEGInitial(CameraJPEGResolution jpegResolution)
{
    LOG(LOG_LEVEL_INFO, "[cam] Sending Initial command.");
    std::array<uint8_t, CommandFrameSize> commandBuffer;

    _commandFactory.BuildInitJPEG(commandBuffer, jpegResolution);
    return SendCommand(commandBuffer);
}

bool LowLevelCameraDriver::SendGetPictureJPEG(CameraPictureType::Enum type, PictureData& pictureData)
{
    LOG(LOG_LEVEL_INFO, "[cam] Sending GetPicture command.");
    std::array<uint8_t, CommandFrameSize> commandBuffer;
    std::array<uint8_t, 2 * CommandFrameSize> receiveBuffer;
    receiveBuffer.fill(0);

    _commandFactory.BuildGetPicture(commandBuffer, type);
    if (!SendCommand(commandBuffer, receiveBuffer, CommandFrameSize))
    {
        return false;
    }

    if (!pictureData.Parse(SkipAck(receiveBuffer)))
    {
        LOG(LOG_LEVEL_ERROR, "[cam] Failed to parse response.");
        return false;
    }

    return (pictureData.type == type && pictureData.dataLength > 0);
}

bool LowLevelCameraDriver::SendSnapshot(CameraSnapshotType type)
{
    LOG(LOG_LEVEL_INFO, "[cam] Sending Snapshot command.");
    std::array<uint8_t, CommandFrameSize> commandBuffer;

    _commandFactory.BuildSnapshot(commandBuffer, type);
    return SendCommand(commandBuffer);
}

bool LowLevelCameraDriver::SendSetPackageSize(uint16_t packageSize)
{
    LOG(LOG_LEVEL_INFO, "[cam] Sending SetPackageSize command.");
    std::array<uint8_t, CommandFrameSize> commandBuffer;

    if (!_commandFactory.BuildSetPackageSize(commandBuffer, packageSize))
    {
        return false;
    }

    return SendCommand(commandBuffer);
}

bool LowLevelCameraDriver::SendSetBaudRate(uint8_t firstDivider, uint8_t secondDivider)
{
    LOG(LOG_LEVEL_INFO, "[cam] Sending SendSetBaudRate command.");
    std::array<uint8_t, CommandFrameSize> commandBuffer;

    _commandFactory.BuildSetBaudRate(commandBuffer, firstDivider, secondDivider);
    return SendCommand(commandBuffer);
}

bool LowLevelCameraDriver::IsValidSyncResponse(gsl::span<const uint8_t> command)
{
    LogReceivedCommand(command);
    return (command[0] == CommandPrefix && (CameraCmd)command[1] == CameraCmd::Sync);
}

void LowLevelCameraDriver::LogSendCommand(gsl::span<uint8_t> cmd)
{
#ifdef CAMERA_DEBUG
    LOGF(LOG_LEVEL_INFO, "[cam] send:%02X %02X %02X %02X %02X %02X", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5]);
#else
    UNREFERENCED_PARAMETER(cmd);
#endif
}

void LowLevelCameraDriver::LogReceivedCommand(gsl::span<const uint8_t> cmd)
{
#ifdef CAMERA_DEBUG
    LOGF(LOG_LEVEL_INFO, "[cam] received:%02X %02X %02X %02X %02X %02X", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5]);
#else
    UNREFERENCED_PARAMETER(cmd);
#endif
}
