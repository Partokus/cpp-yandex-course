#include "render.h"

using namespace std;
using namespace Json;
using namespace Svg;

using BusToColorMap = map<BusWeakPtr, Svg::Color, NameWeakPtrKeyLess<BusWeakPtr>>;

Svg::Color MakeColor(const Json::Node &color)
{
    Color result{};

    static constexpr size_t ColorRgbElemCount = 3U;
    static constexpr size_t ColorRgbaElemCount = 4U;

    if (color.IsString())
    {
        result = color.AsString();
        return result;
    }

    const vector<Node> &arr = color.AsArray();

    if (arr.size() == ColorRgbElemCount)
    {
        Rgb rgb{
            static_cast<uint8_t>(arr[0].AsInt()),
            static_cast<uint8_t>(arr[1].AsInt()),
            static_cast<uint8_t>(arr[2].AsInt())
        };
        result = rgb;
    }
    else if (arr.size() == ColorRgbaElemCount)
    {
        Rgba rgba{
            static_cast<uint8_t>(arr[0].AsInt()),
            static_cast<uint8_t>(arr[1].AsInt()),
            static_cast<uint8_t>(arr[2].AsInt()),
            arr[3].AsDouble()
        };
        result = rgba;
    }
    else
    {
        throw runtime_error("Unexpected array size\n");
    }

    return result;
}

RenderSettings MakeRenderSettigs(const map<string, Json::Node> &render_settings)
{
    RenderSettings result{};

    result.width = render_settings.at("width"s).AsDouble();
    result.height = render_settings.at("height"s).AsDouble();
    result.padding = render_settings.at("padding"s).AsDouble();
    result.stop_radius = render_settings.at("stop_radius"s).AsDouble();
    result.line_width = render_settings.at("line_width"s).AsDouble();
    result.underlayer_width = render_settings.at("underlayer_width"s).AsDouble();
    result.stop_label_font_size = render_settings.at("stop_label_font_size"s).AsInt();

    const vector<Node> &stop_label_offset = render_settings.at("stop_label_offset"s).AsArray();
    result.stop_label_offset = Point{ stop_label_offset[0].AsDouble(), stop_label_offset[1].AsDouble() };

    result.underlayer_color = MakeColor(render_settings.at("underlayer_color"s));

    const vector<Node> &color_palette = render_settings.at("color_palette"s).AsArray();
    for (const Node &node : color_palette)
        result.color_palette.push_back(MakeColor(node));

    if (render_settings.count("bus_label_font_size"s))
    {
        result.bus_label_font_size = render_settings.at("bus_label_font_size"s).AsInt();
        const vector<Node> &bus_label_offset = render_settings.at("bus_label_offset"s).AsArray();
        result.bus_label_offset = Point{ bus_label_offset[0].AsDouble(), bus_label_offset[1].AsDouble() };
    }

    return result;
}

// Проекция координат на карту
class PointCalculator
{
public:
    PointCalculator(DataBase &db)
        : _padding(db.render_settings.padding)
    {
        const RenderSettings &rs = db.render_settings;

        auto [it_min_lat, it_max_lat] = minmax_element(db.stops.begin(), db.stops.end(),
            [](const StopPtr &lhs, const StopPtr &rhs)
            { return lhs->latitude < rhs->latitude; });
        auto [it_min_lon, it_max_lon] = minmax_element(db.stops.begin(), db.stops.end(),
            [](const StopPtr &lhs, const StopPtr &rhs)
            { return lhs->longitude < rhs->longitude; });
        double min_lat = it_min_lat->get()->latitude;
        double max_lat = it_max_lat->get()->latitude;
        double min_lon = it_min_lon->get()->longitude;
        double max_lon = it_max_lon->get()->longitude;

        double zoom_coef = 0.0;
        double diff_lon = max_lon - min_lon;
        double diff_lat = max_lat - min_lat;
        double width_zoom_coef = 0.0;
        if (diff_lon != 0.0)
            width_zoom_coef = (rs.width - 2.0 * rs.padding) / diff_lon;
        double height_zoom_coef = 0.0;
        if (diff_lat != 0.0)
            height_zoom_coef = (rs.height - 2.0 * rs.padding) / diff_lat;

        if (diff_lon != 0.0 and diff_lat != 0.0)
            zoom_coef = min(width_zoom_coef, height_zoom_coef);
        else if (diff_lon != 0.0)
            zoom_coef = width_zoom_coef;
        else
            zoom_coef = height_zoom_coef;

        _zoom_coef = zoom_coef;
        _min_lon = min_lon;
        _max_lat = max_lat;
    }

    Svg::Point Calculate(double lat, double lon) const
    {
        return {
            (lon - _min_lon) * _zoom_coef + _padding,
            (_max_lat - lat) * _zoom_coef + _padding
        };
    }

private:
    double _padding = 0.0;
    double _min_lon = 0.0;
    double _max_lat = 0.0;
    double _zoom_coef = 0.0;
};

string CreateMap(DataBase &db)
{
    const RenderSettings &rs = db.render_settings;
    Svg::Text text{};
    Svg::Text text_back{};

    PointCalculator point_calculator{db};

    // Отрисовка маршрутов
    BusToColorMap buses_to_color{};
    for (const BusPtr &bus : db.buses)
        buses_to_color.insert({bus, Svg::Color{}});

    auto it_color = rs.color_palette.begin();
    for (auto &[bus, bus_color] : buses_to_color)
    {
        bus_color = *it_color;
        if (++it_color == rs.color_palette.end())
            it_color = rs.color_palette.begin();
    }

    Svg::Document doc{};
    Svg::Polyline polyline{};
    polyline.SetStrokeWidth(rs.line_width).
        SetStrokeLineCap("round").
        SetStrokeLineJoin("round");

    for (auto &[bus, bus_color] : buses_to_color)
    {
        polyline.SetStrokeColor(bus_color);

        const vector<StopPtr> &stops = bus.lock()->stops;
        for (const StopPtr &stop : stops)
        {
            polyline.AddPoint(point_calculator.Calculate(stop->latitude, stop->longitude));
        }

        if (not bus.lock()->ring)
        {
            for (auto it = next(stops.rbegin()); it != stops.rend(); ++it)
            {
                polyline.AddPoint(point_calculator.Calculate(it->get()->latitude, it->get()->longitude));
            }
        }

        doc.Add(polyline);
        polyline.points.clear();
    }

    // Отрисовка названий автобусов у конечных остановок (первой и последней)
    text = {};
    text.SetOffset(rs.bus_label_offset).
        SetFontSize(rs.bus_label_font_size).
        SetFontWeight("bold").
        SetFontFamily("Verdana");
    text_back = text;
    text_back.SetFillColor(rs.underlayer_color).
        SetStrokeColor(rs.underlayer_color).
        SetStrokeWidth(rs.underlayer_width).
        SetStrokeLineCap("round").
        SetStrokeLineJoin("round");

    for (auto &[bus_ptr, bus_color] : buses_to_color)
    {
        Bus &bus = *bus_ptr.lock().get();
        StopPtr first_stop = bus.stops.front();

        text.SetData(bus.name);
        text.SetFillColor(bus_color);
        text_back.SetData(bus.name);

        Svg::Point first_stop_point = point_calculator.Calculate(first_stop->latitude, first_stop->longitude);
        text.SetPoint(first_stop_point);
        text_back.SetPoint(first_stop_point);
        doc.Add(text_back);
        doc.Add(text);

        StopPtr last_stop = bus.stops.back();

        if (not bus.ring and first_stop->name != last_stop->name)
        {
            Svg::Point last_stop_point = point_calculator.Calculate(last_stop->latitude, last_stop->longitude);
            text.SetPoint(last_stop_point);
            text_back.SetPoint(last_stop_point);
            doc.Add(text_back);
            doc.Add(text);
        }
    }

    // Отрисовка остановок и их названий
    Svg::Circle circle{};
    circle.SetFillColor("white").
        SetRadius(rs.stop_radius);
    for (const StopPtr &stop : db.sorted_stops)
    {
        Point p = point_calculator.Calculate(stop->latitude, stop->longitude);
        circle.SetCenter(p);
        doc.Add(circle);
    }

    text = {};
    text.SetOffset(rs.stop_label_offset).
        SetFontSize(rs.stop_label_font_size).
        SetFontFamily("Verdana");
    text_back = text;
    text.SetFillColor("black");
    text_back.SetFillColor(rs.underlayer_color).
        SetStrokeColor(rs.underlayer_color).
        SetStrokeWidth(rs.underlayer_width).
        SetStrokeLineCap("round").
        SetStrokeLineJoin("round");
    for (const StopPtr &stop : db.sorted_stops)
    {
        Point p = point_calculator.Calculate(stop->latitude, stop->longitude);

        text.SetData(stop->name);
        text.SetPoint(p);
        text_back.SetData(stop->name);
        text_back.SetPoint(p);

        doc.Add(text_back);
        doc.Add(text);
    }

    ostringstream oss;
    oss << std::setprecision(16);
    doc.Render(oss);

    string map = oss.str();
    string result;

    for (char sym : map)
    {
        if (sym == '\"')
            result.push_back('\\');
        result.push_back(sym);
    }

    // cout << result << endl;

    return result;
}
