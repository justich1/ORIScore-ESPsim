#pragma once

/*
  RTClib.h - stub knihovna pro ORIS ESPsim

  Podporuje hlavně:
    RTC_DS3231 rtc;
    rtc.begin();
    rtc.lostPower();
    rtc.adjust(DateTime(...));
    rtc.now();

  A DateTime:
    DateTime(year, month, day, hour, minute, second)
    DateTime(F(__DATE__), F(__TIME__))
    year(), month(), day(), hour(), minute(), second()
    dayOfTheWeek()

  Poznámka:
  - Na reálném ESP používej originální RTClib knihovnu od Adafruit.
*/

#include <Arduino.h>

class TimeSpan
{
public:
    TimeSpan(int32_t seconds = 0)
        : _seconds(seconds)
    {
    }

    TimeSpan(int16_t days, int8_t hours, int8_t minutes, int8_t seconds)
    {
        _seconds = (int32_t)days * 86400L
                 + (int32_t)hours * 3600L
                 + (int32_t)minutes * 60L
                 + seconds;
    }

    int16_t days() const
    {
        return _seconds / 86400L;
    }

    int8_t hours() const
    {
        return (_seconds / 3600L) % 24;
    }

    int8_t minutes() const
    {
        return (_seconds / 60L) % 60;
    }

    int8_t seconds() const
    {
        return _seconds % 60;
    }

    int32_t totalseconds() const
    {
        return _seconds;
    }

private:
    int32_t _seconds;
};

class DateTime
{
public:
    DateTime()
    {
        _year = 2026;
        _month = 1;
        _day = 1;
        _hour = 0;
        _minute = 0;
        _second = 0;
    }

    DateTime(uint16_t year, uint8_t month, uint8_t day,
             uint8_t hour = 0, uint8_t minute = 0, uint8_t second = 0)
    {
        _year = year;
        _month = month;
        _day = day;
        _hour = hour;
        _minute = minute;
        _second = second;
        normalize();
    }

    DateTime(const char* date, const char* time)
    {
        parseCompileDateTime(String(date), String(time));
        normalize();
    }

#ifdef __FlashStringHelper
    DateTime(const __FlashStringHelper* date, const __FlashStringHelper* time)
    {
        parseCompileDateTime(String(date), String(time));
        normalize();
    }
#endif

    uint16_t year() const
    {
        return _year;
    }

    uint8_t month() const
    {
        return _month;
    }

    uint8_t day() const
    {
        return _day;
    }

    uint8_t hour() const
    {
        return _hour;
    }

    uint8_t minute() const
    {
        return _minute;
    }

    uint8_t second() const
    {
        return _second;
    }

    // 0 = nedele, 1 = pondeli, ... 6 = sobota
    uint8_t dayOfTheWeek() const
    {
        int y = _year;
        int m = _month;
        int d = _day;

        if (m < 3)
        {
            m += 12;
            y--;
        }

        int k = y % 100;
        int j = y / 100;

        int h = (d + 13 * (m + 1) / 5 + k + k / 4 + j / 4 + 5 * j) % 7;

        // Zeller: 0=sobota, 1=nedele, 2=pondeli...
        // Chceme: 0=nedele, 1=pondeli...
        return (h + 6) % 7;
    }

    uint32_t unixtime() const
    {
        return dateToUnix(_year, _month, _day, _hour, _minute, _second);
    }

    DateTime operator+(const TimeSpan& span) const
    {
        return fromUnix(unixtime() + span.totalseconds());
    }

    DateTime operator-(const TimeSpan& span) const
    {
        return fromUnix(unixtime() - span.totalseconds());
    }

private:
    uint16_t _year;
    uint8_t _month;
    uint8_t _day;
    uint8_t _hour;
    uint8_t _minute;
    uint8_t _second;

    static bool isLeap(uint16_t y)
    {
        return ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0));
    }

    static uint8_t daysInMonth(uint16_t y, uint8_t m)
    {
        static const uint8_t days[] = {
            31, 28, 31, 30, 31, 30,
            31, 31, 30, 31, 30, 31
        };

        if (m == 2 && isLeap(y))
            return 29;

        if (m < 1 || m > 12)
            return 31;

        return days[m - 1];
    }

    void normalize()
    {
        if (_year < 2000) _year = 2000;
        if (_month < 1) _month = 1;
        if (_month > 12) _month = 12;

        uint8_t dim = daysInMonth(_year, _month);
        if (_day < 1) _day = 1;
        if (_day > dim) _day = dim;

        if (_hour > 23) _hour = 23;
        if (_minute > 59) _minute = 59;
        if (_second > 59) _second = 59;
    }

    void parseCompileDateTime(const String& date, const String& time)
    {
        // __DATE__ format: "Mmm dd yyyy"
        // __TIME__ format: "hh:mm:ss"

        String monStr = date.substring(0, 3);
        monStr.toLowerCase();

        if      (monStr == "jan") _month = 1;
        else if (monStr == "feb") _month = 2;
        else if (monStr == "mar") _month = 3;
        else if (monStr == "apr") _month = 4;
        else if (monStr == "may") _month = 5;
        else if (monStr == "jun") _month = 6;
        else if (monStr == "jul") _month = 7;
        else if (monStr == "aug") _month = 8;
        else if (monStr == "sep") _month = 9;
        else if (monStr == "oct") _month = 10;
        else if (monStr == "nov") _month = 11;
        else if (monStr == "dec") _month = 12;
        else _month = 1;

        _day = date.substring(4, 6).toInt();
        _year = date.substring(7, 11).toInt();

        _hour = time.substring(0, 2).toInt();
        _minute = time.substring(3, 5).toInt();
        _second = time.substring(6, 8).toInt();
    }

    static uint32_t dateToUnix(uint16_t y, uint8_t m, uint8_t d,
                               uint8_t hh, uint8_t mm, uint8_t ss)
    {
        uint32_t days = 0;

        for (uint16_t year = 1970; year < y; year++)
        {
            days += isLeap(year) ? 366UL : 365UL;
        }

        for (uint8_t month = 1; month < m; month++)
        {
            days += daysInMonth(y, month);
        }

        days += d - 1;

        return days * 86400UL + hh * 3600UL + mm * 60UL + ss;
    }

    static DateTime fromUnix(uint32_t t)
    {
        uint32_t days = t / 86400UL;
        uint32_t rem = t % 86400UL;

        uint8_t hh = rem / 3600UL;
        rem %= 3600UL;

        uint8_t mm = rem / 60UL;
        uint8_t ss = rem % 60UL;

        uint16_t y = 1970;

        while (true)
        {
            uint16_t dy = isLeap(y) ? 366 : 365;
            if (days >= dy)
            {
                days -= dy;
                y++;
            }
            else
            {
                break;
            }
        }

        uint8_t m = 1;

        while (true)
        {
            uint8_t dm = daysInMonth(y, m);
            if (days >= dm)
            {
                days -= dm;
                m++;
            }
            else
            {
                break;
            }
        }

        uint8_t d = days + 1;

        return DateTime(y, m, d, hh, mm, ss);
    }
};

class RTC_DS3231
{
public:
    RTC_DS3231()
    {
        _baseTime = DateTime(2026, 1, 1, 0, 0, 0);
        _baseMillis = 0;
        _started = false;
        _lostPower = false;
    }

    bool begin()
    {
        _started = true;

        if (_baseMillis == 0)
            _baseMillis = millis();

        return true;
    }

    bool lostPower()
    {
        return _lostPower;
    }

    void adjust(const DateTime& dt)
    {
        _baseTime = dt;
        _baseMillis = millis();
        _lostPower = false;
        _started = true;
    }

    DateTime now()
    {
        if (!_started)
            begin();

        uint32_t elapsedSec = (millis() - _baseMillis) / 1000UL;
        return _baseTime + TimeSpan((int32_t)elapsedSec);
    }

    float getTemperature()
    {
        return 25.0;
    }

private:
    DateTime _baseTime;
    unsigned long _baseMillis;
    bool _started;
    bool _lostPower;
};