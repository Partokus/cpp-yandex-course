#pragma once
#include "render_types.h"
#include "json.h"
#include "trans_data_base.h"

RenderSettings MakeRenderSettigs(const std::map<std::string, Json::Node> &render_settings);

std::string CreateMap(DataBase &db);