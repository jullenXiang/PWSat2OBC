#include <algorithm>
#include <em_i2c.h>
#include <gsl/span>
#include <string>
#include <tuple>
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "gmock/gmock-matchers.h"
#include "rapidcheck.hpp"
#include "rapidcheck/gtest.h"
#include "OsMock.hpp"
#include "i2c/I2CMock.hpp"
#include "i2c/i2c.h"
#include "os/os.hpp"
#include "system.h"
#include "utils.hpp"
#include "base/reader.h"
#include "base/writer.h"

#include "imtq/imtq.h"



using testing::_;
using testing::Eq;
using testing::Ne;
using testing::Ge;
using testing::StrEq;
using testing::Return;
using testing::Invoke;
using testing::Pointee;
using testing::ElementsAre;
using testing::Matches;
using gsl::span;
using drivers::i2c::I2CResult;
using namespace devices::imtq;

static const uint8_t ImtqAddress = 0x10;

#define FOR_AXIS(var) for(uint8_t var = 0; var < 3; ++var)

void WriteSignedWordToArray(int16_t value, uint8_t * dest)
{
	uint16_t val = *reinterpret_cast<uint16_t*>(&value);
	span<uint8_t> res{reinterpret_cast<uint8_t*>(&val), 2};
	std::copy(res.begin(), res.end(), dest);
}

TEST(ImtqHelper, SignedConversion)
{
	std::array<uint8_t, 2> result = {0, 0};

	WriteSignedWordToArray(5, result.begin());
	EXPECT_THAT(result, ElementsAre(0b00000101, 0b00000000));

	WriteSignedWordToArray(-5, result.begin());
	EXPECT_THAT(result, ElementsAre(0b11111011, 0b11111111));

	WriteSignedWordToArray(0, result.begin());
	EXPECT_THAT(result, ElementsAre(0b00000000, 0b00000000));

	WriteSignedWordToArray(12345, result.begin());
	EXPECT_THAT(result, ElementsAre(0b00111001, 0b00110000));

	WriteSignedWordToArray(-12345, result.begin());
	EXPECT_THAT(result, ElementsAre(0b11000111, 0b11001111));

	WriteSignedWordToArray(24321, result.begin());
	EXPECT_THAT(result, ElementsAre(0b00000001, 0b01011111));

	WriteSignedWordToArray(-24321, result.begin());
	EXPECT_THAT(result, ElementsAre(0b11111111, 0b10100000));
}

TEST(ImtqTestDataStructures, Status)
{
	devices::imtq::Status status{0x00};
	EXPECT_EQ(status.IsNew(), false);
	EXPECT_EQ(status.InvalidX(), false);
	EXPECT_EQ(status.InvalidY(), false);
	EXPECT_EQ(status.InvalidZ(), false);
	EXPECT_EQ(status.CmdError(), devices::imtq::Status::Error::Accepted);

	status = devices::imtq::Status{0x11};
	EXPECT_EQ(status.IsNew(), false);
	EXPECT_EQ(status.InvalidX(), false);
	EXPECT_EQ(status.InvalidY(), false);
	EXPECT_EQ(status.InvalidZ(), true);
	EXPECT_EQ(status.CmdError(), devices::imtq::Status::Error::Rejected);

	status = devices::imtq::Status{0x22};
	EXPECT_EQ(status.IsNew(), false);
	EXPECT_EQ(status.InvalidX(), false);
	EXPECT_EQ(status.InvalidY(), true);
	EXPECT_EQ(status.InvalidZ(), false);
	EXPECT_EQ(status.CmdError(), devices::imtq::Status::Error::InvalidCommandCode);

	status = devices::imtq::Status{0x33};
	EXPECT_EQ(status.IsNew(), false);
	EXPECT_EQ(status.InvalidX(), false);
	EXPECT_EQ(status.InvalidY(), true);
	EXPECT_EQ(status.InvalidZ(), true);
	EXPECT_EQ(status.CmdError(), devices::imtq::Status::Error::ParameterMissing);

	status = devices::imtq::Status{0x44};
	EXPECT_EQ(status.IsNew(), false);
	EXPECT_EQ(status.InvalidX(), true);
	EXPECT_EQ(status.InvalidY(), false);
	EXPECT_EQ(status.InvalidZ(), false);
	EXPECT_EQ(status.CmdError(), devices::imtq::Status::Error::ParameterInvalid);

	status = devices::imtq::Status{0x55};
	EXPECT_EQ(status.IsNew(), false);
	EXPECT_EQ(status.InvalidX(), true);
	EXPECT_EQ(status.InvalidY(), false);
	EXPECT_EQ(status.InvalidZ(), true);
	EXPECT_EQ(status.CmdError(), devices::imtq::Status::Error::CommandUnavailableInCurrentMode);

	status = devices::imtq::Status{0x67};
	EXPECT_EQ(status.IsNew(), false);
	EXPECT_EQ(status.InvalidX(), true);
	EXPECT_EQ(status.InvalidY(), true);
	EXPECT_EQ(status.InvalidZ(), false);
	EXPECT_EQ(status.CmdError(), devices::imtq::Status::Error::InternalError);

	status = devices::imtq::Status{0x70};
	EXPECT_EQ(status.IsNew(), false);
	EXPECT_EQ(status.InvalidX(), true);
	EXPECT_EQ(status.InvalidY(), true);
	EXPECT_EQ(status.InvalidZ(), true);
	EXPECT_EQ(status.CmdError(), devices::imtq::Status::Error::Accepted);

	status = devices::imtq::Status{0x81};
	EXPECT_EQ(status.IsNew(), true);
	EXPECT_EQ(status.InvalidX(), false);
	EXPECT_EQ(status.InvalidY(), false);
	EXPECT_EQ(status.InvalidZ(), false);
	EXPECT_EQ(status.CmdError(), devices::imtq::Status::Error::Rejected);

	status = devices::imtq::Status{0x92};
	EXPECT_EQ(status.IsNew(), true);
	EXPECT_EQ(status.InvalidX(), false);
	EXPECT_EQ(status.InvalidY(), false);
	EXPECT_EQ(status.InvalidZ(), true);
	EXPECT_EQ(status.CmdError(), devices::imtq::Status::Error::InvalidCommandCode);

	status = devices::imtq::Status{0xA3};
	EXPECT_EQ(status.IsNew(), true);
	EXPECT_EQ(status.InvalidX(), false);
	EXPECT_EQ(status.InvalidY(), true);
	EXPECT_EQ(status.InvalidZ(), false);
	EXPECT_EQ(status.CmdError(), devices::imtq::Status::Error::ParameterMissing);

	status = devices::imtq::Status{0xB4};
	EXPECT_EQ(status.IsNew(), true);
	EXPECT_EQ(status.InvalidX(), false);
	EXPECT_EQ(status.InvalidY(), true);
	EXPECT_EQ(status.InvalidZ(), true);
	EXPECT_EQ(status.CmdError(), devices::imtq::Status::Error::ParameterInvalid);

	status = devices::imtq::Status{0xC5};
	EXPECT_EQ(status.IsNew(), true);
	EXPECT_EQ(status.InvalidX(), true);
	EXPECT_EQ(status.InvalidY(), false);
	EXPECT_EQ(status.InvalidZ(), false);
	EXPECT_EQ(status.CmdError(), devices::imtq::Status::Error::CommandUnavailableInCurrentMode);

	status = devices::imtq::Status{0xD7};
	EXPECT_EQ(status.IsNew(), true);
	EXPECT_EQ(status.InvalidX(), true);
	EXPECT_EQ(status.InvalidY(), false);
	EXPECT_EQ(status.InvalidZ(), true);
	EXPECT_EQ(status.CmdError(), devices::imtq::Status::Error::InternalError);

	status = devices::imtq::Status{0xE0};
	EXPECT_EQ(status.IsNew(), true);
	EXPECT_EQ(status.InvalidX(), true);
	EXPECT_EQ(status.InvalidY(), true);
	EXPECT_EQ(status.InvalidZ(), false);
	EXPECT_EQ(status.CmdError(), devices::imtq::Status::Error::Accepted);

	status = devices::imtq::Status{0xF1};
	EXPECT_EQ(status.IsNew(), true);
	EXPECT_EQ(status.InvalidX(), true);
	EXPECT_EQ(status.InvalidY(), true);
	EXPECT_EQ(status.InvalidZ(), true);
	EXPECT_EQ(status.CmdError(), devices::imtq::Status::Error::Rejected);
}

class ImtqTest : public testing::Test
{
  public:
    ImtqTest() : imtq(i2c) {}

  protected:
  	devices::imtq::ImtqDriver imtq;
    I2CBusMock i2c;
};


TEST_F(ImtqTest, TestNoOperation)
{
	// accepted
    EXPECT_CALL(i2c, WriteRead(ImtqAddress, ElementsAre(0x02), _))
        .WillOnce(Invoke([](uint8_t /*address*/,
        		            auto /*inData*/,
							auto outData) {
			EXPECT_EQ(outData.size(), 2);
    		outData[0] = 0x02;
    		outData[1] = 0;
            return I2CResult::OK;
        }));

    auto status = imtq.SendNoOperation();
    EXPECT_TRUE(status);

	// command rejected
    EXPECT_CALL(i2c, WriteRead(ImtqAddress, ElementsAre(0x02), _))
		.WillOnce(Invoke([](uint8_t /*address*/,
							auto /*inData*/,
							auto outData) {
			EXPECT_EQ(outData.size(), 2);
    		outData[0] = 0x02;
    		outData[1] = 1;
            return I2CResult::OK;
        }));

    status = imtq.SendNoOperation();
    EXPECT_FALSE(status);

    // bad opcode response
    EXPECT_CALL(i2c, WriteRead(ImtqAddress, ElementsAre(0x02), _))
		.WillOnce(Invoke([](uint8_t /*address*/,
							auto /*inData*/,
							auto outData) {
			EXPECT_EQ(outData.size(), 2);
			outData[0] = 0x01;
			outData[1] = 0;
			return I2CResult::OK;
		}));

	status = imtq.SendNoOperation();
	EXPECT_FALSE(status);

	// I2C returned fail
	EXPECT_CALL(i2c, WriteRead(ImtqAddress, ElementsAre(0x02), _))
		.WillOnce(Invoke([](uint8_t /*address*/,
							auto /*inData*/,
							auto outData) {
			EXPECT_EQ(outData.size(), 2);
			outData[0] = 0x02;
			outData[1] = 0;
			return I2CResult::Failure;
		}));

	status = imtq.SendNoOperation();
	EXPECT_FALSE(status);
}

TEST_F(ImtqTest, SoftwareReset)
{
	// reset OK
    EXPECT_CALL(i2c, WriteRead(ImtqAddress, ElementsAre(0xAA), _))
		.WillOnce(Invoke([](uint8_t /*address*/,
							auto /*inData*/,
							auto /*outData*/) {
            return I2CResult::Nack;
        }));

    auto status = imtq.SoftwareReset();
    EXPECT_TRUE(status);

    // fast boot/delay on read
    EXPECT_CALL(i2c, WriteRead(ImtqAddress, ElementsAre(0xAA), _))
		.WillOnce(Invoke([](uint8_t /*address*/,
							auto /*inData*/,
							auto outData) {
			EXPECT_EQ(outData.size(), 2);
			outData[0] = 0xFF;
			outData[1] = 0xFF;
			return I2CResult::OK;
		}));

	status = imtq.SoftwareReset();
	EXPECT_TRUE(status);

	// I2C returned fail
	EXPECT_CALL(i2c, WriteRead(ImtqAddress, ElementsAre(0xAA), _))
		.WillOnce(Invoke([](uint8_t /*address*/,
							auto /*inData*/,
							auto /*outData*/) {
			return I2CResult::Failure;
		}));

	status = imtq.SoftwareReset();
	EXPECT_FALSE(status);

	// Reset rejected
	EXPECT_CALL(i2c, WriteRead(ImtqAddress, ElementsAre(0xAA), _))
		.WillOnce(Invoke([](uint8_t /*address*/,
							auto /*inData*/,
							auto outData) {
			EXPECT_EQ(outData.size(), 2);
			outData[0] = 0xAA;
			outData[1] = static_cast<uint8_t>(devices::imtq::Status::Error::Rejected);
			return I2CResult::OK;
		}));

	status = imtq.SoftwareReset();
	EXPECT_FALSE(status);
}

TEST_F(ImtqTest, CancelOperation)
{
	EXPECT_CALL(i2c, WriteRead(ImtqAddress, ElementsAre(0x03), _))
		.WillOnce(Invoke([](uint8_t /*address*/,
							auto /*inData*/,
							auto outData) {
			outData[0] = 0x03;
			outData[1] = 0;
			return I2CResult::OK;
		}));

	EXPECT_TRUE(imtq.CancelOperation());
}

TEST_F(ImtqTest, StartMTMMeasurement)
{
	// command accepted
	EXPECT_CALL(i2c, WriteRead(ImtqAddress, ElementsAre(0x04), _))
		.WillOnce(Invoke([](uint8_t /*address*/,
							auto /*inData*/,
							auto outData) {
			EXPECT_EQ(outData.size(), 2);
			outData[0] = 0x04;
			outData[1] = 0;
			return I2CResult::OK;
		}));

	EXPECT_TRUE(imtq.StartMTMMeasurement());
}

template<typename IMTQ, typename I2C, typename Method>
void testActuation(IMTQ& imtq, I2C& i2c, Method method, uint8_t command,
		std::array<int16_t, 3> values, uint16_t duration)
{
	EXPECT_CALL(i2c, WriteRead(ImtqAddress, BeginsWith(command), _))
		.WillOnce(Invoke([&](uint8_t /*address*/,
							auto inData,
							auto outData) {
			EXPECT_TRUE(inData.size() == 9);
			EXPECT_TRUE(outData.size() == 2);

			Reader reader{inData};
			reader.Skip(1);
			EXPECT_TRUE(reader.ReadSignedWordLE() == values[0]);
			EXPECT_TRUE(reader.ReadSignedWordLE() == values[1]);
			EXPECT_TRUE(reader.ReadSignedWordLE() == values[2]);

			EXPECT_TRUE(reader.ReadWordLE() == duration);

			outData[0] = command;
			outData[1] = 0;
			return I2CResult::OK;
		}));

	RC_ASSERT((imtq.*method)(values, std::chrono::milliseconds{duration}));
}

RC_GTEST_FIXTURE_PROP(ImtqTest, StartActuationCurrent,
		(std::array<int16_t, 3> currents, uint16_t duration))
{
	testActuation(imtq, i2c, &devices::imtq::ImtqDriver::StartActuationCurrent, 0x05,
			currents, duration);
}

RC_GTEST_FIXTURE_PROP(ImtqTest, StartActuationDipole,
		(std::array<int16_t, 3> dipole, uint16_t duration))
{
	testActuation(imtq, i2c, &devices::imtq::ImtqDriver::StartActuationDipole, 0x06,
			dipole, duration);
}

TEST_F(ImtqTest, StartAllAxisSelfTest)
{
	EXPECT_CALL(i2c, WriteRead(ImtqAddress, BeginsWith(0x08), _))
		.WillOnce(Invoke([](uint8_t /*address*/,
							auto inData,
							auto outData) {
			EXPECT_EQ(inData.size(), 2);
			EXPECT_EQ(outData.size(), 2);

			EXPECT_EQ(inData[1], 0x00);

			outData[0] = 0x08;
			outData[1] = 0;
			return I2CResult::OK;
		}));

	auto status = imtq.StartAllAxisSelfTest();
	EXPECT_TRUE(status);
}

RC_GTEST_FIXTURE_PROP(ImtqTest, StartBDotDetumbling, (uint16_t duration))
{
	EXPECT_CALL(i2c, WriteRead(ImtqAddress, BeginsWith(0x09), _))
		.WillOnce(Invoke([&](uint8_t /*address*/,
							auto inData,
							auto outData) {
			EXPECT_EQ(inData.size(), 3);
			EXPECT_EQ(outData.size(), 2);

			Reader reader{inData};
			reader.Skip(1);
			EXPECT_EQ(reader.ReadWordLE(), duration);

			outData[0] = 0x09;
			outData[1] = 0;
			return I2CResult::OK;
		}));

	RC_ASSERT(imtq.StartBDotDetumbling(std::chrono::seconds{duration}));
}

RC_GTEST_FIXTURE_PROP(ImtqTest, GetSystemState, ())
{
	uint8_t mode = *rc::gen::inRange(0, 3);
	uint8_t error = *rc::gen::inRange(0, 128);
	uint8_t conf = *rc::gen::inRange(0, 2);
	uint32_t uptime = *rc::gen::arbitrary<int>();

	EXPECT_CALL(i2c, WriteRead(ImtqAddress, ElementsAre(0x41), _))
		.WillOnce(Invoke([&](uint8_t /*address*/,
							auto /*inData*/,
							auto outData) {
			EXPECT_EQ(outData.size(), 9);

			Writer writer;
			WriterInitialize(&writer, outData.data(), outData.size());
			WriterWriteByte(&writer, 0x41);
			WriterWriteByte(&writer, 0);

			WriterWriteByte(&writer, mode); // mode
			WriterWriteByte(&writer, error); // error
			WriterWriteByte(&writer, conf); // conf
			WriterWriteDoubleWordLE(&writer, uptime); // uptime

			return I2CResult::OK;
		}));

	ImtqState state;
	RC_ASSERT(imtq.GetSystemState(state));

	RC_ASSERT(state.status.GetValue() == 0);
	RC_ASSERT(static_cast<uint8_t>(state.mode) == mode);
	RC_ASSERT(state.error.GetValue() == error);
}

RC_GTEST_FIXTURE_PROP(ImtqTest, GetCalibratedMagnetometerData,
		(std::array<int32_t, 3> data, bool newValue, bool coilAct))
{
	EXPECT_CALL(i2c, WriteRead(ImtqAddress, ElementsAre(0x43), _))
		.WillOnce(Invoke([=](uint8_t /*address*/,
							auto /*inData*/,
							auto outData) {
			EXPECT_EQ(outData.size(), 15);

			Writer writer;
			WriterInitialize(&writer, outData.data(), outData.size());
			WriterWriteByte(&writer, 0x43);
			WriterWriteByte(&writer, newValue << 7);

			WriterWriteSignedDoubleWordLE(&writer, data[0]);
			WriterWriteSignedDoubleWordLE(&writer, data[1]);
			WriterWriteSignedDoubleWordLE(&writer, data[2]);

			WriterWriteByte(&writer, coilAct);

			return I2CResult::OK;
		}));

	MagnetometerMeasurementResult mgtm;
	bool newFlag;
	RC_ASSERT(imtq.GetCalibratedMagnetometerData(mgtm, newFlag));

	RC_ASSERT(newFlag == newValue);
	RC_ASSERT(mgtm.coilActuationDuringMeasurement == coilAct);
	RC_ASSERT(mgtm.data == data);
}

template<typename IMTQ, typename I2C, typename Method>
void testCurrentTempDipole(IMTQ& imtq, I2C& i2c, Method method, uint8_t command,
		std::array<int16_t, 3> data)
{
	EXPECT_CALL(i2c, WriteRead(ImtqAddress, ElementsAre(command), _))
		.WillOnce(Invoke([&](uint8_t /*address*/,
							auto /*inData*/,
							auto outData) {
			EXPECT_TRUE(outData.size() == 8);

			Writer writer;
			WriterInitialize(&writer, outData.data(), outData.size());
			WriterWriteByte(&writer, command);
			WriterWriteByte(&writer, 0x00);

			WriterWriteSignedWordLE(&writer, data[0]);
			WriterWriteSignedWordLE(&writer, data[1]);
			WriterWriteSignedWordLE(&writer, data[2]);
			return I2CResult::OK;
		}));

	Vector3<int16_t> result;
	RC_ASSERT((imtq.*method)(result));
	RC_ASSERT(result == data);
}

RC_GTEST_FIXTURE_PROP(ImtqTest, GetCoilCurrent, (std::array<int16_t, 3> data))
{
	testCurrentTempDipole(imtq, i2c,
			&devices::imtq::ImtqDriver::GetCoilCurrent, 0x44, data);
}

RC_GTEST_FIXTURE_PROP(ImtqTest, GetCoilTemperature, (std::array<int16_t, 3> data))
{
	testCurrentTempDipole(imtq, i2c,
			&devices::imtq::ImtqDriver::GetCoilTemperature, 0x45, data);
}

RC_GTEST_FIXTURE_PROP(ImtqTest, GetCommandedActuationDipole, (std::array<int16_t, 3> data))
{
	testCurrentTempDipole(imtq, i2c,
			&devices::imtq::ImtqDriver::GetCommandedActuationDipole, 0x46, data);
}

inline bool operator==(const SelfTestResult::StepResult a, const SelfTestResult::StepResult b)
{
	if (a.error.GetValue() == b.error.GetValue() &&
		a.actualStep == b.actualStep &&
		a.RawMagnetometerMeasurement == b.RawMagnetometerMeasurement &&
		a.CalibratedMagnetometerMeasurement == b.CalibratedMagnetometerMeasurement &&
		a.CoilCurrent == b.CoilCurrent &&
		a.CoilTemperature == b.CoilTemperature)
		return true;
	return false;
}

RC_GTEST_FIXTURE_PROP(ImtqTest, GetSelfTestResult, ())
{
	SelfTestResult data;

	for(int i = 0; i < 8; ++i) {
		uint8_t err = *rc::gen::inRange(0, 128);
		data.stepResults[i].error = Error{err};
		data.stepResults[i].actualStep = static_cast<SelfTestResult::Step>(i);

		data.stepResults[i].RawMagnetometerMeasurement = *rc::gen::arbitrary<Vector3<int32_t>>();
		data.stepResults[i].CalibratedMagnetometerMeasurement = *rc::gen::arbitrary<Vector3<int32_t>>();
		data.stepResults[i].CoilCurrent = *rc::gen::arbitrary<Vector3<int16_t>>();
		data.stepResults[i].CoilTemperature = *rc::gen::arbitrary<Vector3<int16_t>>();
	}
	EXPECT_CALL(i2c, WriteRead(ImtqAddress, ElementsAre(0x47), _))
		.WillOnce(Invoke([&](uint8_t /*address*/,
							auto /*inData*/,
							auto outData) {
			EXPECT_TRUE(outData.size() == 320);

			Writer writer;
			WriterInitialize(&writer, outData.data(), outData.size());

			for(int i = 0; i < 8; ++i) {
				WriterWriteByte(&writer, 0x47);
				WriterWriteByte(&writer, 0x00);

				WriterWriteByte(&writer, data.stepResults[i].error.GetValue());
				WriterWriteByte(&writer, i);

				FOR_AXIS(x)
				{
					WriterWriteSignedDoubleWordLE(&writer, data.stepResults[i].RawMagnetometerMeasurement[x]);
				}
				FOR_AXIS(x)
				{
					WriterWriteSignedDoubleWordLE(&writer, data.stepResults[i].CalibratedMagnetometerMeasurement[x]);
				}
				FOR_AXIS(x)
				{
					WriterWriteSignedWordLE(&writer, data.stepResults[i].CoilCurrent[x]);
				}
				FOR_AXIS(x)
				{
					WriterWriteSignedWordLE(&writer, data.stepResults[i].CoilTemperature[x]);
				}
			}
			return I2CResult::OK;
		}));

	SelfTestResult result;
	RC_ASSERT(imtq.GetSelfTestResult(result));

	for(int i = 0; i < 8; ++i)
	{
		bool x = (result.stepResults[i] == data.stepResults[i]);
		RC_ASSERT(x);
	}
}



RC_GTEST_FIXTURE_PROP(ImtqTest, GetDetumbleData, ())
{
	DetumbleData data;

	data.calibratedMagnetometerMeasurement = *rc::gen::arbitrary<Vector3<int32_t>>();
	data.filteredMagnetometerMeasurement = *rc::gen::arbitrary<Vector3<int32_t>>();
	data.bDotData = *rc::gen::arbitrary<Vector3<int32_t>>();
	data.commandedDipole = *rc::gen::arbitrary<Vector3<int16_t>>();
	data.commandedCurrent = *rc::gen::arbitrary<Vector3<int16_t>>();
	data.measuredCurrent = *rc::gen::arbitrary<Vector3<int16_t>>();

	EXPECT_CALL(i2c, WriteRead(ImtqAddress, ElementsAre(0x48), _))
		.WillOnce(Invoke([&](uint8_t /*address*/,
							auto /*inData*/,
							auto outData) {
			EXPECT_TRUE(outData.size() == 56);

			Writer writer;
			WriterInitialize(&writer, outData.data(), outData.size());

			WriterWriteByte(&writer, 0x48);
			WriterWriteByte(&writer, 0x00);

			FOR_AXIS(x)
			{
				WriterWriteSignedDoubleWordLE(&writer, data.calibratedMagnetometerMeasurement[x]);
			}
			FOR_AXIS(x)
			{
				WriterWriteSignedDoubleWordLE(&writer, data.filteredMagnetometerMeasurement[x]);
			}
			FOR_AXIS(x)
			{
				WriterWriteSignedDoubleWordLE(&writer, data.bDotData[x]);
			}
			FOR_AXIS(x)
			{
				WriterWriteSignedWordLE(&writer, data.commandedDipole[x]);
			}
			FOR_AXIS(x)
			{
				WriterWriteSignedWordLE(&writer, data.commandedCurrent[x]);
			}
			FOR_AXIS(x)
			{
				WriterWriteSignedWordLE(&writer, data.measuredCurrent[x]);
			}
			return I2CResult::OK;
		}));

	DetumbleData result;
	RC_ASSERT(imtq.GetDetumbleData(result));

	RC_ASSERT(data.calibratedMagnetometerMeasurement == result.calibratedMagnetometerMeasurement);
}

RC_GTEST_FIXTURE_PROP(ImtqTest, GetHouseKeepingRAW, ())
{
	HouseKeepingRAW data;

	data.digitalVoltage = *rc::gen::arbitrary<uint16_t>();
	data.analogVoltage = *rc::gen::arbitrary<uint16_t>();
	data.digitalCurrent = *rc::gen::arbitrary<uint16_t>();
	data.analogCurrent = *rc::gen::arbitrary<uint16_t>();

	data.coilCurrent = *rc::gen::arbitrary<Vector3<uint16_t>>();
	data.coilTemperature = *rc::gen::arbitrary<Vector3<uint16_t>>();

	data.MCUtemperature = *rc::gen::arbitrary<uint16_t>();

	EXPECT_CALL(i2c, WriteRead(ImtqAddress, ElementsAre(0x49), _))
		.WillOnce(Invoke([&](uint8_t /*address*/,
							auto /*inData*/,
							auto outData) {
			EXPECT_TRUE(outData.size() == 24);

			Writer writer;
			WriterInitialize(&writer, outData.data(), outData.size());

			WriterWriteByte(&writer, 0x49);
			WriterWriteByte(&writer, 0x00);

			WriterWriteWordLE(&writer, data.digitalVoltage);
			WriterWriteWordLE(&writer, data.analogVoltage);
			WriterWriteWordLE(&writer, data.digitalCurrent);
			WriterWriteWordLE(&writer, data.analogCurrent);

			FOR_AXIS(x)
			{
				WriterWriteWordLE(&writer, data.coilCurrent[x]);
			}
			FOR_AXIS(x)
			{
				WriterWriteWordLE(&writer, data.coilTemperature[x]);
			}
			WriterWriteWordLE(&writer, data.MCUtemperature);
			return I2CResult::OK;
		}));

	HouseKeepingRAW result;
	RC_ASSERT(imtq.GetHouseKeepingRAW(result));

	RC_ASSERT(data.digitalVoltage == result.digitalVoltage);
	RC_ASSERT(data.analogVoltage == result.analogVoltage);
	RC_ASSERT(data.digitalCurrent == result.digitalCurrent);
	RC_ASSERT(data.analogCurrent == result.analogCurrent);
	RC_ASSERT(data.coilCurrent == result.coilCurrent);
	RC_ASSERT(data.coilTemperature == result.coilTemperature);
	RC_ASSERT(data.MCUtemperature == result.MCUtemperature);
}

RC_GTEST_FIXTURE_PROP(ImtqTest, GetHouseKeepingEngineering, ())
{
	HouseKeepingEngineering data;

	data.digitalVoltage = *rc::gen::arbitrary<uint16_t>();
	data.analogVoltage = *rc::gen::arbitrary<uint16_t>();
	data.digitalCurrent = *rc::gen::arbitrary<uint16_t>();
	data.analogCurrent = *rc::gen::arbitrary<uint16_t>();

	data.coilCurrent = *rc::gen::arbitrary<Vector3<int16_t>>();
	data.coilTemperature = *rc::gen::arbitrary<Vector3<int16_t>>();

	data.MCUtemperature = *rc::gen::arbitrary<int16_t>();

	EXPECT_CALL(i2c, WriteRead(ImtqAddress, ElementsAre(0x4A), _))
		.WillOnce(Invoke([&](uint8_t /*address*/,
							auto /*inData*/,
							auto outData) {
			EXPECT_TRUE(outData.size() == 24);

			Writer writer;
			WriterInitialize(&writer, outData.data(), outData.size());

			WriterWriteByte(&writer, 0x4A);
			WriterWriteByte(&writer, 0x00);

			WriterWriteWordLE(&writer, data.digitalVoltage);
			WriterWriteWordLE(&writer, data.analogVoltage);
			WriterWriteWordLE(&writer, data.digitalCurrent);
			WriterWriteWordLE(&writer, data.analogCurrent);

			FOR_AXIS(x)
			{
				WriterWriteWordLE(&writer, data.coilCurrent[x]);
			}
			FOR_AXIS(x)
			{
				WriterWriteWordLE(&writer, data.coilTemperature[x]);
			}
			WriterWriteWordLE(&writer, data.MCUtemperature);
			return I2CResult::OK;
		}));

	HouseKeepingEngineering result;
	RC_ASSERT(imtq.GetHouseKeepingEngineering(result));

	RC_ASSERT(data.digitalVoltage == result.digitalVoltage);
	RC_ASSERT(data.analogVoltage == result.analogVoltage);
	RC_ASSERT(data.digitalCurrent == result.digitalCurrent);
	RC_ASSERT(data.analogCurrent == result.analogCurrent);
	RC_ASSERT(data.coilCurrent == result.coilCurrent);
	RC_ASSERT(data.coilTemperature == result.coilTemperature);
	RC_ASSERT(data.MCUtemperature == result.MCUtemperature);
}

RC_GTEST_FIXTURE_PROP(ImtqTest, SetParameter, ())
{
	uint16_t param = *rc::gen::arbitrary<uint16_t>();
	const auto tab = *rc::gen::suchThat<std::vector<uint8_t>>([](std::vector<uint8_t> x) {
	  return (x.size() > 0 && x.size() < 9);
	});

	EXPECT_CALL(i2c, WriteRead(ImtqAddress, BeginsWith(0x82), _))
		.WillOnce(Invoke([&](uint8_t /*address*/,
							auto inData,
							auto outData) {
			EXPECT_TRUE((size_t)inData.size() == 3u + tab.size());
			EXPECT_TRUE((size_t)outData.size() == 4u + tab.size());

			Writer writer;
			WriterInitialize(&writer, outData.data(), outData.size());
			Reader reader{inData};
			reader.Skip(1);


			WriterWriteByte(&writer, 0x82);
			WriterWriteByte(&writer, 0x00);

			uint16_t id = reader.ReadWordLE();
			EXPECT_EQ(id, param);
			WriterWriteWordLE(&writer, id);

			auto value = reader.ReadToEnd();
			EXPECT_EQ(value, gsl::make_span(tab));
			WriterWriteArray(&writer, tab.data(), tab.size());
			return I2CResult::OK;
		}));

	RC_ASSERT(imtq.SetParameter(param, tab));
}

RC_GTEST_FIXTURE_PROP(ImtqTest, GetParameter, ())
{
	uint16_t param = *rc::gen::arbitrary<uint16_t>();
	const auto tab = *rc::gen::suchThat<std::vector<uint8_t>>([](std::vector<uint8_t> x) {
	  return (x.size() > 0 && x.size() < 9);
	});

	EXPECT_CALL(i2c, WriteRead(ImtqAddress, BeginsWith(0x81), _))
		.WillOnce(Invoke([&](uint8_t /*address*/,
							auto inData,
							auto outData) {
			EXPECT_TRUE(inData.size() == 3);
			EXPECT_TRUE((size_t)outData.size() == 4u + tab.size());

			Reader reader{inData};
			reader.Skip(1);
			uint16_t id = reader.ReadWordLE();
			EXPECT_EQ(id, param);


			Writer writer;
			WriterInitialize(&writer, outData.data(), outData.size());

			WriterWriteByte(&writer, 0x81);
			WriterWriteByte(&writer, 0x00);

			WriterWriteWordLE(&writer, id);
			WriterWriteArray(&writer, tab.data(), tab.size());
			return I2CResult::OK;
		}));

	std::vector<uint8_t> result;
	result.resize(tab.size());

	RC_ASSERT(imtq.GetParameter(param, result));

	RC_ASSERT(tab == result);
}

RC_GTEST_FIXTURE_PROP(ImtqTest, ResetParameter, ())
{
	uint16_t param = *rc::gen::arbitrary<uint16_t>();
	const auto tab = *rc::gen::suchThat<std::vector<uint8_t>>([](std::vector<uint8_t> x) {
	  return (x.size() > 0 && x.size() < 9);
	});

	EXPECT_CALL(i2c, WriteRead(ImtqAddress, BeginsWith(0x83), _))
		.WillOnce(Invoke([&](uint8_t /*address*/,
							auto inData,
							auto outData) {
			EXPECT_TRUE(inData.size() == 3);
			EXPECT_TRUE((size_t)outData.size() == 4u + tab.size());

			Reader reader{inData};
			reader.Skip(1);
			uint16_t id = reader.ReadWordLE();
			EXPECT_EQ(id, param);


			Writer writer;
			WriterInitialize(&writer, outData.data(), outData.size());

			WriterWriteByte(&writer, 0x83);
			WriterWriteByte(&writer, 0x00);

			WriterWriteWordLE(&writer, id);
			WriterWriteArray(&writer, tab.data(), tab.size());
			return I2CResult::OK;
		}));

	std::vector<uint8_t> result;
	result.resize(tab.size());

	RC_ASSERT(imtq.ResetParameterAndGetDefault(param, result));

	RC_ASSERT(tab == result);
}
