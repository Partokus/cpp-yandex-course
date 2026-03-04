#pragma once

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

template <typename PtrWithName>
struct NamePtrHasher
{
    size_t operator()(const PtrWithName &value) const
    {
        if (value == nullptr)
            return 0U;
        return str_hasher(value->name);
    }

    hash<string> str_hasher;
};

template <typename PtrWithName>
struct NamePtrKeyEqual
{
    bool operator()(const PtrWithName &lhs, const PtrWithName &rhs) const
    {
        if (lhs == nullptr and rhs == nullptr)
            return true;
        else if (lhs == nullptr or rhs == nullptr)
            return false;
        return lhs->name == rhs->name;
    }
};

template <typename PtrWithName>
struct NameWeakPtrKeyLess
{
    bool operator()(const PtrWithName &lhs, const PtrWithName &rhs) const
    {
        return lhs.lock()->name < rhs.lock()->name;
    }
};

struct Bus;
using BusPtr = shared_ptr<Bus>;
using BusWeakPtr = weak_ptr<Bus>;
using BusesSorted = set<BusWeakPtr, NameWeakPtrKeyLess<BusWeakPtr>>;

struct Stop
{
    string name;
    double latitude = 0.0;
    double longitude = 0.0;
    BusesSorted buses;

    bool operator==(const Stop &o) const
    { return name == o.name; }
};
using StopPtr = shared_ptr<Stop>;

struct Bus
{
    string name;
    vector<StopPtr> stops;
    bool ring = false;

    bool operator==(const Bus &o) const
    { return name == o.name; }
};

using Stops = unordered_set<StopPtr, NamePtrHasher<StopPtr>, NamePtrKeyEqual<StopPtr>>;
using Buses = unordered_set<BusPtr, NamePtrHasher<BusPtr>, NamePtrKeyEqual<BusPtr>>;

using Weight = double;
using DirectedWeightedGraph = Graph::DirectedWeightedGraph<Weight>;
using Router = Graph::Router<Weight>;
using RouteInfo = Router::RouteInfo;
using Edge = Graph::Edge<Weight>;

struct EdgeHasher
{
    size_t operator()(const Edge &value) const
    {
        size_t x1 = hasher(value.from);
        size_t x2 = hasher(value.to);
        return x1 * x1 + x2;
    }

    hash<Graph::VertexId> hasher;
};

struct DataBase
{
    Stops stops;
    Buses buses;

    struct BusInfo
    {
        size_t stops_on_route = 0U;
        size_t unique_stops = 0U;
        double route_length_geo = 0.0;
        size_t route_length_road = 0U;

        double curvature() const
        { return route_length_road / route_length_geo; }
    };

    struct RoutingSettings
    {
        size_t bus_wait_time = 0U; // мин
        double bus_velocity = 0.0; // км/час
        double bus_velocity_meters_min = 0.0; // м/мин
        double meters_past_while_wait_bus = 0.0;
    } routing_settings{};

    template <typename Key, typename Value>
    using UnorderedMap = unordered_map<Key, Value, NamePtrHasher<Key>, NamePtrKeyEqual<Key>>;

    UnorderedMap<BusPtr, BusInfo> buses_info;
    UnorderedMap<StopPtr, UnorderedMap<StopPtr, size_t>> road_route_length; // в метрах

    DirectedWeightedGraph graph{0};

    void CreateInfo(size_t bus_wait_time = 0U, double bus_velocity = 0.0, bool output = false);

    // Дорожная единица (Route Unit) представляет из себя структуру, в которой содержится
    // остановка, маршрут и следующая остановка в этом маршруте. Комбинации этих трёх состовляющих
    // достаточно, чтобы задать уникальную вершину без возникновения конфликтов с другими вершинами
    // Сигнатура: [stop][bus][next_stop] = vertex_id
    // next_stop может быть nullptr и это означает, что следующей остановки нет.
    // Под понятием "следующей остановки нет" скрывается, что текущая остановка является последним
    // элементов в векторе остановок автобуса, а не то, что остановка конечная
    UnorderedMap<StopPtr, UnorderedMap<BusPtr, UnorderedMap<StopPtr, Graph::VertexId>>> route_unit_to_vertex_id;
    // Сигнатура: vertex_id = [stop][bus][next_stop]
    unordered_map<Graph::VertexId, tuple<StopPtr, BusPtr, StopPtr>> vertex_id_to_route_unit;

    // Для переходов между разными маршрутами автобусов были добавлены
    // специальные вершины, которые символизируют конкретную остановку внезависимости
    // от маршрута и следующей остановки и являются так наызваюемым абстрактными остановками
    UnorderedMap<StopPtr, Graph::VertexId> abstract_stop_to_vertex_id;
    unordered_map<Graph::VertexId, StopPtr> vertex_id_to_abstract_stop;

private:
    Graph::VertexId _vertex_id = 0U;

    void CreateRoutingSettings(size_t bus_wait_time, double bus_velocity);

    // return meters
    std::optional<size_t> CalcRoadDistance(const StopPtr &lhs, const StopPtr &rhs) const;

    void CreateGraph(bool debug = false);
};

struct WaitItem
{
    StopPtr stop;
};

struct BusItem
{
    BusPtr bus;
    size_t span_count = 0U;
    double time = 0.0;
};

using Item = variant<WaitItem, BusItem>;

struct RouteQueryAnswer
{
    double total_time = 0.0; // суммарное время в минутах, требуемое для прохождения маршрута
    vector<Item> items;
};

double ToRadians(double deg);
double CalcGeoDistance(double lat1, double lon1, double lat2, double lon2);

StopPtr ParseAddStopQuery(const map<string, Json::Node> &req, DataBase &db);
BusPtr ParseAddBusQuery(const map<string, Json::Node> &req, Stops &stops);
std::optional<RouteQueryAnswer> ParseRouteQuery(StopPtr from, StopPtr to, DataBase &db, Router &router);
void Parse(istream &is, ostream &os);