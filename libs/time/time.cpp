#include "TimePoint.h"

TimeSpan TimeSpanFromMilliseconds(uint64_t milliseconds)
{
    const TimeSpan span = {milliseconds};
    return span;
}

TimeSpan TimeSpanFromSeconds(uint32_t seconds)
{
    return TimeSpanFromMilliseconds(seconds * 1000ull);
}

TimeSpan TimeSpanFromMinutes(uint32_t minutes)
{
    return TimeSpanFromMilliseconds(minutes * 60000ull);
}

TimeSpan TimeSpanFromHours(uint32_t hours)
{
    return TimeSpanFromMilliseconds(hours * 3600000ull);
}

TimeSpan TimeSpanFromDays(uint32_t days)
{
    return TimeSpanFromMilliseconds(days * 24ull * 3600000ull);
}

TimePoint TimePointBuild(uint16_t day, uint8_t hour, uint8_t minute, uint8_t second, uint16_t millisecond)
{
    TimePoint point;
    point.milisecond = millisecond;
    point.second = second;
    point.minute = minute;
    point.hour = hour;
    point.day = day;
    return TimePointNormalize(point);
}

TimePoint TimePointNormalize(TimePoint point)
{
    return TimePointFromTimeSpan(TimePointToTimeSpan(point));
}

TimePoint TimePointFromTimeSpan(const TimeSpan timeSpan)
{
    TimePoint point = {};
    uint64_t span = timeSpan.value;
    point.milisecond = span % 1000;
    span /= 1000;
    point.second = span % 60;
    span /= 60;
    point.minute = span % 60;
    span /= 60;
    point.hour = span % 24;
    point.day = span / 24;
    return point;
}

TimeSpan TimePointToTimeSpan(TimePoint point)
{
    uint64_t result = point.day;
    result *= 24;
    result += point.hour;
    result *= 60;
    result += point.minute;
    result *= 60;
    result += point.second;
    result *= 1000;
    result += point.milisecond;
    return TimeSpanFromMilliseconds(result);
}

TimeSpan TimeSpanAdd(TimeSpan left, TimeSpan right)
{
    left.value += right.value;
    return left;
}

TimeShift TimeSpanSub(TimeSpan left, TimeSpan right)
{
    const TimeShift result = {static_cast<uint8_t>(left.value - right.value)};
    return result;
}