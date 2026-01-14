#include <profile.h>
#include <test_runner.h>

#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <utility>
#include <map>
#include <optional>
#include <unordered_set>
#include <algorithm>
#include <numeric>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <cmath>
#include <random>
#include <stdexcept>
#include <ctime>
#include <cmath>
#include <istream>
#include <variant>
#include <tuple>
#include <bitset>
#include <cassert>
#include <limits>

using namespace std;
using namespace std::chrono;
using namespace std::chrono_literals;

void TestAll();
void Profile();

namespace Svg
{

struct Point
{
    double x = 0.0;
    double y = 0.0;
};

struct Rgb
{
    uint8_t red = 0U;
    uint8_t green = 0U;
    uint8_t blue = 0U;
};

ostream & operator<<(ostream &os, const Rgb &v)
{
    return os << "rgb("    <<
        to_string(v.red)   << ',' <<
        to_string(v.green) << ',' <<
        to_string(v.blue)  << ')';
}

struct Color
{
    Color() = default;
    Color(const Rgb &v) : value(v) {}
    Color(const string &v) : value(v) {}

    variant<string, Rgb> value = "none"s;
};

ostream & operator<<(ostream &os, const Color &v)
{
    if (holds_alternative<string>(v.value))
        return os << get<string>(v.value);
    return os << get<Rgb>(v.value);
}

Color NoneColor{};

}

void TestRgbCout()
{
    using namespace Svg;
    {
        ostringstream oss;
        oss << Rgb{255, 255, 255};
        ASSERT_EQUAL(oss.str(), "rgb(255,255,255)");
    }
    {
        ostringstream oss;
        oss << Rgb{10, 50, 70};
        ASSERT_EQUAL(oss.str(), "rgb(10,50,70)");
    }
    {
        ostringstream oss;
        Rgb rgb;
        rgb.red = 10U;
        rgb.green = 50U;
        rgb.blue = 70U;
        oss << rgb;
        ASSERT_EQUAL(oss.str(), "rgb(10,50,70)");
    }
}

void TestColorCout()
{
    using namespace Svg;
    {
        ostringstream oss;
        Color color;
        oss << color;
        ASSERT_EQUAL(oss.str(), "none");
    }
    {
        ostringstream oss;
        oss << NoneColor;
        ASSERT_EQUAL(oss.str(), "none");
    }
    {
        ostringstream oss;
        Color color{"RED"};
        oss << color;
        ASSERT_EQUAL(oss.str(), "RED");
    }
    {
        ostringstream oss;
        Color color{Rgb{10, 0, 150}};
        oss << color;
        ASSERT_EQUAL(oss.str(), "rgb(10,0,150)");
    }
}

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, TestRgbCout);
    RUN_TEST(tr, TestColorCout);
}

void Profile()
{
}

int main()
{
    TestAll();

    return 0;
}