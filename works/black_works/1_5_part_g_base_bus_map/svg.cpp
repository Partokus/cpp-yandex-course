#include "svg.h"

namespace Svg
{

void Circle::Render(ostream &os)
{
    os << "<circle " <<
        "cx=\"" << center.x << "\" cy=\"" << center.y << "\" " <<
        "r=\"" << radius << "\" ";

    RenderCommon(os);
    os << "/>";
}

void Polyline::Render(std::ostream &os)
{
    os << "<polyline points=\"";

    for (const Point point : points)
        os << point << ' ';

    os << "\" ";

    RenderCommon(os);
    os << "/>";
}

void Text::Render(ostream &os)
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

void Document::Render(ostream &os)
{
    os << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>";
    os << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">";

    for (const unique_ptr<ObjectAbstract> &obj : _objects)
    {
        obj->Render(os);
    }

    os << "</svg>";
}

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
        ASSERT_EQUAL(oss.str(), "rgba(1,2,3,0.800000)");
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