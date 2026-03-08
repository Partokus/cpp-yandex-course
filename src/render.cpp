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

