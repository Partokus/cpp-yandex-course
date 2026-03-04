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

void TestDocument();

namespace Svg
{

struct Point
{
    double x = 0.0;
    double y = 0.0;
};

ostream & operator<<(ostream &os, const Point &v)
{
    return os << v.x << ',' << v.y;
}

struct Rgb
{
    uint8_t red = 0U;
    uint8_t green = 0U;
    uint8_t blue = 0U;
};

struct Rgba
{
    uint8_t red = 0U;
    uint8_t green = 0U;
    uint8_t blue = 0U;
    double alpha = 0.0;
};

ostream & operator<<(ostream &os, const Rgb &v)
{
    return os << "rgb("    <<
        to_string(v.red)   << ',' <<
        to_string(v.green) << ',' <<
        to_string(v.blue)  << ')';
}

ostream & operator<<(ostream &os, const Rgba &v)
{
    return os << "rgb("     <<
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
};

ostream & operator<<(ostream &os, const Color &v)
{
    if (holds_alternative<Rgb>(v.value))
        return os << get<Rgb>(v.value);
    else if (holds_alternative<Rgba>(v.value))
        return os << get<Rgba>(v.value);
    return os << get<string>(v.value);
}

Color NoneColor{};

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

    void Render(ostream &os) override
    {
        os << "<circle " <<
            "cx=\"" << center.x << "\" cy=\"" << center.y << "\" " <<
            "r=\"" << radius << "\" ";

        RenderCommon(os);
        os << "/>";
    }

    Point center{};
    double radius = 1.0;
};

class Polyline : public ObjectAbstract, public Object<Polyline>
{
public:
    Polyline &  AddPoint(Point value) { points.push_back(value); return *this; }

    void Render(ostream &os) override
    {
        os << "<polyline points=\"";

        for (const Point point : points)
            os << point << ' ';

        os << "\" ";

        RenderCommon(os);
        os << "/>";
    }

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

    void Render(ostream &os) override
    {
        os << "<text " <<
            "x=\"" << point.x << "\" y=\"" << point.y << "\" " <<
            "dx=\"" << offset.x << "\" dy=\"" << offset.y << "\" " <<
            "font-size=\"" << font_size << "\" ";

        if (not font_family.empty())
            os << "font-family=\"" << font_family << "\" ";

        RenderCommon(os);
        os << ">" << data << "</text>";
    }

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

    void Render(ostream &os)
    {
        os << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>";
        os << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">";

        for (const unique_ptr<ObjectAbstract> &obj : _objects)
        {
            obj->Render(os);
        }

        os << "</svg>";
    }

private:
    vector<unique_ptr<ObjectAbstract>> _objects{};
};

} // namespace Svg

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
        oss << Rgb{33, 162, 226};
        ASSERT_EQUAL(oss.str(), "rgb(33,162,226)");
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
    {
        ostringstream oss;
        Color color{Rgba{1, 2, 3, 0.8}};
        oss << color;
        ASSERT_EQUAL(oss.str(), "rgb(1,2,3,0.800000)");
    }
}

void TestDocument()
{
    using namespace Svg;
    {
        Document doc{};
        doc.Add(Circle{}.SetCenter({1.0, 1.0}).SetFillColor(NoneColor).SetStrokeWidth(1.0));
    }
    {
        Document doc{};
        ostringstream oss;
        Circle{}.Render(oss);
        ASSERT_EQUAL(oss.str(), "<circle cx=\"0\" cy=\"0\" r=\"1\" fill=\"none\" stroke=\"none\" stroke-width=\"1\" />");
    }
    {
        Document doc{};
        ostringstream oss;
        Circle{}.SetStrokeLineJoin("MyLineJoin").Render(oss);
        ASSERT_EQUAL(oss.str(), "<circle cx=\"0\" cy=\"0\" r=\"1\" fill=\"none\" stroke=\"none\" stroke-width=\"1\" stroke-linejoin=\"MyLineJoin\" />");
    }
    {
        Document doc{};
        ostringstream oss;
        Circle{}.SetCenter({1.0, 1.0}).SetFillColor({"white"}).SetStrokeWidth(5.5).
            SetStrokeColor({"black"}).
            SetStrokeLineCap("MyLineCap").Render(oss);
        ASSERT_EQUAL(oss.str(), "<circle cx=\"1\" cy=\"1\" r=\"1\" fill=\"white\" stroke=\"black\" stroke-width=\"5.5\" stroke-linecap=\"MyLineCap\" />");
    }
    {
        Document doc{};
        ostringstream oss;
        Circle{}.SetCenter({1.0, 1.0}).SetFillColor({"white"}).SetStrokeWidth(5.5).
            SetStrokeColor({"black"}).
            SetStrokeLineCap("MyLineCap").
            SetStrokeLineJoin("MyLineJoin").Render(oss);
        ASSERT_EQUAL(oss.str(), "<circle cx=\"1\" cy=\"1\" r=\"1\" fill=\"white\" stroke=\"black\" stroke-width=\"5.5\" stroke-linecap=\"MyLineCap\" stroke-linejoin=\"MyLineJoin\" />");
    }
    {
        Document doc{};
        ostringstream oss;
        Circle{}.SetCenter({1.0, 1.0}).
            SetRadius(105.0).
            SetFillColor({"white"}).
            SetStrokeWidth(5.5).
            SetStrokeColor({"black"}).
            SetStrokeLineCap("MyLineCap").
            SetStrokeLineJoin("MyLineJoin").Render(oss);
        ASSERT_EQUAL(oss.str(), "<circle cx=\"1\" cy=\"1\" r=\"105\" fill=\"white\" stroke=\"black\" stroke-width=\"5.5\" stroke-linecap=\"MyLineCap\" stroke-linejoin=\"MyLineJoin\" />");
    }
    {
        Document doc{};
        ostringstream oss;
        Polyline{}.Render(oss);
        ASSERT_EQUAL(oss.str(), "<polyline points=\"\" fill=\"none\" stroke=\"none\" stroke-width=\"1\" />");
    }
    {
        Document doc{};
        ostringstream oss;
        Polyline{}.AddPoint({1.5, 2.6}).Render(oss);
        ASSERT_EQUAL(oss.str(), "<polyline points=\"1.5,2.6 \" fill=\"none\" stroke=\"none\" stroke-width=\"1\" />");
    }
    {
        Document doc{};
        ostringstream oss;
        Polyline{}.AddPoint({1.5, 2.6}).AddPoint({2.0, 2.0}).AddPoint({3.5, 3.5}).Render(oss);
        ASSERT_EQUAL(oss.str(), "<polyline points=\"1.5,2.6 2,2 3.5,3.5 \" fill=\"none\" stroke=\"none\" stroke-width=\"1\" />");
    }
    {
        Document doc{};
        ostringstream oss;
        Text{}.Render(oss);
        ASSERT_EQUAL(oss.str(), "<text x=\"0\" y=\"0\" dx=\"0\" dy=\"0\" font-size=\"1\" fill=\"none\" stroke=\"none\" stroke-width=\"1\" ></text>");
    }
    {
        Document doc{};
        ostringstream oss;
        Text{}.SetData("Lets go").Render(oss);
        ASSERT_EQUAL(oss.str(), "<text x=\"0\" y=\"0\" dx=\"0\" dy=\"0\" font-size=\"1\" fill=\"none\" stroke=\"none\" stroke-width=\"1\" >Lets go</text>");
    }
    {
        Document doc{};
        ostringstream oss;
        Text{}.SetData("Lets go").SetFontFamily("Vernanda").Render(oss);
        ASSERT_EQUAL(oss.str(), "<text x=\"0\" y=\"0\" dx=\"0\" dy=\"0\" font-size=\"1\" font-family=\"Vernanda\" fill=\"none\" stroke=\"none\" stroke-width=\"1\" >Lets go</text>");
    }
    {
        Document doc{};
        ostringstream oss;
        doc.Add(Circle{}.SetCenter({1.0, 1.0}).SetFillColor({"white"}).SetStrokeWidth(5.5).
            SetStrokeColor("black").
            SetStrokeLineCap("MyLineCap").
            SetStrokeLineJoin("MyLineJoin"));
        doc.Add(Polyline{}.AddPoint({1.5, 2.6}).AddPoint({2.0, 2.0}).AddPoint({3.5, 3.5}));
        doc.Add(Text{}.SetData("Lets go").SetFontFamily("Vernanda"));
        doc.Render(oss);
        ASSERT_EQUAL(oss.str(), "<?xml version=\"1.0\" encoding=\"UTF-8\" ?><svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\"><circle cx=\"1\" cy=\"1\" r=\"1\" fill=\"white\" stroke=\"black\" stroke-width=\"5.5\" stroke-linecap=\"MyLineCap\" stroke-linejoin=\"MyLineJoin\" /><polyline points=\"1.5,2.6 2,2 3.5,3.5 \" fill=\"none\" stroke=\"none\" stroke-width=\"1\" /><text x=\"0\" y=\"0\" dx=\"0\" dy=\"0\" font-size=\"1\" font-family=\"Vernanda\" fill=\"none\" stroke=\"none\" stroke-width=\"1\" >Lets go</text></svg>");
    }
}