#include "camera.h"
#include <cstring>
#include "base/os.h"
#include "logger/logger.h"
#include "system.h"

using namespace devices::camera;
using namespace std::chrono_literals;

InitializationResult::InitializationResult() : IsSuccess(false), SyncCount(0)
{
}

Camera::Camera(error_counter::ErrorCounting& errorCounting, ILineIO& lineIO) : _cameraDriver{errorCounting, lineIO}
{
}

InitializationResult Camera::Initialize()
{
    InitializationResult result;

    _cameraDriver.SendReset();

    if (!CameraSync(result.SyncCount))
    {
        LOG(LOG_LEVEL_ERROR, "Camera: Sync failed");
        return result;
    }

    LOG(LOG_LEVEL_INFO, "Camera: Sync finished");

    result.IsSuccess = true;

    return result;
}

bool Camera::TakeJPEGPicture(CameraJPEGResolution resolution)
{
    const uint16_t packageSize = 512;

    if (!_cameraDriver.SendJPEGInitial(resolution))
    {
        LOG(LOG_LEVEL_ERROR, "Camera: SendJPEGInitial failed");
        return false;
    }

    if (!_cameraDriver.SendSetPackageSize(packageSize))
    {
        LOG(LOG_LEVEL_ERROR, "Camera: SendSetPackageSize Package failed");
        return false;
    }

    if (!_cameraDriver.SendSnapshot(CameraSnapshotType::Compressed))
    {
        LOG(LOG_LEVEL_ERROR, "Camera: SendSnapshot failed");
        return false;
    }

    LOG(LOG_LEVEL_INFO, "Camera: Picture Ready To Download");

    return true;
}

gsl::span<uint8_t> Camera::CameraReceiveJPEGData(gsl::span<uint8_t> buffer)
{
    PictureData pictureData;
    if (!_cameraDriver.SendGetPictureJPEG(CameraPictureType::Enum::Snapshot, pictureData))
    {
        LOG(LOG_LEVEL_ERROR, "Camera: SendGetPictureJPEG failed");
        return {};
    }

    uint32_t totalDataLength = std::min(pictureData.dataLength, static_cast<uint32_t>(buffer.size()));
    uint8_t packageCnt = totalDataLength / (PackageSize - 6) + (totalDataLength % (PackageSize - 6) != 0 ? 1 : 0);

    if (totalDataLength + (6u * packageCnt) > static_cast<uint32_t>(buffer.size()))
    {
        LOG(LOG_LEVEL_ERROR, "Camera: Buffer to small");
        return gsl::span<uint8_t, 0>();
    }

    uint32_t dataIndex = 0;
    uint32_t bufferIndex = 0;

    uint16_t i = 0;
    for (i = 0; i < packageCnt; i++)
    {
        auto dataToTake = std::min(static_cast<uint32_t>(PackageSize - 6), totalDataLength - dataIndex);

        bool result = false;

        for (auto j = 0; j < 3; j++)
        {
            result = _cameraDriver.SendAckWithResponse(       //
                CameraCmd::None,                              //
                i,                                            //
                buffer.subspan(bufferIndex, dataToTake + 6)); //

            if (result)
            {
                break;
            }

            LOGF(LOG_LEVEL_INFO, "[cam] Retrying package download %d", j);
        }

        if (!result)
        {
            break;
        }

        dataIndex += dataToTake;
        bufferIndex += dataToTake + 6;
    }

    _cameraDriver.SendAck(CameraCmd::None, 0xF0, 0xF0);

    return buffer.first(bufferIndex);
}

bool Camera::CameraSync(uint8_t& syncCount)
{
    syncCount = 0;
    uint8_t i = 0;

    while (i < MaxSyncRetries)
    {
        syncCount++;
        if (!_cameraDriver.SendSync(std::chrono::milliseconds(10 + i)))
        {
            i++;
            continue;
        }

        _cameraDriver.SendAck(CameraCmd::Sync);
        return true;
    }

    return false;
}

devices::camera::LowLevelCameraDriver& Camera::Driver()
{
    return _cameraDriver;
}
