#ifndef UNIT_TESTS_RTC_RTCMOCK_HPP_
#define UNIT_TESTS_RTC_RTCMOCK_HPP_

#pragma once

#include "gmock/gmock.h"
#include "rtc/rtc.hpp"
#include "time/TimePoint.h"

class RtcMock : public devices::rtc::IRTC
{
  public:
    RtcMock() : readTimeResult(OSResult::Success)
    {
        fakeTime.seconds = 0;
        fakeTime.minutes = 0;
        fakeTime.hours = 0;
        fakeTime.days = 1;
        fakeTime.months = 1;
        fakeTime.years = 0;
    }

    OSResult ReadTime(devices::rtc::RTCTime& rtcTime)
    {
        rtcTime = fakeTime;

        return readTimeResult;
    }

    void SetTime(std::chrono::seconds duration)
    {
        auto timePoint = TimePointNormalize(TimePointFromDuration(duration));

        fakeTime.seconds = timePoint.second;
        fakeTime.minutes = timePoint.minute;
        fakeTime.hours = timePoint.hour;
    }

    void SetTime(devices::rtc::RTCTime& rtcTime)
    {
        fakeTime = rtcTime;
    }

    void AdvanceTime(std::chrono::seconds delta)
    {
        SetTime(fakeTime.ToDuration() + delta);
    }

    void SetReadResult(OSResult result)
    {
        readTimeResult = result;
    }

    std::chrono::seconds GetTime()
    {
        return this->fakeTime.ToDuration();
    }

    bool IsIntegrityGuaranteed()
    {
        return true;
    }

    OSResult SetTime(const devices::rtc::RTCTime& /*time*/)
    {
        return OSResult::Success;
    }

  private:
    devices::rtc::RTCTime fakeTime;
    OSResult readTimeResult;
};

#endif /* UNIT_TESTS_RTC_RTCMOCK_HPP_ */
