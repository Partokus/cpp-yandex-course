#include "render.h"

using namespace std;
using namespace Json;
using namespace Svg;

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

    return result;
}

string CreateMap(DataBase &db)
{
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
        width_zoom_coef = (db.render_settings.width - 2.0 * db.render_settings.padding) / diff_lon;
    double height_zoom_coef = 0.0;
    if (diff_lat != 0.0)
        height_zoom_coef = (db.render_settings.height - 2.0 * db.render_settings.padding) / diff_lat;

    if (diff_lon != 0.0 and diff_lat != 0.0)
        zoom_coef = min(width_zoom_coef, height_zoom_coef);
    else if (diff_lon != 0.0)
        zoom_coef = width_zoom_coef;
    else
        zoom_coef = height_zoom_coef;

    auto CalcPoint = [min_lon, max_lat, zoom_coef, &db](double lat, double lon) {
        return Svg::Point{
            (lon - min_lon) * zoom_coef + db.render_settings.padding,
            (max_lat - lat) * zoom_coef + db.render_settings.padding
        };
    };

    map<BusWeakPtr, Svg::Color, NameWeakPtrKeyLess<BusWeakPtr>> buses{};
    for (const BusPtr &bus : db.buses)
        buses.insert({bus, Svg::Color{}});

    auto it_color = db.render_settings.color_palette.begin();
    for (auto &[bus, bus_color]  : buses)
    {
        bus_color = *it_color;
        if (++it_color == db.render_settings.color_palette.end())
            it_color = db.render_settings.color_palette.begin();
    }

    Svg::Document doc{};
    Svg::Polyline polyline{};
    polyline.SetStrokeWidth(db.render_settings.line_width).
        SetStrokeLineCap("round").
        SetStrokeLineJoin("round");

    for (auto &[bus, bus_color]  : buses)
    {
        polyline.SetStrokeColor(bus_color);

        for (const StopPtr &stop : bus.lock()->stops)
        {
            polyline.AddPoint(CalcPoint(stop->latitude, stop->longitude));
        }

        doc.Add(polyline);
        polyline.points.clear();
    }

    ostringstream oss;
    doc.Render(oss);

    // cout << oss.str();

    return oss.str();
}
