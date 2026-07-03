#pragma once

#include "trans_types.h"
#include "trans_data_base.h"
#include "json.h"
#include "router.h"
#include "svg.h"
#include "render.h"

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
#include <set>

using namespace std;

double ToRadians(double deg);
double CalcGeoDistance(double lat1, double lon1, double lat2, double lon2);

StopPtr ParseAddStopQuery(const map<string, Json::Node> &req, DataBase &db);
BusPtr ParseAddBusQuery(const map<string, Json::Node> &req, Stops &stops);
std::optional<RouteQueryAnswer> ParseRouteQuery(StopPtr from, StopPtr to, DataBase &db, Router &router);
void Parse(istream &is, ostream &os, DataBase &db);