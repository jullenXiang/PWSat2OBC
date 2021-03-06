#include "gtest/gtest.h"
#include "I2C/I2CMock.hpp"
#include "mock/error_counter.hpp"
#include "utils.hpp"

#include "rtc/rtc.hpp"

using namespace devices::rtc;

using testing::Return;
using testing::_;
using testing::Eq;
using testing::Ge;
using testing::Ne;
using testing::Invoke;
using testing::ElementsAre;
using testing::DoAll;
using gsl::span;
using drivers::i2c::I2CResult;

namespace
{
    class RTCObjectTest : public testing::Test
    {
      protected:
        RTCObjectTest();

        testing::NiceMock<ErrorCountingConfigrationMock> errorsConfig;
        error_counter::ErrorCounting errors;
        I2CBusMock i2c;
        RTCObject rtc;
        RTCObject::ErrorCounter error_counter;
    };

    RTCObjectTest::RTCObjectTest() : errors{errorsConfig}, rtc{errors, i2c}, error_counter{errors}
    {
    }

    TEST_F(RTCObjectTest, AllZeros)
    {
        EXPECT_CALL(i2c, WriteRead(RTCObject::I2CAddress, ElementsAre((std::uint8_t)Registers::VL_seconds), _))
            .WillOnce(Invoke([=](uint8_t /*address*/, span<const uint8_t> /*inData*/, span<uint8_t> outData) {
                std::fill(outData.begin(), outData.end(), 0);
                return I2CResult::OK;
            }));

        RTCTime time;
        ASSERT_THAT(rtc.ReadTime(time), Eq(OSResult::Success));

        ASSERT_THAT(time.seconds, Eq(0));
        ASSERT_THAT(time.minutes, Eq(0));
        ASSERT_THAT(time.hours, Eq(0));
        ASSERT_THAT(time.days, Eq(0));
        ASSERT_THAT(time.months, Eq(0));
        ASSERT_THAT(time.years, Eq(0));
        ASSERT_THAT(error_counter, Eq(0));
    }

    TEST_F(RTCObjectTest, Seconds)
    {
        EXPECT_CALL(i2c, WriteRead(RTCObject::I2CAddress, ElementsAre((std::uint8_t)Registers::VL_seconds), _))
            .WillOnce(Invoke([=](uint8_t /*address*/, span<const uint8_t> /*inData*/, span<uint8_t> outData) {
                std::fill(outData.begin(), outData.end(), 0);
                outData[0] = 0x23;
                return I2CResult::OK;
            }));

        RTCTime time;
        ASSERT_THAT(rtc.ReadTime(time), Eq(OSResult::Success));

        ASSERT_THAT(time.seconds, Eq(23));
        ASSERT_THAT(time.minutes, Eq(0));
        ASSERT_THAT(time.hours, Eq(0));
        ASSERT_THAT(time.days, Eq(0));
        ASSERT_THAT(time.months, Eq(0));
        ASSERT_THAT(time.years, Eq(0));
        ASSERT_THAT(error_counter, Eq(0));
    }

    TEST_F(RTCObjectTest, Minutes)
    {
        EXPECT_CALL(i2c, WriteRead(RTCObject::I2CAddress, ElementsAre((std::uint8_t)Registers::VL_seconds), _))
            .WillOnce(Invoke([=](uint8_t /*address*/, span<const uint8_t> /*inData*/, span<uint8_t> outData) {
                std::fill(outData.begin(), outData.end(), 0);
                outData[1] = 0x23;
                return I2CResult::OK;
            }));

        RTCTime time;
        ASSERT_THAT(rtc.ReadTime(time), Eq(OSResult::Success));

        ASSERT_THAT(time.seconds, Eq(0));
        ASSERT_THAT(time.minutes, Eq(23));
        ASSERT_THAT(time.hours, Eq(0));
        ASSERT_THAT(time.days, Eq(0));
        ASSERT_THAT(time.months, Eq(0));
        ASSERT_THAT(time.years, Eq(0));
        ASSERT_THAT(error_counter, Eq(0));
    }

    TEST_F(RTCObjectTest, Hours)
    {
        EXPECT_CALL(i2c, WriteRead(RTCObject::I2CAddress, ElementsAre((std::uint8_t)Registers::VL_seconds), _))
            .WillOnce(Invoke([=](uint8_t /*address*/, span<const uint8_t> /*inData*/, span<uint8_t> outData) {
                std::fill(outData.begin(), outData.end(), 0);
                outData[2] = 0x23;
                return I2CResult::OK;
            }));

        RTCTime time;
        ASSERT_THAT(rtc.ReadTime(time), Eq(OSResult::Success));

        ASSERT_THAT(time.seconds, Eq(0));
        ASSERT_THAT(time.minutes, Eq(0));
        ASSERT_THAT(time.hours, Eq(23));
        ASSERT_THAT(time.days, Eq(0));
        ASSERT_THAT(time.months, Eq(0));
        ASSERT_THAT(time.years, Eq(0));
        ASSERT_THAT(error_counter, Eq(0));
    }

    TEST_F(RTCObjectTest, Days)
    {
        EXPECT_CALL(i2c, WriteRead(RTCObject::I2CAddress, ElementsAre((std::uint8_t)Registers::VL_seconds), _))
            .WillOnce(Invoke([=](uint8_t /*address*/, span<const uint8_t> /*inData*/, span<uint8_t> outData) {
                std::fill(outData.begin(), outData.end(), 0);
                outData[3] = 0x23;
                return I2CResult::OK;
            }));

        RTCTime time;
        ASSERT_THAT(rtc.ReadTime(time), Eq(OSResult::Success));

        ASSERT_THAT(time.seconds, Eq(0));
        ASSERT_THAT(time.minutes, Eq(0));
        ASSERT_THAT(time.hours, Eq(0));
        ASSERT_THAT(time.days, Eq(23));
        ASSERT_THAT(time.months, Eq(0));
        ASSERT_THAT(time.years, Eq(0));
        ASSERT_THAT(error_counter, Eq(0));
    }

    TEST_F(RTCObjectTest, IgnoredField)
    {
        EXPECT_CALL(i2c, WriteRead(RTCObject::I2CAddress, ElementsAre((std::uint8_t)Registers::VL_seconds), _))
            .WillOnce(Invoke([=](uint8_t /*address*/, span<const uint8_t> /*inData*/, span<uint8_t> outData) {
                std::fill(outData.begin(), outData.end(), 0);
                outData[4] = 13;
                return I2CResult::OK;
            }));

        RTCTime time;
        ASSERT_THAT(rtc.ReadTime(time), Eq(OSResult::Success));

        ASSERT_THAT(time.seconds, Eq(0));
        ASSERT_THAT(time.minutes, Eq(0));
        ASSERT_THAT(time.hours, Eq(0));
        ASSERT_THAT(time.days, Eq(0));
        ASSERT_THAT(time.months, Eq(0));
        ASSERT_THAT(time.years, Eq(0));
        ASSERT_THAT(error_counter, Eq(0));
    }

    TEST_F(RTCObjectTest, Months)
    {
        EXPECT_CALL(i2c, WriteRead(RTCObject::I2CAddress, ElementsAre((std::uint8_t)Registers::VL_seconds), _))
            .WillOnce(Invoke([=](uint8_t /*address*/, span<const uint8_t> /*inData*/, span<uint8_t> outData) {
                std::fill(outData.begin(), outData.end(), 0);
                outData[5] = 0x11;
                return I2CResult::OK;
            }));

        RTCTime time;
        ASSERT_THAT(rtc.ReadTime(time), Eq(OSResult::Success));

        ASSERT_THAT(time.seconds, Eq(0));
        ASSERT_THAT(time.minutes, Eq(0));
        ASSERT_THAT(time.hours, Eq(0));
        ASSERT_THAT(time.days, Eq(0));
        ASSERT_THAT(time.months, Eq(11));
        ASSERT_THAT(time.years, Eq(0));
        ASSERT_THAT(error_counter, Eq(0));
    }

    TEST_F(RTCObjectTest, Years)
    {
        EXPECT_CALL(i2c, WriteRead(RTCObject::I2CAddress, ElementsAre((std::uint8_t)Registers::VL_seconds), _))
            .WillOnce(Invoke([=](uint8_t /*address*/, span<const uint8_t> /*inData*/, span<uint8_t> outData) {
                std::fill(outData.begin(), outData.end(), 0);
                outData[6] = 0x23;
                return I2CResult::OK;
            }));

        RTCTime time;
        ASSERT_THAT(rtc.ReadTime(time), Eq(OSResult::Success));

        ASSERT_THAT(time.seconds, Eq(0));
        ASSERT_THAT(time.minutes, Eq(0));
        ASSERT_THAT(time.hours, Eq(0));
        ASSERT_THAT(time.days, Eq(0));
        ASSERT_THAT(time.months, Eq(0));
        ASSERT_THAT(time.years, Eq(23));
        ASSERT_THAT(error_counter, Eq(0));
    }

    TEST_F(RTCObjectTest, FullRTCDate)
    {
        EXPECT_CALL(i2c, WriteRead(RTCObject::I2CAddress, ElementsAre((std::uint8_t)Registers::VL_seconds), _))
            .WillOnce(Invoke([=](uint8_t /*address*/, span<const uint8_t> /*inData*/, span<uint8_t> outData) {
                std::fill(outData.begin(), outData.end(), 0);
                // 23, 58, 10, 16, 0, 3, 17
                outData[0] = 0x23;
                outData[1] = 0x58;
                outData[2] = 0x10;
                outData[3] = 0x16;
                outData[4] = 0x00;
                outData[5] = 0x03;
                outData[6] = 0x17;
                return I2CResult::OK;
            }));

        RTCTime time;
        ASSERT_THAT(rtc.ReadTime(time), Eq(OSResult::Success));

        ASSERT_THAT(time.seconds, Eq(23));
        ASSERT_THAT(time.minutes, Eq(58));
        ASSERT_THAT(time.hours, Eq(10));
        ASSERT_THAT(time.days, Eq(16));
        ASSERT_THAT(time.months, Eq(3));
        ASSERT_THAT(time.years, Eq(17));
        ASSERT_THAT(error_counter, Eq(0));
    }

    TEST_F(RTCObjectTest, TestI2CFailure)
    {
        EXPECT_CALL(i2c, WriteRead(RTCObject::I2CAddress, ElementsAre((std::uint8_t)Registers::VL_seconds), _))
            .WillOnce(Invoke(
                [=](uint8_t /*address*/, span<const uint8_t> /*inData*/, span<uint8_t> /* outData */) { return I2CResult::Failure; }));

        RTCTime time;
        ASSERT_THAT(rtc.ReadTime(time), Eq(OSResult::InvalidOperation));
        ASSERT_THAT(error_counter, Eq(5));
    }

    TEST_F(RTCObjectTest, CheckIntegrityFlagSet)
    {
        EXPECT_CALL(i2c, WriteRead(RTCObject::I2CAddress, ElementsAre(num(Registers::VL_seconds)), SpanOfSize(1)))
            .WillOnce(DoAll(FillBuffer<2>(0b00000000), Return(I2CResult::OK)));
        auto r = rtc.IsIntegrityGuaranteed();
        ASSERT_THAT(r, Eq(true));
    }

    TEST_F(RTCObjectTest, CheckIntegrityFlagNotSet)
    {
        EXPECT_CALL(i2c, WriteRead(RTCObject::I2CAddress, ElementsAre(num(Registers::VL_seconds)), SpanOfSize(1)))
            .WillOnce(DoAll(FillBuffer<2>(0b10000000), Return(I2CResult::OK)));
        auto r = rtc.IsIntegrityGuaranteed();
        ASSERT_THAT(r, Eq(false));
    }

    TEST_F(RTCObjectTest, SetTime)
    {
        RTCTime time;
        time.years = 43;
        time.months = 5;
        time.days = 4;
        time.hours = 14;
        time.minutes = 47;
        time.seconds = 4;

        auto expected = ElementsAre(num(Registers::VL_seconds), 0x04, 0x47, 0x14, 0x04, 0x0, 0x05, 0x43);

        EXPECT_CALL(i2c, Write(RTCObject::I2CAddress, expected)).WillOnce(Return(I2CResult::OK));

        auto r = rtc.SetTime(time);
        ASSERT_THAT(r, Eq(OSResult::Success));
    }

    TEST_F(RTCObjectTest, SetTimeFail)
    {
        RTCTime time;

        EXPECT_CALL(i2c, Write(RTCObject::I2CAddress, _)).WillOnce(Return(I2CResult::Failure));
        auto r = rtc.SetTime(time);
        ASSERT_THAT(r, Eq(OSResult::DeviceNotFound));
        ASSERT_THAT(error_counter, Eq(5));
    }

    TEST_F(RTCObjectTest, ShouldDoNothingOnInitializeIfIntegrityIsOk)
    {
        ON_CALL(i2c, WriteRead(RTCObject::I2CAddress, ElementsAre(num(Registers::VL_seconds)), SpanOfSize(1)))
            .WillByDefault(DoAll(FillBuffer<2>(0b00000000), Return(I2CResult::OK)));

        auto r = rtc.Initialize();

        ASSERT_THAT(r, Eq(OSResult::Success));
    }

    TEST_F(RTCObjectTest, ShouldSetTimeOnInitializeIfIntegrityNotOk)
    {
        ON_CALL(i2c, WriteRead(RTCObject::I2CAddress, ElementsAre(num(Registers::VL_seconds)), SpanOfSize(1)))
            .WillByDefault(DoAll(FillBuffer<2>(0b10000000), Return(I2CResult::OK)));

        EXPECT_CALL(i2c, Write(RTCObject::I2CAddress, ElementsAre(num(Registers::VL_seconds), 0, 0, 0, 1, 0, 1, 0)))
            .WillOnce(Return(I2CResult::OK));

        auto r = rtc.Initialize();

        ASSERT_THAT(r, Eq(OSResult::Success));
    }

    TEST(RTCTimeTest, DecodeRTCTimeProperly)
    {
        RTCTime rtcTime;
        memset(&rtcTime, 0, sizeof(rtcTime));
        rtcTime.years = 17;
        rtcTime.months = 3;
        rtcTime.days = 31;
        rtcTime.hours = 5;
        rtcTime.minutes = 10;
        rtcTime.seconds = 59;

        auto sinceEpoch = rtcTime.ToDuration();

        ASSERT_THAT(sinceEpoch.count(), Eq(1490937059));
    }
}
