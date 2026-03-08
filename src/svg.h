#pragma once
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

namespace Svg
{

struct Point
{
    double x = 0.0;
    double y = 0.0;

    bool operator==(const Point &o) const
    { return tie(x, y) == tie(o.x, o.y); }
};

inline ostream & operator<<(ostream &os, const Point &v)
{
    return os << v.x << ',' << v.y;
}

struct Rgb
{
    uint8_t red = 0U;
    uint8_t green = 0U;
    uint8_t blue = 0U;

    bool operator==(const Rgb &o) const
    { return tie(red, green, blue) == tie(o.red, o.green, o.blue); }
};

struct Rgba
{
    uint8_t red = 0U;
    uint8_t green = 0U;
    uint8_t blue = 0U;
    double alpha = 0.0;

    bool operator==(const Rgba &o) const
    { return tie(red, green, blue, alpha) == tie(o.red, o.green, o.blue, o.alpha); }
};

inline ostream & operator<<(ostream &os, const Rgb &v)
{
    return os << "rgb("    <<
        to_string(v.red)   << ',' <<
        to_string(v.green) << ',' <<
        to_string(v.blue)  << ')';
}

inline ostream & operator<<(ostream &os, const Rgba &v)
{
    return os << "rgba("     <<
        to_string(v.red)    << ',' <<
        to_string(v.green)  << ',' <<
        to_string(v.blue)   << ',' <<
        to_string(v.alpha)  << ')';
}

struct Color
{
    Color() = default;
    Color(const Rgb &v) : value(v) {}
    Color(const Rgba &v) : value(v) {}
    Color(const string &v) : value(v) {}
    Color(const char *v) : value(string(v)) {}

    variant<string, Rgb, Rgba> value = "none"s;

    bool operator==(const Color &o) const
    {
        if (value.index() != o.value.index())
            return false;
        if (holds_alternative<string>(value) and holds_alternative<string>(o.value))
            return get<string>(value) == get<string>(o.value);
        else if (holds_alternative<Rgb>(value) and holds_alternative<Rgb>(o.value))
            return get<Rgb>(value) == get<Rgb>(o.value);
        else if (holds_alternative<Rgba>(value) and holds_alternative<Rgba>(o.value))
            return get<Rgba>(value) == get<Rgba>(o.value);
        return false;
    }
};

inline ostream & operator<<(ostream &os, const Color &v)
{
    if (holds_alternative<Rgb>(v.value))
        return os << get<Rgb>(v.value);
    else if (holds_alternative<Rgba>(v.value))
        return os << get<Rgba>(v.value);
    return os << get<string>(v.value);
}

inline Color NoneColor{};

class ObjectAbstract
{
public:
    virtual ~ObjectAbstract() = default;
    virtual void Render(ostream &os) = 0;
};

template <typename T>
class Object
{
public:
    T & SetFillColor(const Color &value) { fill_color = value; return static_cast<T &>(*this); }
    T & SetStrokeColor(const Color &value) { stroke_color = value; return static_cast<T &>(*this); }
    T & SetStrokeWidth(double value) { stroke_width = value; return static_cast<T &>(*this); }
    T & SetStrokeLineCap(const string &value) { stroke_linecap = value; return static_cast<T &>(*this); } 
    T & SetStrokeLineJoin(const string &value) { stroke_linejoin = value; return static_cast<T &>(*this); }

    void RenderCommon(ostream &os) const
    {
        os << "fill=\"" << fill_color << "\" " <<
            "stroke=\"" << stroke_color << "\" " <<
            "stroke-width=\"" << stroke_width << "\" ";
        if (not stroke_linecap.empty())
            os << "stroke-linecap=\"" << stroke_linecap << "\" ";
        if (not stroke_linejoin.empty())
            os << "stroke-linejoin=\"" << stroke_linejoin << "\" ";
    }

    Color fill_color = NoneColor;
    Color stroke_color = NoneColor;
    double stroke_width = 1.0;
    string stroke_linecap{};
    string stroke_linejoin{};
};

class Circle : public ObjectAbstract, public Object<Circle>
{
public:
    Circle & SetFillColor(const Color &value) { fill_color = value; return *this; }
    Circle & SetStrokeColor(const Color &value) { stroke_color = value; return *this; }
    Circle & SetStrokeWidth(double value) { stroke_width = value; return *this; }
    Circle & SetStrokeLineCap(const string &value) { stroke_linecap = value; return *this; } 
    Circle & SetStrokeLineJoin(const string &value) { stroke_linejoin = value; return *this; } 

    Circle & SetCenter(Point value) { center = value; return *this; }
    Circle & SetRadius(double value) { radius = value; return *this; }

    void Render(ostream &os) override;

    Point center{};
    double radius = 1.0;
};

class Polyline : public ObjectAbstract, public Object<Polyline>
{
public:
    Polyline &  AddPoint(Point value) { points.push_back(value); return *this; }

    void Render(ostream &os) override;

    vector<Point> points{};
};

class Text : public ObjectAbstract, public Object<Text>
{
public:
    Text & SetPoint(Point value) { point = value; return *this; }
    Text & SetOffset(Point value) { offset = value; return *this; }
    Text & SetFontSize(uint32_t value) { font_size = value; return *this; }
    Text & SetFontFamily(const string &value) { font_family = value; return *this; }
    Text & SetData(const string &value) { data = value; return *this; }

    void Render(ostream &os) override;

    Point point{};
    Point offset{};
    uint32_t font_size = 1U;
    string font_family{};
    string data{};
};

class Document
{
public:
    template <typename T>
    void Add(T obj)
    {
        _objects.push_back(make_unique<T>(move(obj)));
    }

    void Render(ostream &os);

private:
    vector<unique_ptr<ObjectAbstract>> _objects{};
};

} // namespace Svg

void TestRgbCout();
void TestColorCout();
void TestDocument();