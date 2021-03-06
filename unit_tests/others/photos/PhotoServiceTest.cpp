#include <algorithm>
#include <gsl/span>
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "OsMock.hpp"
#include "base/os.h"
#include "mock/FsMock.hpp"
#include "mock/power.hpp"
#include "photo/photo_service.hpp"
#include "power/power.h"

using testing::Each;
using testing::ElementsAre;
using testing::Eq;
using testing::InSequence;
using testing::Invoke;
using testing::Ne;
using testing::Return;
using testing::StrEq;
using testing::_;
using namespace services::photo;
using namespace std::chrono_literals;

struct CameraMock : ICamera
{
    MOCK_METHOD0(Sync, SyncResult());
    MOCK_METHOD1(TakePhoto, TakePhotoResult(PhotoResolution));
    MOCK_METHOD1(DownloadPhoto, DownloadPhotoResult(gsl::span<std::uint8_t> buffer));
};

struct CameraSelectorMock : ICameraSelector
{
    MOCK_METHOD1(Select, void(Camera camera));
};

struct CameraPowerControlMock : PowerControlMock
{
    CameraPowerControlMock();

    MOCK_METHOD2(ControlCamera, bool(Camera, bool));
};

CameraPowerControlMock::CameraPowerControlMock()
{
    ON_CALL(*this, CameraNadir(_)).WillByDefault(Invoke([this](bool enabled) { return this->ControlCamera(Camera::Nadir, enabled); }));
    ON_CALL(*this, CameraWing(_)).WillByDefault(Invoke([this](bool enabled) { return this->ControlCamera(Camera::Wing, enabled); }));
}

namespace
{
    class PhotoServiceTest : public testing::TestWithParam<Camera>
    {
      protected:
        PhotoServiceTest();

        Camera Cam() const;

        testing::NiceMock<OSMock> _os;
        OSReset _osReset{InstallProxy(&_os)};

        testing::NiceMock<CameraPowerControlMock> _power;
        CameraMock _camera;
        testing::NiceMock<CameraSelectorMock> _selector;
        testing::NiceMock<FsMock> _fs;

        PhotoService _service{_power, _camera, _selector, _fs};
    };

    PhotoServiceTest::PhotoServiceTest()
    {
    }

    Camera PhotoServiceTest::Cam() const
    {
        return GetParam();
    }

    TEST_F(PhotoServiceTest, AllBuffersAreEmptyOnInitialize)
    {
        for (auto i = 0; i < PhotoService::BuffersCount; i++)
        {
            auto buffer = _service.GetBufferInfo(i);
            ASSERT_THAT(buffer.Status(), Eq(BufferStatus::Empty));
            ASSERT_THAT(buffer.Size(), Eq(0U));
        }
    }

    TEST_F(PhotoServiceTest, AllBuffersAreEmptyBeyondLimit)
    {
        for (int i = PhotoService::BuffersCount; i <= 0xff; i++)
        {
            auto buffer = _service.GetBufferInfo(i);
            ASSERT_THAT(buffer.Status(), Eq(BufferStatus::Empty));
            ASSERT_THAT(buffer.Size(), Eq(0U));
        }
    }

    TEST_P(PhotoServiceTest, ShouldDisableCamera)
    {
        EXPECT_CALL(_power, ControlCamera(Cam(), false)).WillOnce(Return(true));

        auto r = _service.Invoke(DisableCamera{Cam()});

        ASSERT_THAT(r, Eq(OSResult::Success));
    }

    TEST_P(PhotoServiceTest, ShouldEnableCameraAndSync)
    {
        EXPECT_CALL(_power, ControlCamera(Cam(), true)).WillOnce(Return(true));
        EXPECT_CALL(_selector, Select(Cam()));
        EXPECT_CALL(_camera, Sync()).WillOnce(Return(SyncResult(true, 10)));

        auto r = _service.Invoke(EnableCamera{Cam()});

        ASSERT_THAT(r, Eq(OSResult::Success));
    }

    TEST_P(PhotoServiceTest, ShouldReturnErrorWhenUnableToPowerOnCamera)
    {
        EXPECT_CALL(_power, ControlCamera(Cam(), true)).WillOnce(Return(false));
        EXPECT_CALL(_camera, Sync()).Times(0);

        auto r = _service.Invoke(EnableCamera{Cam()});

        ASSERT_THAT(r, Eq(OSResult::PowerFailure));
    }

    TEST_P(PhotoServiceTest, ShouldReturnErrorWhenUnableToSyncCamera)
    {
        EXPECT_CALL(_power, ControlCamera(Cam(), true)).WillOnce(Return(true));
        EXPECT_CALL(_selector, Select(Cam()));
        EXPECT_CALL(_camera, Sync()).WillOnce(Return(SyncResult(false, 60)));

        auto r = _service.Invoke(EnableCamera{Cam()});

        ASSERT_THAT(r, Eq(OSResult::DeviceNotFound));
    }

    TEST_P(PhotoServiceTest, ShouldRequestTakingAPhoto)
    {
        EXPECT_CALL(_selector, Select(Cam()));
        EXPECT_CALL(_camera, TakePhoto(PhotoResolution::p480)).WillOnce(Return(TakePhotoResult::Success));

        auto r = _service.Invoke(TakePhoto{Cam(), PhotoResolution::p480});

        ASSERT_THAT(r, Eq(OSResult::Success));
    }

    TEST_P(PhotoServiceTest, ShouldRestartCameraIfTakePhotoFailed)
    {
        {
            InSequence s;

            EXPECT_CALL(_camera, TakePhoto(PhotoResolution::p480)).WillOnce(Return(TakePhotoResult::NotSynced));
            EXPECT_CALL(_power, ControlCamera(Cam(), false)).WillOnce(Return(true));
            EXPECT_CALL(_power, ControlCamera(Cam(), true)).WillOnce(Return(true));
            EXPECT_CALL(_camera, Sync()).WillOnce(Return(SyncResult(true, 1)));

            EXPECT_CALL(_camera, TakePhoto(PhotoResolution::p480)).WillOnce(Return(TakePhotoResult::NotSynced));
            EXPECT_CALL(_power, ControlCamera(Cam(), false)).WillOnce(Return(true));
            EXPECT_CALL(_power, ControlCamera(Cam(), true)).WillOnce(Return(true));
            EXPECT_CALL(_camera, Sync()).WillOnce(Return(SyncResult(true, 1)));

            EXPECT_CALL(_camera, TakePhoto(PhotoResolution::p480)).WillOnce(Return(TakePhotoResult::Success));
        }

        auto r = _service.Invoke(TakePhoto{Cam(), PhotoResolution::p480});

        ASSERT_THAT(r, Eq(OSResult::Success));
    }

    TEST_P(PhotoServiceTest, ShouldReturnErrorOfTakingPhotoFailsOnAllRetries)
    {
        ON_CALL(_power, ControlCamera(Cam(), _)).WillByDefault(Return(true));
        ON_CALL(_camera, Sync()).WillByDefault(Return(SyncResult(true, 1)));
        ON_CALL(_camera, TakePhoto(PhotoResolution::p480)).WillByDefault(Return(TakePhotoResult::NotSynced));

        auto r = _service.Invoke(TakePhoto{Cam(), PhotoResolution::p480});

        ASSERT_THAT(r, Eq(OSResult::DeviceNotFound));
    }

    TEST_P(PhotoServiceTest, ShouldDownloadPhotoToBuffer)
    {
        EXPECT_CALL(_selector, Select(Cam()));
        EXPECT_CALL(_camera, DownloadPhoto(_)).WillOnce(Invoke([](auto buffer) {
            std::fill(buffer.begin(), buffer.begin() + 1_KB, 0xAB);

            return DownloadPhotoResult(buffer.subspan(0, 1_KB));
        }));

        auto r = _service.Invoke(DownloadPhoto{Cam(), 1});

        ASSERT_THAT(r, Eq(OSResult::Success));

        auto buf = _service.GetBufferInfo(1);

        ASSERT_THAT(buf.Status(), Eq(BufferStatus::Occupied));
        ASSERT_THAT(buf.Size(), Eq(1_KB));
    }

    TEST_P(PhotoServiceTest, ShouldNotDownloadPhotoWithInvalidBufferId)
    {
        EXPECT_CALL(_selector, Select(Cam())).Times(0);
        EXPECT_CALL(_camera, DownloadPhoto(_)).Times(0);
        auto r = _service.Invoke(DownloadPhoto{Cam(), PhotoService::BuffersCount});

        ASSERT_THAT(r, Ne(OSResult::Success));
    }

    TEST_P(PhotoServiceTest, ShouldReturnFailIfDownloadFails)
    {
        EXPECT_CALL(_selector, Select(Cam()));
        EXPECT_CALL(_camera, DownloadPhoto(_)).Times(3).WillRepeatedly(Return(DownloadPhotoResult(OSResult::DeviceNotFound)));

        auto r = _service.Invoke(DownloadPhoto{Cam(), 1});

        ASSERT_THAT(r, Eq(OSResult::DeviceNotFound));

        auto buf = _service.GetBufferInfo(1);

        ASSERT_THAT(buf.Status(), Eq(BufferStatus::Failed));
    }

    TEST_F(PhotoServiceTest, ShouldFillAdjacentPartsOfBuffer)
    {
        EXPECT_CALL(_camera, DownloadPhoto(_))
            .WillOnce(Invoke([](auto buffer) {
                std::fill(buffer.begin(), buffer.begin() + 1_KB, 0xAB);

                return DownloadPhotoResult(buffer.subspan(0, 1_KB));
            }))
            .WillOnce(Invoke([](auto buffer) {
                std::fill(buffer.begin(), buffer.begin() + 2_KB, 0xCD);

                return DownloadPhotoResult(buffer.subspan(0, 1_KB));
            }));

        _service.Invoke(DownloadPhoto{Camera::Nadir, 1});
        _service.Invoke(DownloadPhoto{Camera::Nadir, 4});

        auto b1 = _service.GetBufferInfo(1);
        auto b4 = _service.GetBufferInfo(4);

        ASSERT_THAT(b1.Buffer(), Each(Eq(0xAB)));
        ASSERT_THAT(b4.Buffer(), Each(Eq(0xCD)));
    }

    TEST_F(PhotoServiceTest, ShouldFreeAllBuffersAfterResetCommand)
    {
        EXPECT_CALL(_camera, DownloadPhoto(_))
            .WillOnce(Invoke([](auto buffer) {
                std::fill(buffer.begin(), buffer.begin() + 1_KB, 0xAB);

                return DownloadPhotoResult(buffer.subspan(0, 1_KB));
            }))
            .WillOnce(Invoke([](auto buffer) {
                std::fill(buffer.begin(), buffer.begin() + 2_KB, 0xCD);

                return DownloadPhotoResult(buffer.subspan(0, 1_KB));
            }));

        _service.Invoke(DownloadPhoto{Camera::Nadir, 1});
        _service.Invoke(DownloadPhoto{Camera::Nadir, 4});
        _service.Invoke(Reset());

        for (auto i = 0; i < PhotoService::BuffersCount; i++)
        {
            auto buffer = _service.GetBufferInfo(i);
            ASSERT_THAT(buffer.Status(), Eq(BufferStatus::Empty));
            ASSERT_THAT(buffer.Size(), Eq(0U));
        }
    }

    TEST_F(PhotoServiceTest, ShouldSavePhotoToFile)
    {
        EXPECT_CALL(_camera, DownloadPhoto(_)).WillOnce(Invoke([](auto buffer) {
            std::fill(buffer.begin(), buffer.begin() + 1_KB, 0xAB);

            return DownloadPhotoResult(buffer.subspan(0, 1_KB));
        }));

        std::array<std::uint8_t, 1_KB> photoBuffer;

        _fs.AddFile("/photo", photoBuffer);

        _service.Invoke(DownloadPhoto{Camera::Nadir, 1});
        _service.Invoke(SavePhoto{1, "/photo"});

        ASSERT_THAT(photoBuffer, Each(Eq(0xAB)));
    }

    TEST_F(PhotoServiceTest, ShouldNotSavePhotoToFileWithInvalidBufferId)
    {
        std::array<std::uint8_t, 1_KB> photoBuffer;

        _fs.AddFile("/photo", photoBuffer);
        const auto status = _service.Invoke(SavePhoto{PhotoService::BuffersCount, "/photo"});
        ASSERT_THAT(status, Ne(OSResult::Success));
    }

    TEST_F(PhotoServiceTest, ShouldSaveMarkerTextToFileIfBufferIsEmpty)
    {
        std::array<std::uint8_t, 1_KB> photoBuffer;

        _fs.AddFile("/photo", photoBuffer);

        _service.Invoke(SavePhoto{1, "/photo"});

        auto s = reinterpret_cast<char*>(photoBuffer.data());

        ASSERT_THAT(s, StrEq("Empty"));
    }

    TEST_F(PhotoServiceTest, ShouldSaveMarkerTextToFileIfBufferIsFailed)
    {
        EXPECT_CALL(_camera, DownloadPhoto(_)).Times(3).WillRepeatedly(Return(DownloadPhotoResult(OSResult::AccessDenied)));
        std::array<std::uint8_t, 1_KB> photoBuffer;

        _fs.AddFile("/photo", photoBuffer);

        _service.Invoke(DownloadPhoto{Camera::Nadir, 1});
        _service.Invoke(SavePhoto{1, "/photo"});

        auto s = reinterpret_cast<char*>(photoBuffer.data());

        ASSERT_THAT(s, StrEq("Failed"));
    }

    TEST_F(PhotoServiceTest, ShouldSleep)
    {
        EXPECT_CALL(_os, EventGroupWaitForBits(_, 1 << 1, false, true, 10000ms));

        auto r = _service.Invoke(Sleep{10000ms});

        ASSERT_THAT(r, Eq(OSResult::Success));
    }

    TEST_F(PhotoServiceTest, PurgePedningCommands)
    {
        testing::InSequence sequence;
        EXPECT_CALL(_os, QueueReset(_));
        EXPECT_CALL(_os, QueueSend(_, _, _)).WillOnce(Invoke([](OSQueueHandle /*handle*/, const void* elementPtr, auto /*timeout*/) {
            auto ptr = static_cast<const PossibleCommand*>(elementPtr);
            EXPECT_THAT(ptr->Selected, Eq(Command::Break));
            return true;
        }));

        EXPECT_CALL(_os, EventGroupSetBits(_, 1 << 1));
        EXPECT_CALL(_os, EventGroupClearBits(_, 1 << 0));

        _service.PurgePendingCommands();
    }

    TEST_F(PhotoServiceTest, TestIsEmptyIndexOverflow)
    {
        ASSERT_THAT(_service.IsEmpty(PhotoService::BuffersCount), Eq(false));
        ASSERT_THAT(_service.IsEmpty(PhotoService::BuffersCount + 1), Eq(false));
        ASSERT_THAT(_service.IsEmpty(PhotoService::BuffersCount + 2), Eq(false));
        ASSERT_THAT(_service.IsEmpty(PhotoService::BuffersCount + 3), Eq(false));
        ASSERT_THAT(_service.IsEmpty(0xff), Eq(false));
    }

    TEST_F(PhotoServiceTest, ShouldResetQueue)
    {
        EXPECT_CALL(_os, QueueReset(_));
        _service.PurgePendingCommands();
    }

    INSTANTIATE_TEST_CASE_P(PhotoServiceTest, PhotoServiceTest, testing::Values(Camera::Nadir, Camera::Wing), );
}
