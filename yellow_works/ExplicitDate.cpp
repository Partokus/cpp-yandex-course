#include <test_runner.h>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <optional>
#include <tuple>
#include <chrono>

using namespace std;

class Year
{
public:
    explicit Year(int year) : _year(year) {}
    const int operator()() const
    {
        return _year;
    }

private:
    const int _year;
};

class Month
{
public:
    explicit Month(unsigned int month) : _month(month) {}
    const unsigned int operator()() const
    {
        return _month;
    }

private:
    const unsigned int _month;
};

class Day
{
public:
    explicit Day(unsigned int day) : _day(day) {}
    const unsigned int operator()() const
    {
        return _day;
    }

private:
    const unsigned int _day;
};

struct Date
{
    Year year;
    Month month;
    Day day;
};