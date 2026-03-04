#pragma once
#include "svg.h"
#include <vector>

struct RenderSettings
{
    double width = 0.0;
    double height = 0.0;
    double padding = 0.0;
    double stop_radius = 0.0;
    double line_width = 0.0;
    size_t stop_label_font_size = 0U;
    Svg::Point stop_label_offset{};
    Svg::Color underlayer_color = Svg::NoneColor;
    double underlayer_width = 0.0;
    std::vector<Svg::Color> color_palette{};
};