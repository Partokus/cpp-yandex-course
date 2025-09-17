#include <profile.h>
#include <test_runner.h>

#include "json.h"
#include "router.h"

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
#include <bitset>   // For std::bitset

using namespace std;

void TestAll();
void Profile();

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
struct NamePtrKeyLess
{
    bool operator()(const PtrWithName &lhs, const PtrWithName &rhs) const
    {
        return lhs->name < rhs->name;
    }
};

struct Bus;
using BusPtr = shared_ptr<Bus>;
using BusesSorted = set<BusPtr, NamePtrKeyLess<BusPtr>>;

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

double ToRadians(double deg)
{
    static constexpr double Pi = 3.1415926535;
    return deg * Pi / 180.0;
}

// Возвращает метры
// Haversine formula implementation
double CalcGeoDistance(double lat1, double lon1, double lat2, double lon2)
{
    static constexpr double EearthRaduis = 6'371'000.0; // m

    double dLat = ToRadians(lat2 - lat1);
    double dLon = ToRadians(lon2 - lon1);

    lat1 = ToRadians(lat1);
    lat2 = ToRadians(lat2);

    double a = sin(dLat / 2.0) * sin(dLat / 2.0) +
                cos(lat1) * cos(lat2) *
                sin(dLon / 2.0) * sin(dLon / 2.0);
    double c = 2.0 * atan2(sqrt(a), sqrt(1 - a));
    return EearthRaduis * c;
}

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
    using Map = unordered_map<Key, Value, NamePtrHasher<Key>, NamePtrKeyEqual<Key>>;
    template <typename Key>
    using Set = unordered_set<Key, NamePtrHasher<Key>, NamePtrKeyEqual<Key>>;

    Map<BusPtr, BusInfo> buses_info;
    Map<StopPtr, Map<StopPtr, size_t>> road_route_length; // в метрах

    DirectedWeightedGraph graph{0};

    void CreateInfo(size_t bus_wait_time = 0U, double bus_velocity = 0.0, bool output = false)
    {
        CreateRoutingSettings(bus_wait_time, bus_velocity);

        for (const BusPtr &bus : buses)
        {
            auto &info = buses_info[bus];

            if (bus->ring)
            {
                info.stops_on_route = bus->stops.size();
            }
            else
            {
                info.stops_on_route = bus->stops.size() * 2U - 1U;
            }

            const Stops unique_stops{ bus->stops.begin(), bus->stops.end() };
            info.unique_stops = unique_stops.size();

            for (auto it = bus->stops.begin(); it != bus->stops.end(); ++it)
            {
                auto it_next = next(it);
                const StopPtr &stop = *it;

                if (it_next == bus->stops.end())
                {
                    route_unit_to_vertex_id[stop][bus][nullptr] = _vertex_id;
                    vertex_id_to_route_unit[_vertex_id] = make_tuple(stop, bus, nullptr);
                    ++_vertex_id;
                    break;
                }

                const StopPtr &next_stop = *it_next;

                route_unit_to_vertex_id[stop][bus][next_stop] = _vertex_id;
                vertex_id_to_route_unit[_vertex_id] = make_tuple(stop, bus, next_stop);
                ++_vertex_id;

                info.route_length_geo += CalcGeoDistance(
                    stop->latitude, stop->longitude,
                    next_stop->latitude, next_stop->longitude
                );

                std::optional<size_t> road_distance = CalcRoadDistance(stop, next_stop);
                if (road_distance)
                    info.route_length_road += *road_distance;
                if (not bus->ring)
                {
                    road_distance = CalcRoadDistance(next_stop, stop);
                    if (road_distance)
                        info.route_length_road += *road_distance;
                }
            }

            if (not bus->ring)
                info.route_length_geo *= 2.0;
        }

        CreateGraph(output);
    }

    // [stop][bus][next_stop] = vertex_id
    // этой информации достаточно, чтобы однозначно определить вершину
    // next_stop может быть равен nullptr, и это означает, что следующей остановки нет
    Map<StopPtr, Map<BusPtr, Map<StopPtr, Graph::VertexId>>> route_unit_to_vertex_id;
    // vertex_id = [stop][bus][next_stop]
    unordered_map<Graph::VertexId, tuple<StopPtr, BusPtr, StopPtr>> vertex_id_to_route_unit; // TODO: переделать в vector

    Map<StopPtr, Graph::VertexId> shadow_stops_to_vertex_id; // Для хранения вершин, которые необходимы для
                                                             // выбора нужного автобуса в начале и в конце пути.
                                                             // Без этого непонятно будет какой автобус в начале выбрать,
                                                             // а это в свою очередь приводит к неправильному выбору оптимального пути
                                                             // за счёт того, что переход между остановками тоже занимает время
    unordered_map<Graph::VertexId, StopPtr> vertex_id_to_shadow_stops;

private:
    Graph::VertexId _vertex_id = 0U;

    void CreateRoutingSettings(size_t bus_wait_time, double bus_velocity)
    {
        routing_settings.bus_wait_time = bus_wait_time;
        routing_settings.bus_velocity = bus_velocity;
        routing_settings.bus_velocity_meters_min = bus_velocity * 1000.0 / 60.0;
        routing_settings.meters_past_while_wait_bus = bus_wait_time *
                                                      routing_settings.bus_velocity_meters_min;
    }

    // return meters
    std::optional<size_t> CalcRoadDistance(const StopPtr &lhs, const StopPtr &rhs) const
    {
        std::optional<size_t> result = std::nullopt;
        auto it_lhs = road_route_length.find(lhs);
        if (it_lhs != road_route_length.end())
        {
            auto it_rhs = it_lhs->second.find(rhs);
            if (it_rhs != it_lhs->second.end())
                result = it_rhs->second;
        }
        return result;
    }

    void CreateGraph(bool output = false)
    {
        if (output)
        {
            for (const auto &[stop, bus_to_next_stop] : route_unit_to_vertex_id)
            {
                for (const auto &[bus, next_stop_to_vertex_id] : bus_to_next_stop)
                {
                    for (const auto &[next_stop, vertex_id] : next_stop_to_vertex_id)
                    {
                        cout << "vertex_id[" << vertex_id << "] = { " <<
                            "stop( " << stop->name << " ), bus( " << bus->name << ") }" << endl;
                    }
                }
            }
        }

        // формируем исскуственные вершины для пересадок в начале и в конце пути
        for (const auto &[stop, bus_to_next_stop] : route_unit_to_vertex_id)
        {
            // если у остановки только один автобус и у этой остановки
            // на данном автобусе нет её копий с другой следующей остановкой
            // (то есть у автобуса все остановки уникальны)
            if (bus_to_next_stop.size() <= 1U and bus_to_next_stop.begin()->second.size() <= 1)
                continue;

            shadow_stops_to_vertex_id[stop] = _vertex_id;
            vertex_id_to_shadow_stops[_vertex_id] = stop;
            ++_vertex_id;
        }

        graph = DirectedWeightedGraph{ _vertex_id };

        for (const BusPtr &bus : buses)
        {
            for (auto it = bus->stops.begin(); it != bus->stops.end(); ++it)
            {
                auto it_next = next(it);
                if (it_next == bus->stops.end())
                {
                    break;
                }

                StopPtr &from = *it;
                StopPtr &to = *it_next;
                StopPtr to_next = next(it_next) != bus->stops.end() ?
                                  *next(it_next) : nullptr; // следующая остановка после to

                auto it_road_route = road_route_length.find(from);
                if (it_road_route == road_route_length.end())
                    return;
                auto it_length = it_road_route->second.find(to);
                if (it_length == it_road_route->second.end())
                    return;
                double road_distance = it_length->second; // weight, в метрах

                Graph::VertexId vertex_id_from = route_unit_to_vertex_id[from][bus][to];
                Graph::VertexId vertex_id_to = route_unit_to_vertex_id[to][bus][to_next];

                Edge edge{
                    .from = vertex_id_from,
                    .to = vertex_id_to,
                    .weight = road_distance
                };

                graph.AddEdge(edge);

                if (not bus->ring)
                {
                    road_distance = road_route_length.at(to).at(from);
                    edge = {
                        .from = vertex_id_to,
                        .to = vertex_id_from,
                        .weight = road_distance
                    };
                    graph.AddEdge(edge);
                }
                else
                {
                    // если это первая остановка, то делаем вид, что это последняя остановка,
                    // которая не имеет следующей остановки, так как конечная, и добавляем
                    // ребро перехода от неё ко второй остановке, учитывающей ещё и затрату на
                    // ожидание автобуса
                    if (it == bus->stops.begin())
                    {
                        Graph::VertexId vertex_id_from_with_wait_bus = route_unit_to_vertex_id[from][bus][nullptr];
                        edge.from = vertex_id_from_with_wait_bus;
                        edge.weight += routing_settings.meters_past_while_wait_bus;
                        graph.AddEdge(edge);
                    }
                }
            }
        }

        auto add_edge = [this](Graph::VertexId from, Graph::VertexId to)
        {
            Edge edge{
                .from = from,
                .to = to,
                .weight = routing_settings.meters_past_while_wait_bus / 2.0
            };
            graph.AddEdge(edge);
        };

        for (const auto &[stop, bus_to_next_stop] : route_unit_to_vertex_id)
        {
            // если у остановки только один автобус и у этой остановки
            // на данном автобусе нет её копий с другой следующей остановкой
            // (то есть у автобуса все остановки уникальны)
            if (bus_to_next_stop.size() <= 1U and bus_to_next_stop.begin()->second.size() <= 1)
                continue;

            for (auto it = bus_to_next_stop.begin(); it != bus_to_next_stop.end(); ++it)
            {
                const auto &[bus, next_stop_to_vertex_id] = *it;

                for (auto it2 = next_stop_to_vertex_id.begin(); it2 != next_stop_to_vertex_id.end(); ++it2)
                {
                    const auto &[next_stop, vertex_id] = *it2;

                    Graph::VertexId shadow_vertex_id = shadow_stops_to_vertex_id[stop];

                    add_edge(shadow_vertex_id, vertex_id);
                    add_edge(vertex_id, shadow_vertex_id);
                }
            }
        }
    }
};

/*
{
    "type": "Stop",
    "road_distances": {
        "Rossoshanskaya ulitsa": 7500,
        "Biryusinka": 1800,
        "Universam": 2400
    },
    "longitude": 37.6517,
    "name": "Biryulyovo Zapadnoye",
    "latitude": 55.574371
}
*/
StopPtr ParseAddStopQuery(const map<string, Json::Node> &req, DataBase &db)
{
    StopPtr result = make_shared<Stop>();

    result->name = req.at("name"s).AsString();
    result->latitude = req.at("latitude"s).AsDouble();
    result->longitude = req.at("longitude"s).AsDouble();

    const map<string, Json::Node> &road_distances = req.at("road_distances").AsMap();

    if (road_distances.empty())
        return result;

    auto &road_route_length = db.road_route_length[result];

    for (const auto &p : road_distances)
    {
        const string &stop_name = p.first;
        const size_t road_distance = p.second.AsInt();

        auto [it_stop, inserted] = db.stops.insert(make_shared<Stop>(Stop{ stop_name }));

        road_route_length[*it_stop] = road_distance;

        auto &road_route_length_rhs = db.road_route_length[*it_stop];

        if (auto it = road_route_length_rhs.find(result);
            it == road_route_length_rhs.end())
        {
            road_route_length_rhs[result] = road_distance;
        }
    }

    return result;
}

/*
{
    "type": "Bus",
    "name": "828",
    "stops": [
      "Biryulyovo Zapadnoye",
      "Universam",
      "Rossoshanskaya ulitsa",
      "Biryulyovo Zapadnoye"
    ],
    "is_roundtrip": true
}
*/
BusPtr ParseAddBusQuery(const map<string, Json::Node> &req, Stops &stops)
{
    using namespace Json;

    BusPtr result = make_shared<Bus>();
    result->name = req.at("name"s).AsString();
    result->ring = req.at("is_roundtrip"s).AsBool();

    auto push_stop = [&stops, &result](string name)
    {
        StopPtr stop = make_shared<Stop>(move(Stop{ move(name) }));
        auto [it, inserted] = stops.insert(stop);
        result->stops.push_back(*it);
        it->get()->buses.insert(result);
    };

    const vector<Node> json_stops = req.at("stops"s).AsArray();

    for (auto it = json_stops.begin(); it != json_stops.end(); ++it)
    {
        push_stop(it->AsString());
    }

    return result;
}

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

/*
{
  "type": "Route",
  "from": "Biryulyovo Zapadnoye",
  "to": "Universam",
  "id": 4
}
*/
std::optional<RouteQueryAnswer> ParseRouteQuery(StopPtr from, StopPtr to, DataBase &db, Router &router)
{
    if (from->name == to->name)
        return RouteQueryAnswer{};

    // проверяем, что для остановок имеются вершины
    if (db.route_unit_to_vertex_id.find(from) == db.route_unit_to_vertex_id.end() or
        db.route_unit_to_vertex_id.find(to) == db.route_unit_to_vertex_id.end())
    {
        return std::nullopt;
    }

    Graph::VertexId vertex_id_from = 0U;
    Graph::VertexId vertex_id_to = 0U;

    bool is_from_vertex_transfer = false;
    bool is_to_vertex_transfer = false;

    if (auto it = db.shadow_stops_to_vertex_id.find(from);
        it != db.shadow_stops_to_vertex_id.end())
    {
        vertex_id_from = it->second;
        is_from_vertex_transfer = true;
    }
    else
        vertex_id_from = db.route_unit_to_vertex_id[from].begin()->second.begin()->second;

    if (auto it = db.shadow_stops_to_vertex_id.find(to);
        it != db.shadow_stops_to_vertex_id.end())
    {
        vertex_id_to = it->second;
        is_to_vertex_transfer = true;
    }
    else
        vertex_id_to = db.route_unit_to_vertex_id[to].begin()->second.begin()->second;

    std::optional<RouteInfo> router_info = router.BuildRoute(vertex_id_from, vertex_id_to);

    if (not router_info)
        return std::nullopt;

    RouteQueryAnswer result{};

    // суммарное время равно длина дороги в метрах, делённая на скорость (метры/мин), плюс
    // время на ожидание первого автобуса
    result.total_time = router_info->weight / db.routing_settings.bus_velocity_meters_min +
        db.routing_settings.bus_wait_time;

    // если первая вершина или последняя исскуственная, сделанная для пересадки между автобусами,
    // то вычитаем время ожидания автобуса, так как для перехода между искусственной и настоящей остановки,
    // тратится время ожидания автобуса
    if (is_from_vertex_transfer)
        result.total_time -= db.routing_settings.bus_wait_time / 2.0;
    if (is_to_vertex_transfer)
        result.total_time -= db.routing_settings.bus_wait_time / 2.0;

    size_t edge_count = router_info->edge_count;

    // последнее ребро не учитываем, так как оно ведёт к исскуственной остановке
    if (is_to_vertex_transfer)
        --edge_count;

    result.items.push_back(WaitItem{ .stop = from });

    BusItem bus_item;

    // не учитываем первое ребро, если он ведёт от исскуственной вершины до настоящей,
    // принадлежащей конкретной остановке
    size_t begin_edge_idx = is_from_vertex_transfer ? 1U : 0U;

    auto push_items = [&result](BusItem &bus_item, const WaitItem &wait_item)
    {
        if (bus_item.span_count == 0U)
            throw runtime_error("span_count must not to be zero");

        result.items.push_back(bus_item);
        bus_item = {};

        result.items.push_back(wait_item);
    };

    for (size_t i = begin_edge_idx; i < edge_count; ++i)
    {
        Graph::EdgeId edge_id = router.GetRouteEdge(router_info->id, i);
        Graph::Edge edge = db.graph.GetEdge(edge_id);

        const auto &[stop_from, bus_from, next_stop_from] = db.vertex_id_to_route_unit.at(edge.from);

        auto it_to = db.vertex_id_to_route_unit.find(edge.to);
        if (it_to == db.vertex_id_to_route_unit.end())
        {
            StopPtr stop_to = db.vertex_id_to_shadow_stops[edge.to];
            bus_item.bus = bus_from;
            push_items(bus_item, WaitItem{ .stop = stop_to });
            // в следующем цикле stop_from будет равен теневой вершине,
            // поэтому прыгаем через одну вершину
            ++i;
            continue;
        }

        if (bus_from->ring and next_stop_from == nullptr)
        {
            bus_item.bus = bus_from;
            push_items(bus_item, WaitItem{ .stop = stop_from });

            ++bus_item.span_count;
            bus_item.time += (edge.weight - db.routing_settings.meters_past_while_wait_bus)
                / db.routing_settings.bus_velocity_meters_min;
        }
        else
        {
            ++bus_item.span_count;
            bus_item.time += edge.weight / db.routing_settings.bus_velocity_meters_min;
        }

        if (i == (edge_count - 1U))
        {
            bus_item.bus = bus_from;
            result.items.push_back(bus_item);
        }
    }

    router.ReleaseRoute(router_info->id);
    return { move(result) };
}

void Parse(istream &is, ostream &os)
{
    using namespace Json;

    os.precision(6);

    DataBase db{};
    Document doc = Load(is);

    const map<string, Node> &root = doc.GetRoot().AsMap();

    const vector<Node> &base_requests = root.at("base_requests"s).AsArray();

    for (const Node &node : base_requests)
    {
        const map<string, Node> &req = node.AsMap();
        const string &type = req.at("type"s).AsString();

        if (type == "Stop")
        {
            StopPtr stop = ParseAddStopQuery(req, db);
            auto [it, inserted] = db.stops.insert(stop);
            if (not inserted)
            {
                it->get()->latitude = stop->latitude;
                it->get()->longitude = stop->longitude;
            }
        }
        else if (type == "Bus")
        {
            BusPtr bus = ParseAddBusQuery(req, db.stops);
            db.buses.insert(move(bus));
        }
    }

    const map<string, Node> &routing_settings = root.at("routing_settings"s).AsMap();
    const size_t bus_wait_time = routing_settings.at("bus_wait_time"s).AsInt();
    const double bus_velocity = routing_settings.at("bus_velocity"s).AsDouble();

    db.CreateInfo(bus_wait_time, bus_velocity);

    Router router{db.graph};

    const vector<Node> &stat_requests = root.at("stat_requests"s).AsArray();

    os << "[" << '\n';

    for (auto it = stat_requests.begin(); it != stat_requests.end(); ++it)
    {
        const map<string, Node> &req = it->AsMap();
        const string &type = req.at("type"s).AsString();
        int id = req.at("id"s).AsInt();

        os << "  {" << '\n';

        os << "    \"request_id\": " << id << ",\n";

        if (type == "Bus")
        {
            const string &name = req.at("name"s).AsString();
            auto bus = make_shared<Bus>(Bus{});
            bus->name = name;

            if (auto it = db.buses_info.find(bus); it != db.buses_info.end())
            {
                auto &info = it->second;
                os << "    \"stop_count\": "        << info.stops_on_route    << ",\n"
                   << "    \"unique_stop_count\": " << info.unique_stops      << ",\n"
                   << "    \"route_length\": "      << info.route_length_road << ",\n"
                   << "    \"curvature\": "         << info.curvature()       << "\n";
            }
            else
                os << "    \"error_message\": \"not found\"" << '\n';
        }
        else if (type == "Stop")
        {
            const string &name = req.at("name"s).AsString();
            auto stop = make_shared<Stop>(Stop{});
            stop->name = name;

            if (auto it = db.stops.find(stop); it != db.stops.end())
            {
                const StopPtr &stop = *it;
                if (not stop->buses.empty())
                {
                    os << "    \"buses\": [" << '\n';

                    for (auto it_bus = stop->buses.begin();
                         it_bus != stop->buses.end();
                         ++it_bus)
                    {
                        os << "      \"" << it_bus->get()->name << '\"';
                        if (next(it_bus) != stop->buses.end())
                            os << ',';
                        os << '\n';
                    }

                    os << "    ]" << '\n';
                }
                else
                    os << "    \"buses\": []" << '\n';
            }
            else
                os << "    \"error_message\": \"not found\"" << '\n';
        }
        else if (type == "Route")
        {
            const string &from_name = req.at("from"s).AsString();
            auto stop_from = make_shared<Stop>(Stop{});
            stop_from->name = from_name;

            const string &to_name = req.at("to"s).AsString();
            auto stop_to = make_shared<Stop>(Stop{});
            stop_to->name = to_name;

            std::optional<RouteQueryAnswer> answer = ParseRouteQuery(stop_from, stop_to, db, router);

            if (not answer)
                os << "    \"error_message\": \"not found\"\n";
            else
            {
                os << "    \"total_time\": " << answer->total_time << ",\n";

                os << "    \"items\": [" << '\n';
                for (auto it = answer->items.begin(); it != answer->items.end(); ++it)
                {
                    os << "        {\n";

                    if (WaitItem *item = get_if<WaitItem>(&(*it)); item != nullptr)
                    {
                        os << "            \"time\": " << db.routing_settings.bus_wait_time << ",\n";
                        os << "            \"type\": \"Wait\"" << ",\n";
                        os << "            \"stop_name\": \"" << item->stop->name << "\"" << "\n";
                    }
                    else if (BusItem *item = get_if<BusItem>(&(*it)); item != nullptr)
                    {
                        os << "            \"span_count\": "  << item->span_count << ",\n";
                        os << "            \"bus\": \""       << item->bus->name << "\"" << ",\n";
                        os << "            \"type\": \"Bus\"" << ",\n";
                        os << "            \"time\": "        << item->time << "\n";
                    }

                    if (next(it) != answer->items.end())
                        os << "        },\n";
                    else
                        os << "        }\n";
                }
                os << "    ]" << '\n';
            }
        }

        os << "  }";
        if (next(it) != stat_requests.end())
            os << ',';
        os << '\n';
    }

    os << "]";
}


bool AssertDouble(double lhs, double rhs)
{
    return lhs > (rhs - 0.1) and lhs < (rhs + 0.1);
}

void TestParseAddStopQuery()
{
    using namespace Json;
    {
        istringstream input(R"({
    "type": "Stop",
    "road_distances": {},
    "longitude": 2.34,
    "name": "Empire   Street Building 5",
    "latitude": 55.611087
}
)");
        Document doc = Load(input);
        DataBase db;
        StopPtr stop = ParseAddStopQuery(doc.GetRoot().AsMap(), db);
        Stop expect{
            .name = "Empire   Street Building 5",
            .latitude = 55.611087,
            .longitude = 2.34
        };
        ASSERT_EQUAL(stop->name, expect.name);
        ASSERT_EQUAL(stop->latitude, expect.latitude);
        ASSERT_EQUAL(stop->longitude, expect.longitude);
        ASSERT_EQUAL(db.road_route_length.size(), 0U);
    }
    {
        istringstream input(R"({
    "type": "Stop",
    "road_distances": {},
    "longitude": 89.5,
    "name": "E",
    "latitude": 21.0
}
)");
        Document doc = Load(input);
        DataBase db;
        StopPtr stop = ParseAddStopQuery(doc.GetRoot().AsMap(), db);
        Stop expect{
            .name = "E",
            .latitude = 21.0,
            .longitude = 89.5
        };
        ASSERT_EQUAL(stop->name, expect.name);
        ASSERT_EQUAL(stop->latitude, expect.latitude);
        ASSERT_EQUAL(stop->longitude, expect.longitude);
        ASSERT_EQUAL(db.road_route_length.size(), 0U);
    }

    // with Dim to stop#
    {
        istringstream input(R"({
    "type": "Stop",
    "road_distances": {
        "Marushkino": 3900
    },
    "longitude": 89.591,
    "name": "Univer",
    "latitude": 21.666
}
)");
        Document doc = Load(input);
        DataBase db;
        StopPtr stop = ParseAddStopQuery(doc.GetRoot().AsMap(), db);
        Stop expect{
            .name = "Univer",
            .latitude = 21.666,
            .longitude = 89.591
        };
        ASSERT_EQUAL(stop->name, expect.name);
        ASSERT_EQUAL(stop->latitude, expect.latitude);
        ASSERT_EQUAL(stop->longitude, expect.longitude);
        auto it = db.road_route_length.find(stop);
        ASSERT(it != db.road_route_length.end());
        ASSERT_EQUAL(it->second.size(), 1U);
        ASSERT_EQUAL(it->second[make_shared<Stop>(Stop{"Marushkino"})], 3900U);
    }
    {
        istringstream input(R"([
    {
        "type": "Stop",
        "road_distances": {
            "Marushkino": 3900,
            "Sabotajnaya": 1000,
            "Meteornaya": 100000
        },
        "longitude": 89.591,
        "name": "Univer",
        "latitude": 21.666
    },
    {
        "type": "Stop",
        "road_distances": {
            "Univer": 500
        },
        "longitude": 89.2,
        "name": "Meteornaya",
        "latitude": 21.1
    }
]
)");
        Document doc = Load(input);
        DataBase db;
        StopPtr stop_univer = ParseAddStopQuery(doc.GetRoot().AsArray().at(0).AsMap(), db);
        StopPtr stop_meteornaya = ParseAddStopQuery(doc.GetRoot().AsArray().at(1).AsMap(), db);
        Stop expect{
            .name = "Univer",
            .latitude = 21.666,
            .longitude = 89.591
        };
        StopPtr stop_marushkino = make_shared<Stop>(Stop{"Marushkino"});
        StopPtr stop_sabotajnaya = make_shared<Stop>(Stop{"Sabotajnaya"});
        ASSERT_EQUAL(stop_univer->name, expect.name);
        ASSERT_EQUAL(stop_univer->latitude, expect.latitude);
        ASSERT_EQUAL(stop_univer->longitude, expect.longitude);
        expect.name = "Meteornaya";
        expect.latitude = 21.1;
        expect.longitude = 89.2;
        ASSERT_EQUAL(stop_meteornaya->name, expect.name);
        ASSERT_EQUAL(stop_meteornaya->latitude, expect.latitude);
        ASSERT_EQUAL(stop_meteornaya->longitude, expect.longitude);
        auto it = db.road_route_length.find(stop_univer);
        ASSERT(it != db.road_route_length.end());
        ASSERT_EQUAL(it->second.size(), 3U);
        ASSERT_EQUAL(it->second[stop_marushkino], 3900U);
        ASSERT_EQUAL(it->second[stop_sabotajnaya], 1000U);
        ASSERT_EQUAL(it->second[stop_meteornaya], 100000U);

        it = db.road_route_length.find(stop_marushkino);
        ASSERT(it != db.road_route_length.end());
        ASSERT_EQUAL(it->second.size(), 1U);
        ASSERT_EQUAL(it->second[stop_univer], 3900U);

        it = db.road_route_length.find(stop_sabotajnaya);
        ASSERT(it != db.road_route_length.end());
        ASSERT_EQUAL(it->second.size(), 1U);
        ASSERT_EQUAL(it->second[stop_univer], 1000U);

        it = db.road_route_length.find(stop_meteornaya);
        ASSERT(it != db.road_route_length.end());
        ASSERT_EQUAL(it->second.size(), 1U);
        ASSERT_EQUAL(it->second[stop_univer], 500U);
    }
}

void TestParseAddBusQuery()
{
    using namespace Json;
    {
        istringstream input(R"({
    "type": "Bus",
    "name": "256",
    "stops": [
        "Biryulyovo  Zapadnoye",
        "Biryusinka",
        "Universam",
        "Biryulyovo  Tovarnaya"
    ],
    "is_roundtrip": false
}
)");
        Document doc = Load(input);
        StopPtr stop1 = make_shared<Stop>(Stop{"Biryulyovo  Zapadnoye"});
        StopPtr stop2 = make_shared<Stop>(Stop{"Biryusinka"});
        StopPtr stop3 = make_shared<Stop>(Stop{"Universam"});
        StopPtr stop4 = make_shared<Stop>(Stop{"Biryulyovo  Tovarnaya"});
        Stops stops{stop1, stop2, stop3, stop4};
        BusPtr bus = ParseAddBusQuery(doc.GetRoot().AsMap(), stops);
        Bus expect{
            .name = "256",
            .stops = {stop1, stop2, stop3, stop4},
            .ring = false
        };
        ASSERT_EQUAL(bus->name, expect.name);
        for (int i = 0; i < 4; ++i)
        {
            ASSERT(bus->stops[i].get() == expect.stops[i].get());
            ASSERT(*bus->stops[i] == *expect.stops[i]);
        }
        ASSERT_EQUAL(bus->ring, expect.ring);
    }
    {
        istringstream input(R"({
    "type": "Bus",
    "name": "256",
    "stops": [
        "Biryulyovo Zapadnoye",
        "Biryusinka",
        "Universam",
        "Biryulyovo Tovarnaya",
        "Biryulyovo Passazhirskaya",
        "Biryulyovo Zapadnoye"
    ],
    "is_roundtrip": true
}
)");
        Document doc = Load(input);
        StopPtr stop1 = make_shared<Stop>(Stop{"Biryulyovo Zapadnoye"});
        StopPtr stop2 = make_shared<Stop>(Stop{"Biryusinka"});
        StopPtr stop3 = make_shared<Stop>(Stop{"Universam"});
        StopPtr stop4 = make_shared<Stop>(Stop{"Biryulyovo Tovarnaya"});
        StopPtr stop5 = make_shared<Stop>(Stop{"Biryulyovo Passazhirskaya"});
        Stops stops{stop1, stop2, stop3, stop4, stop5};
        BusPtr bus = ParseAddBusQuery(doc.GetRoot().AsMap(), stops);
        Bus expect{
            .name = "256",
            .stops = {stop1, stop2, stop3, stop4, stop5, stop1},
            .ring = true
        };
        ASSERT_EQUAL(bus->stops.size(), expect.stops.size());
        ASSERT_EQUAL(bus->name, expect.name);
        for (int i = 0; i < 5; ++i)
            ASSERT(bus->stops[i].get() == expect.stops[i].get());
        ASSERT_EQUAL(bus->ring, expect.ring);
    }
    {
        istringstream input(R"({
    "type": "Bus",
    "name": "My Bus",
    "stops": [
        "Biryulyovo Zapadnoye",
        "Tomash"
    ],
    "is_roundtrip": false
}
)");
        Document doc = Load(input);
        StopPtr stop1 = make_shared<Stop>(Stop{"Biryulyovo Zapadnoye"});
        StopPtr stop2 = make_shared<Stop>(Stop{"Tomash"});
        Stops stops{stop1, stop2};
        BusPtr bus = ParseAddBusQuery(doc.GetRoot().AsMap(), stops);
        Bus expect{
            .name = "My Bus",
            .stops = {stop1, stop2},
            .ring = false
        };
        ASSERT_EQUAL(bus->name, expect.name);
        ASSERT(bus->stops[0].get() == expect.stops[0].get());
        ASSERT(bus->stops[1].get() == expect.stops[1].get());
        ASSERT_EQUAL(bus->ring, expect.ring);
    }
    {
        istringstream input(R"({
    "type": "Bus",
    "name": "My Bus  HardBass",
    "stops": [
        "Biryulyovo",
        "Tomash"
    ],
    "is_roundtrip": false
}
)");
        Document doc = Load(input);
        StopPtr stop1 = make_shared<Stop>(Stop{"Biryulyovo"});
        StopPtr stop2 = make_shared<Stop>(Stop{"Tomash"});
        Stops stops{stop1, stop2};
        BusPtr bus = ParseAddBusQuery(doc.GetRoot().AsMap(), stops);
        Bus expect{
            .name = "My Bus  HardBass",
            .stops = {stop1, stop2},
            .ring = false
        };
        ASSERT_EQUAL(bus->name, expect.name);
        ASSERT(bus->stops[0].get() == expect.stops[0].get());
        ASSERT(bus->stops[1].get() == expect.stops[1].get());
        ASSERT_EQUAL(bus->ring, expect.ring);
    }
}

void TestCalcGeoDistance()
{
    {
        double lat1 = 55.611087;
        double lon1 = 37.20829;
        double lat2 = 55.595884;
        double lon2 = 37.209755;

        double length = CalcGeoDistance(lat1, lon1, lat2, lon2);
        ASSERT(length > 1692.0 and length < 1694.0);
        length = CalcGeoDistance(lat2, lon2, lat1, lon1);
        ASSERT(length > 1692.0 and length < 1694.0);
        length = CalcGeoDistance(lat1, lat2, lat1, lat2);
        ASSERT_EQUAL(length, 0.0);
    }
}

void TestDataBaseCreateInfo()
{
    {
        StopPtr stop1 = make_shared<Stop>(Stop{"Biryulyovo Zapadnoye"});
        StopPtr stop2 = make_shared<Stop>(Stop{"Biryusinka"});
        StopPtr stop3 = make_shared<Stop>(Stop{"Universam"});
        StopPtr stop4 = make_shared<Stop>(Stop{"Biryulyovo Tovarnaya"});
        StopPtr stop5 = make_shared<Stop>(Stop{"Biryulyovo Passazhirskaya"});
        StopPtr stop6 = make_shared<Stop>(Stop{"Nekrasovka"});
        StopPtr stop7 = make_shared<Stop>(Stop{"Lepeshkina"});

        BusPtr bus1 = make_shared<Bus>(Bus{"841", {stop1, stop2, stop3}});
        BusPtr bus2 = make_shared<Bus>(Bus{"842", {stop3, stop4, stop5, stop6}});
        BusPtr bus3 = make_shared<Bus>(Bus{"843", {stop6, stop5, stop4, stop3, stop7, stop6}});
        bus3->ring = true;

        // уникальные остановки: Biryulyovo Zapadnoye, Biryusinka, Lepeshkina

        DataBase db;
        db.stops = {stop1, stop2, stop3, stop4, stop5, stop6, stop7};
        db.buses = {bus1, bus2, bus3};

        db.CreateInfo();

        ASSERT_EQUAL(db.buses_info[bus1].stops_on_route, 5U);
        ASSERT_EQUAL(db.buses_info[bus2].stops_on_route, 7U);
        ASSERT_EQUAL(db.buses_info[bus3].stops_on_route, 6U);

        ASSERT_EQUAL(db.buses_info[bus1].unique_stops, 3U);
        ASSERT_EQUAL(db.buses_info[bus2].unique_stops, 4U);
        ASSERT_EQUAL(db.buses_info[bus3].unique_stops, 5U);
    }
    {
        StopPtr stop1 = make_shared<Stop>(Stop{"Tolstopaltsevo", 55.611087, 37.20829});
        StopPtr stop2 = make_shared<Stop>(Stop{"Marushkino", 55.595884, 37.209755});
        StopPtr stop3 = make_shared<Stop>(Stop{"Rasskazovka", 55.632761, 37.333324});
        StopPtr stop4 = make_shared<Stop>(Stop{"Biryulyovo Zapadnoye", 55.574371, 37.6517});
        StopPtr stop5 = make_shared<Stop>(Stop{"Biryusinka", 55.581065, 37.64839});
        StopPtr stop6 = make_shared<Stop>(Stop{"Universam", 55.587655, 37.645687});
        StopPtr stop7 = make_shared<Stop>(Stop{"Biryulyovo Tovarnaya", 55.592028, 37.653656});
        StopPtr stop8 = make_shared<Stop>(Stop{"Biryulyovo Passazhirskaya", 55.580999, 37.659164});

        BusPtr bus1 = make_shared<Bus>(Bus{"256", {stop4, stop5, stop6, stop7, stop8, stop4}});
        bus1->ring = true;
        BusPtr bus2 = make_shared<Bus>(Bus{"750", {stop1, stop2, stop3}});

        DataBase db;
        db.stops = {stop1, stop2, stop3, stop4, stop5, stop6, stop7, stop8};
        db.buses = {bus1, bus2};

        db.CreateInfo();

        ASSERT_EQUAL(db.buses_info[bus1].stops_on_route, 6U);
        ASSERT_EQUAL(db.buses_info[bus2].stops_on_route, 5U);

        ASSERT_EQUAL(db.buses_info[bus1].unique_stops, 5U);
        ASSERT_EQUAL(db.buses_info[bus2].unique_stops, 3U);

        ASSERT(db.buses_info[bus1].route_length_geo > 4370.0 and db.buses_info[bus1].route_length_geo < 4372.0);
        ASSERT(db.buses_info[bus2].route_length_geo > 20938.0 and db.buses_info[bus2].route_length_geo < 20940.0);
    }
    {
        StopPtr stop1 = make_shared<Stop>(Stop{"Tolstopaltsevo", 55.611087, 37.20829});
        StopPtr stop2 = make_shared<Stop>(Stop{"Marushkino", 55.595884, 37.209755});
        StopPtr stop3 = make_shared<Stop>(Stop{"Rasskazovka", 55.632761, 37.333324});

        BusPtr bus1 = make_shared<Bus>(Bus{"256", {stop1, stop2, stop3}});

        DataBase db;
        db.stops = {stop1, stop2, stop3};
        db.buses = {bus1};

        db.road_route_length[stop1][stop2] = 1000;
        db.road_route_length[stop2][stop1] = 1000;
        db.road_route_length[stop2][stop3] = 1500;
        db.road_route_length[stop3][stop2] = 1500;

        db.CreateInfo();

        ASSERT_EQUAL(db.buses_info[bus1].stops_on_route, 5U);
        ASSERT_EQUAL(db.buses_info[bus1].unique_stops, 3U);
        ASSERT(db.buses_info[bus1].route_length_geo > 20938.0 and db.buses_info[bus1].route_length_geo < 20940.0);
    }
}

void TestDataBaseCreateGraph()
{
    {
        StopPtr stop1 = make_shared<Stop>(Stop{"Biryulyovo Zapadnoye"});
        StopPtr stop2 = make_shared<Stop>(Stop{"Biryusinka"});
        StopPtr stop3 = make_shared<Stop>(Stop{"Universam"});
        StopPtr stop4 = make_shared<Stop>(Stop{"Nekrasovka"});
        StopPtr stop5 = make_shared<Stop>(Stop{"Malinovka"});

        BusPtr bus1 = make_shared<Bus>(Bus{"841", {stop1, stop2, stop3}});
        BusPtr bus2 = make_shared<Bus>(Bus{"842", {stop3, stop4, stop5}});

        DataBase db;
        db.stops = {stop1, stop2, stop3, stop4, stop5};
        db.buses = {bus1, bus2};

        db.road_route_length[stop1][stop2] = 1000;
        db.road_route_length[stop2][stop1] = 1000;
        db.road_route_length[stop2][stop3] = 500;
        db.road_route_length[stop3][stop2] = 500;
        db.road_route_length[stop3][stop4] = 800;
        db.road_route_length[stop4][stop3] = 800;
        db.road_route_length[stop4][stop5] = 300;
        db.road_route_length[stop5][stop4] = 300;

        db.CreateInfo(6, 40.0);

        ASSERT_EQUAL(db.routing_settings.bus_wait_time, 6U);
        ASSERT_EQUAL(db.routing_settings.bus_velocity, 40.0);
        ASSERT(AssertDouble(db.routing_settings.bus_velocity_meters_min, 666.6));
        ASSERT(AssertDouble(db.routing_settings.meters_past_while_wait_bus, 4000.0));

        /* bus1: 1 <-> 2 <-> 3
         * bus2: 3 <-> 4 <-> 5
         * bus1-bus2: 3 <-> 3
         * 3 (transfer) -> 3-bus1
         * 3 (transfer) -> 3-bus2
        */
        // ASSERT_EQUAL(db.graph.GetVertexCount(), 7U);
        // ASSERT_EQUAL(db.graph.GetEdgeCount(), 14U);
    }
    {
        StopPtr stop1 = make_shared<Stop>(Stop{"Biryulyovo Zapadnoye"});
        StopPtr stop2 = make_shared<Stop>(Stop{"Biryusinka"});
        StopPtr stop3 = make_shared<Stop>(Stop{"Universam"});
        StopPtr stop4 = make_shared<Stop>(Stop{"Nekrasovka"});
        StopPtr stop5 = make_shared<Stop>(Stop{"Malinovka"});

        BusPtr bus1 = make_shared<Bus>(Bus{"841", {stop1, stop2, stop3}});
        BusPtr bus2 = make_shared<Bus>(Bus{"842", {stop2, stop3, stop4, stop5}});

        DataBase db;
        db.stops = {stop1, stop2, stop3, stop4, stop5};
        db.buses = {bus1, bus2};

        db.road_route_length[stop1][stop2] = 1000;
        db.road_route_length[stop2][stop1] = 1000;
        db.road_route_length[stop2][stop3] = 500;
        db.road_route_length[stop3][stop2] = 500;
        db.road_route_length[stop3][stop4] = 800;
        db.road_route_length[stop4][stop3] = 800;
        db.road_route_length[stop4][stop5] = 300;
        db.road_route_length[stop5][stop4] = 300;

        db.CreateInfo();

        /* bus1: 1 <-> 2 <-> 3
         * bus2: 2 <-> 3 <-> 4 <-> 5
         * bus1-bus2: 2 <-> 2, 3 <-> 3
         * 2 (transfer) -> 2-bus1
         * 2 (transfer) -> 2-bus2
         * 3 (transfer) -> 3-bus1
         * 3 (transfer) -> 3-bus2
        */
        // ASSERT_EQUAL(db.graph.GetVertexCount(), 9U);
        // ASSERT_EQUAL(db.graph.GetEdgeCount(), 22U);
    }
    {
        StopPtr stop1 = make_shared<Stop>(Stop{"Biryulyovo Zapadnoye"});
        StopPtr stop2 = make_shared<Stop>(Stop{"Biryusinka"});
        StopPtr stop3 = make_shared<Stop>(Stop{"Universam"});
        StopPtr stop4 = make_shared<Stop>(Stop{"Nekrasovka"});
        StopPtr stop5 = make_shared<Stop>(Stop{"Malinovka"});

        BusPtr bus1 = make_shared<Bus>(Bus{"841", {stop1, stop2, stop3}});
        BusPtr bus2 = make_shared<Bus>(Bus{"842", {stop4, stop5}});

        DataBase db;
        db.stops = {stop1, stop2, stop3, stop4, stop5};
        db.buses = {bus1, bus2};

        db.road_route_length[stop1][stop2] = 1000;
        db.road_route_length[stop2][stop1] = 1000;
        db.road_route_length[stop2][stop3] = 500;
        db.road_route_length[stop3][stop2] = 500;
        db.road_route_length[stop4][stop5] = 300;
        db.road_route_length[stop5][stop4] = 300;

        db.CreateInfo();

        /* bus1: 1 <-> 2 <-> 3
         * bus2: 4 <-> 5
        */
        // ASSERT_EQUAL(db.graph.GetVertexCount(), 5U);
        // ASSERT_EQUAL(db.graph.GetEdgeCount(), 6U);
    }
    {
        StopPtr stop1 = make_shared<Stop>(Stop{"Biryulyovo Zapadnoye"});
        StopPtr stop2 = make_shared<Stop>(Stop{"Biryusinka"});
        StopPtr stop3 = make_shared<Stop>(Stop{"Universam"});
        StopPtr stop4 = make_shared<Stop>(Stop{"Nekrasovka"});
        StopPtr stop5 = make_shared<Stop>(Stop{"Malinovka"});

        BusPtr bus1 = make_shared<Bus>(Bus{"841", {stop1, stop2, stop3}});
        BusPtr bus2 = make_shared<Bus>(Bus{"842", {stop3, stop4, stop5, stop3}});
        bus2->ring = true;

        DataBase db;
        db.stops = {stop1, stop2, stop3, stop4, stop5};
        db.buses = {bus1, bus2};

        db.road_route_length[stop1][stop2] = 1000;
        db.road_route_length[stop2][stop1] = 1000;
        db.road_route_length[stop2][stop3] = 500;
        db.road_route_length[stop3][stop2] = 500;
        db.road_route_length[stop3][stop4] = 800;
        db.road_route_length[stop4][stop3] = 800;
        db.road_route_length[stop4][stop5] = 300;
        db.road_route_length[stop5][stop4] = 300;
        db.road_route_length[stop5][stop3] = 400;

        db.CreateInfo();

        /* bus1: 1 <-> 2 <-> 3
         * bus2: 3 -> 4 -> 5 -> 3
         * bus1-bus2: 3 <-> 3
         * 3 (transfer) -> 3-bus1
         * 3 (transfer) -> 3-bus2
        */
        // ASSERT_EQUAL(db.graph.GetVertexCount(), 7U);
        // ASSERT_EQUAL(db.graph.GetEdgeCount(), 13U);
    }
    {
        StopPtr stop1 = make_shared<Stop>(Stop{"Biryulyovo Zapadnoye"});
        StopPtr stop2 = make_shared<Stop>(Stop{"Biryusinka"});
        StopPtr stop3 = make_shared<Stop>(Stop{"Universam"});
        StopPtr stop4 = make_shared<Stop>(Stop{"Nekrasovka"});
        StopPtr stop5 = make_shared<Stop>(Stop{"Malinovka"});
        StopPtr stop6 = make_shared<Stop>(Stop{"Ejevikovka"});

        BusPtr bus1 = make_shared<Bus>(Bus{"841", {stop1, stop2, stop3}});
        BusPtr bus2 = make_shared<Bus>(Bus{"842", {stop3, stop4, stop5}});
        BusPtr bus3 = make_shared<Bus>(Bus{"843", {stop3, stop6}});

        DataBase db;
        db.stops = {stop1, stop2, stop3, stop4, stop5, stop6};
        db.buses = {bus1, bus2, bus3};

        db.road_route_length[stop1][stop2] = 1000;
        db.road_route_length[stop2][stop1] = 1000;
        db.road_route_length[stop2][stop3] = 500;
        db.road_route_length[stop3][stop2] = 500;
        db.road_route_length[stop3][stop4] = 800;
        db.road_route_length[stop4][stop3] = 800;
        db.road_route_length[stop4][stop5] = 300;
        db.road_route_length[stop5][stop4] = 300;
        db.road_route_length[stop3][stop6] = 200;
        db.road_route_length[stop6][stop3] = 200;

        db.CreateInfo();

        /* bus1: 1 <-> 2 <-> 3
         * bus2: 3 <-> 4 <-> 5
         * bus3: 3 <-> 6
         * bus1-bus2: 3 <-> 3
         * bus1-bus3: 3 <-> 3
         * bus2-bus3: 3 <-> 3
         * 3 (transfer) -> 3-bus1
         * 3 (transfer) -> 3-bus2
         * 3 (transfer) -> 3-bus3
        */
        // ASSERT_EQUAL(db.graph.GetVertexCount(), 9U);
        // ASSERT_EQUAL(db.graph.GetEdgeCount(), 22U);
    }
}

void TestBuildRoute()
{
    {
        StopPtr stop1 = make_shared<Stop>(Stop{"Biryulyovo Zapadnoye"});
        StopPtr stop2 = make_shared<Stop>(Stop{"Biryusinka"});
        StopPtr stop3 = make_shared<Stop>(Stop{"Universam"});
        StopPtr stop4 = make_shared<Stop>(Stop{"Nekrasovka"});
        StopPtr stop5 = make_shared<Stop>(Stop{"Malinovka"});
        StopPtr stop6 = make_shared<Stop>(Stop{"Ejevikovka"});
        StopPtr stop7 = make_shared<Stop>(Stop{"Apelsinovka"});
        StopPtr stop8 = make_shared<Stop>(Stop{"Kivivka"});

        BusPtr bus1 = make_shared<Bus>(Bus{"841", {stop1, stop2, stop3}});
        BusPtr bus2 = make_shared<Bus>(Bus{"842", {stop3, stop4, stop5}});
        BusPtr bus3 = make_shared<Bus>(Bus{"843", {stop3, stop6}});
        BusPtr bus4 = make_shared<Bus>(Bus{"844", {stop7, stop8}});

        DataBase db;
        db.stops = {stop1, stop2, stop3, stop4, stop5, stop6, stop7, stop8};
        db.buses = {bus1, bus2, bus3, bus4};

        db.road_route_length[stop1][stop2] = 1000;
        db.road_route_length[stop2][stop1] = 1000;
        db.road_route_length[stop2][stop3] = 500;
        db.road_route_length[stop3][stop2] = 500;
        db.road_route_length[stop3][stop4] = 800;
        db.road_route_length[stop4][stop3] = 800;
        db.road_route_length[stop4][stop5] = 300;
        db.road_route_length[stop5][stop4] = 300;
        db.road_route_length[stop3][stop6] = 200;
        db.road_route_length[stop6][stop3] = 200;
        db.road_route_length[stop7][stop8] = 100;
        db.road_route_length[stop8][stop7] = 200;

        db.CreateInfo(6/*bus_wait_time*/, 40.0/*bus_velocity km/hour*/);

        /* bus1: 1 <-> 2 <-> 3
         * bus2: 3 <-> 4 <-> 5
         * bus3: 3 <-> 6
         * bus4: 7 <-> 8
         * bus1-bus2: 3 <-> 3
         * bus1-bus3: 3 <-> 3
         * bus2-bus3: 3 <-> 3
         * 3 (transfer) -> 3-bus1
         * 3 (transfer) -> 3-bus2
         * 3 (transfer) -> 3-bus3
        */
        // ASSERT_EQUAL(db.graph.GetVertexCount(), 11U);
        // ASSERT_EQUAL(db.graph.GetEdgeCount(), 24U);

        Router router{db.graph};

        Graph::VertexId stop1_bus1_vertex_id = db.route_unit_to_vertex_id[stop1][bus1][stop2];
        Graph::VertexId stop2_bus1_vertex_id = db.route_unit_to_vertex_id[stop2][bus1][stop3];
        Graph::VertexId stop3_bus1_vertex_id = db.route_unit_to_vertex_id[stop3][bus1][nullptr];

        Graph::VertexId stop3_bus2_vertex_id = db.route_unit_to_vertex_id[stop3][bus2][stop4];
        Graph::VertexId stop4_bus2_vertex_id = db.route_unit_to_vertex_id[stop4][bus2][stop5];
        Graph::VertexId stop5_bus2_vertex_id = db.route_unit_to_vertex_id[stop5][bus2][nullptr];

        Graph::VertexId stop3_bus3_vertex_id = db.route_unit_to_vertex_id[stop3][bus3][stop6];
        Graph::VertexId stop6_bus3_vertex_id = db.route_unit_to_vertex_id[stop6][bus3][nullptr];

        Graph::VertexId stop7_bus4_vertex_id = db.route_unit_to_vertex_id[stop7][bus4][stop8];
        Graph::VertexId stop8_bus4_vertex_id = db.route_unit_to_vertex_id[stop8][bus4][nullptr];

        RouteInfo route_info = router.BuildRoute(stop1_bus1_vertex_id, stop3_bus1_vertex_id).value();
        ASSERT(AssertDouble(route_info.weight, 1500.0));
        ASSERT_EQUAL(route_info.edge_count, 2U);
        Graph::EdgeId edge_id = router.GetRouteEdge(route_info.id, 0);
        Edge edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop1_bus1_vertex_id);
        ASSERT_EQUAL(edge.to, stop2_bus1_vertex_id);
        edge_id = router.GetRouteEdge(route_info.id, 1);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop2_bus1_vertex_id);
        ASSERT_EQUAL(edge.to, stop3_bus1_vertex_id);

        router.ReleaseRoute(route_info.id);

        route_info = router.BuildRoute(stop3_bus1_vertex_id, stop1_bus1_vertex_id).value();
        ASSERT(AssertDouble(route_info.weight, 1500.0));
        ASSERT_EQUAL(route_info.edge_count, 2U);
        edge_id = router.GetRouteEdge(route_info.id, 0);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop3_bus1_vertex_id);
        ASSERT_EQUAL(edge.to, stop2_bus1_vertex_id);
        edge_id = router.GetRouteEdge(route_info.id, 1);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop2_bus1_vertex_id);
        ASSERT_EQUAL(edge.to, stop1_bus1_vertex_id);

        router.ReleaseRoute(route_info.id);

        route_info = router.BuildRoute(stop3_bus1_vertex_id, stop3_bus2_vertex_id).value();
        ASSERT(AssertDouble(route_info.weight, 4000.0));
        router.ReleaseRoute(route_info.id);
        route_info = router.BuildRoute(stop3_bus1_vertex_id, stop3_bus3_vertex_id).value();
        ASSERT(AssertDouble(route_info.weight, 4000.0));
        router.ReleaseRoute(route_info.id);
        route_info = router.BuildRoute(stop3_bus2_vertex_id, stop3_bus3_vertex_id).value();
        ASSERT(AssertDouble(route_info.weight, 4000.0));
        router.ReleaseRoute(route_info.id);

        route_info = router.BuildRoute(stop1_bus1_vertex_id, stop5_bus2_vertex_id).value();
        ASSERT(AssertDouble(route_info.weight, 6600.0));
        ASSERT_EQUAL(route_info.edge_count, 6U);
        edge_id = router.GetRouteEdge(route_info.id, 0);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop1_bus1_vertex_id);
        ASSERT_EQUAL(edge.to, stop2_bus1_vertex_id);
        edge_id = router.GetRouteEdge(route_info.id, 1);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop2_bus1_vertex_id);
        ASSERT_EQUAL(edge.to, stop3_bus1_vertex_id);
        edge_id = router.GetRouteEdge(route_info.id, 2);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop3_bus1_vertex_id);
        ASSERT_EQUAL(edge.to, db.shadow_stops_to_vertex_id[stop3]);
        edge_id = router.GetRouteEdge(route_info.id, 3);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, db.shadow_stops_to_vertex_id[stop3]);
        ASSERT_EQUAL(edge.to, stop3_bus2_vertex_id);
        edge_id = router.GetRouteEdge(route_info.id, 4);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop3_bus2_vertex_id);
        ASSERT_EQUAL(edge.to, stop4_bus2_vertex_id);
        edge_id = router.GetRouteEdge(route_info.id, 5);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop4_bus2_vertex_id);
        ASSERT_EQUAL(edge.to, stop5_bus2_vertex_id);

        route_info = router.BuildRoute(stop5_bus2_vertex_id, stop1_bus1_vertex_id).value();
        ASSERT(AssertDouble(route_info.weight, 6600.0));
        ASSERT_EQUAL(route_info.edge_count, 6U);
        edge_id = router.GetRouteEdge(route_info.id, 0);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop5_bus2_vertex_id);
        ASSERT_EQUAL(edge.to, stop4_bus2_vertex_id);
        edge_id = router.GetRouteEdge(route_info.id, 1);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop4_bus2_vertex_id);
        ASSERT_EQUAL(edge.to, stop3_bus2_vertex_id);
        edge_id = router.GetRouteEdge(route_info.id, 2);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop3_bus2_vertex_id);
        ASSERT_EQUAL(edge.to, db.shadow_stops_to_vertex_id[stop3]);
        edge_id = router.GetRouteEdge(route_info.id, 3);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, db.shadow_stops_to_vertex_id[stop3]);
        ASSERT_EQUAL(edge.to, stop3_bus1_vertex_id);
        edge_id = router.GetRouteEdge(route_info.id, 4);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop3_bus1_vertex_id);
        ASSERT_EQUAL(edge.to, stop2_bus1_vertex_id);
        edge_id = router.GetRouteEdge(route_info.id, 5);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop2_bus1_vertex_id);
        ASSERT_EQUAL(edge.to, stop1_bus1_vertex_id);

        router.ReleaseRoute(route_info.id);

        route_info = router.BuildRoute(stop1_bus1_vertex_id, stop6_bus3_vertex_id).value();
        ASSERT(AssertDouble(route_info.weight, 5700.0));
        ASSERT_EQUAL(route_info.edge_count, 5U);
        edge_id = router.GetRouteEdge(route_info.id, 0);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop1_bus1_vertex_id);
        ASSERT_EQUAL(edge.to, stop2_bus1_vertex_id);
        edge_id = router.GetRouteEdge(route_info.id, 1);
        edge = db.graph.GetEdge(edge_id);

        ASSERT_EQUAL(edge.from, stop2_bus1_vertex_id);
        ASSERT_EQUAL(edge.to, stop3_bus1_vertex_id);
        edge_id = router.GetRouteEdge(route_info.id, 2);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop3_bus1_vertex_id);
        ASSERT_EQUAL(edge.to, db.shadow_stops_to_vertex_id[stop3]);
        edge_id = router.GetRouteEdge(route_info.id, 3);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, db.shadow_stops_to_vertex_id[stop3]);
        ASSERT_EQUAL(edge.to, stop3_bus3_vertex_id);
        edge_id = router.GetRouteEdge(route_info.id, 4);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop3_bus3_vertex_id);
        ASSERT_EQUAL(edge.to, stop6_bus3_vertex_id);

        router.ReleaseRoute(route_info.id);

        route_info = router.BuildRoute(stop5_bus2_vertex_id, stop6_bus3_vertex_id).value();
        ASSERT(AssertDouble(route_info.weight, 5300.0));
        ASSERT_EQUAL(route_info.edge_count, 5U);
        edge_id = router.GetRouteEdge(route_info.id, 0);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop5_bus2_vertex_id);
        ASSERT_EQUAL(edge.to, stop4_bus2_vertex_id);
        edge_id = router.GetRouteEdge(route_info.id, 1);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop4_bus2_vertex_id);
        ASSERT_EQUAL(edge.to, stop3_bus2_vertex_id);
        edge_id = router.GetRouteEdge(route_info.id, 2);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop3_bus2_vertex_id);
        ASSERT_EQUAL(edge.to, db.shadow_stops_to_vertex_id[stop3]);
        edge_id = router.GetRouteEdge(route_info.id, 3);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, db.shadow_stops_to_vertex_id[stop3]);
        ASSERT_EQUAL(edge.to, stop3_bus3_vertex_id);
        edge_id = router.GetRouteEdge(route_info.id, 4);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop3_bus3_vertex_id);
        ASSERT_EQUAL(edge.to, stop6_bus3_vertex_id);

        router.ReleaseRoute(route_info.id);

        route_info = router.BuildRoute(stop7_bus4_vertex_id, stop8_bus4_vertex_id).value();
        ASSERT(AssertDouble(route_info.weight, 100.0));
        ASSERT_EQUAL(route_info.edge_count, 1U);
        edge_id = router.GetRouteEdge(route_info.id, 0);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop7_bus4_vertex_id);
        ASSERT_EQUAL(edge.to, stop8_bus4_vertex_id);

        router.ReleaseRoute(route_info.id);

        route_info = router.BuildRoute(stop8_bus4_vertex_id, stop7_bus4_vertex_id).value();
        ASSERT(AssertDouble(route_info.weight, 200.0));
        ASSERT_EQUAL(route_info.edge_count, 1U);
        edge_id = router.GetRouteEdge(route_info.id, 0);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop8_bus4_vertex_id);
        ASSERT_EQUAL(edge.to, stop7_bus4_vertex_id);

        router.ReleaseRoute(route_info.id);

        ASSERT(not router.BuildRoute(stop7_bus4_vertex_id, stop6_bus3_vertex_id));
        ASSERT(not router.BuildRoute(stop8_bus4_vertex_id, stop6_bus3_vertex_id));
        ASSERT(not router.BuildRoute(stop8_bus4_vertex_id, stop1_bus1_vertex_id));
        ASSERT(not router.BuildRoute(stop8_bus4_vertex_id, stop3_bus2_vertex_id));
        ASSERT(not router.BuildRoute(stop7_bus4_vertex_id, stop5_bus2_vertex_id));
    }
}

void TestParseJson()
{
    istringstream input(R"({
  "base_requests": [
    {
      "type": "Stop",
      "road_distances": {
        "Marushkino": 3900
      },
      "longitude": 37.20829,
      "name": "Tolstopaltsevo",
      "latitude": 55.611087
    },
    {
      "type": "Stop",
      "road_distances": {
        "Rasskazovka": 9900
      },
      "longitude": 37.209755,
      "name": "Marushkino",
      "latitude": 55.595884
    },
    {
      "type": "Bus",
      "name": "256",
      "stops": [
        "Biryulyovo Zapadnoye",
        "Biryusinka",
        "Universam",
        "Biryulyovo Tovarnaya",
        "Biryulyovo Passazhirskaya",
        "Biryulyovo Zapadnoye"
      ],
      "is_roundtrip": true
    },
    {
      "type": "Bus",
      "name": "750",
      "stops": [
        "Tolstopaltsevo",
        "Marushkino",
        "Rasskazovka"
      ],
      "is_roundtrip": false
    },
    {
      "type": "Stop",
      "road_distances": {},
      "longitude": 37.333324,
      "name": "Rasskazovka",
      "latitude": 55.632761
    },
    {
      "type": "Stop",
      "road_distances": {
        "Rossoshanskaya ulitsa": 7500,
        "Biryusinka": 1800,
        "Universam": 2400
      },
      "longitude": 37.6517,
      "name": "Biryulyovo Zapadnoye",
      "latitude": 55.574371
    },
    {
      "type": "Stop",
      "road_distances": {
        "Universam": 750
      },
      "longitude": 37.64839,
      "name": "Biryusinka",
      "latitude": 55.581065
    },
    {
      "type": "Stop",
      "road_distances": {
        "Rossoshanskaya ulitsa": 5600,
        "Biryulyovo Tovarnaya": 900
      },
      "longitude": 37.645687,
      "name": "Universam",
      "latitude": 55.587655
    },
    {
      "type": "Stop",
      "road_distances": {
        "Biryulyovo Passazhirskaya": 1300
      },
      "longitude": 37.653656,
      "name": "Biryulyovo Tovarnaya",
      "latitude": 55.592028
    },
    {
      "type": "Stop",
      "road_distances": {
        "Biryulyovo Zapadnoye": 1200
      },
      "longitude": 37.659164,
      "name": "Biryulyovo Passazhirskaya",
      "latitude": 55.580999
    },
    {
      "type": "Bus",
      "name": "828",
      "stops": [
        "Biryulyovo Zapadnoye",
        "Universam",
        "Rossoshanskaya ulitsa",
        "Biryulyovo Zapadnoye"
      ],
      "is_roundtrip": true
    },
    {
      "type": "Stop",
      "road_distances": {},
      "longitude": 37.605757,
      "name": "Rossoshanskaya ulitsa",
      "latitude": 55.595579
    },
    {
      "type": "Stop",
      "road_distances": {},
      "longitude": 37.603831,
      "name": "Prazhskaya",
      "latitude": 55.611678
    }
  ],
  "stat_requests": [
    {
      "type": "Bus",
      "name": "256",
      "id": 1965312327
    },
    {
      "type": "Bus",
      "name": "750",
      "id": 519139350
    },
    {
      "type": "Bus",
      "name": "751",
      "id": 194217464
    },
    {
      "type": "Stop",
      "name": "Samara",
      "id": 746888088
    },
    {
      "type": "Stop",
      "name": "Prazhskaya",
      "id": 65100610
    },
    {
      "type": "Stop",
      "name": "Biryulyovo Zapadnoye",
      "id": 1042838872
    }
  ]
}
)");
    using namespace Json;
    Document doc = Load(input);
    const map<string, Node> &root = doc.GetRoot().AsMap();
    ASSERT_EQUAL(root.size(), 2U);

    const vector<Node> &base_requests = root.at("base_requests"s).AsArray();
    ASSERT_EQUAL(base_requests.size(), 13U);

    size_t node_num = 0U;
    {
        /*
        {
            "type": "Stop",
            "road_distances": {
                "Marushkino": 3900
            },
            "longitude": 37.20829,
            "name": "Tolstopaltsevo",
            "latitude": 55.611087
        },
        */
        const map<string, Node> &req = base_requests.at(node_num++).AsMap();
        ASSERT_EQUAL( req.at("type"s).AsString(), "Stop"s );
        const map<string, Node> &road_distances = req.at("road_distances"s).AsMap();
        ASSERT_EQUAL( road_distances.size(), 1U );
        ASSERT_EQUAL( road_distances.at("Marushkino"s).AsInt(), 3900 );
        ASSERT( AssertDouble(req.at("longitude"s).AsDouble(), 37.20829) );
        ASSERT( AssertDouble(req.at("latitude"s).AsDouble(), 55.611087) );
        ASSERT_EQUAL( req.at("name"s).AsString(), "Tolstopaltsevo"s );
    }
    {
        /*
        {
            "type": "Stop",
            "road_distances": {
                "Rasskazovka": 9900
            },
            "longitude": 37.209755,
            "name": "Marushkino",
            "latitude": 55.595884
        },
        */
        const map<string, Node> &req = base_requests.at(node_num++).AsMap();
        ASSERT_EQUAL( req.at("type"s).AsString(), "Stop"s );
        const map<string, Node> &road_distances = req.at("road_distances"s).AsMap();
        ASSERT_EQUAL( road_distances.size(), 1U );
        ASSERT_EQUAL( road_distances.at("Rasskazovka"s).AsInt(), 9900 );
        ASSERT( AssertDouble(req.at("longitude"s).AsDouble(), 37.209755) );
        ASSERT( AssertDouble(req.at("latitude"s).AsDouble(), 55.595884) );
        ASSERT_EQUAL( req.at("name"s).AsString(), "Marushkino"s );
    }
    {
        /*
        {
            "type": "Bus",
            "name": "256",
            "stops": [
                "Biryulyovo Zapadnoye",
                "Biryusinka",
                "Universam",
                "Biryulyovo Tovarnaya",
                "Biryulyovo Passazhirskaya",
                "Biryulyovo Zapadnoye"
            ],
            "is_roundtrip": true
        },
        */
        const map<string, Node> &req = base_requests.at(node_num++).AsMap();
        ASSERT_EQUAL( req.at("type"s).AsString(), "Bus"s );
        ASSERT_EQUAL( req.at("name"s).AsString(), "256"s );
        const vector<Node> &stops = req.at("stops"s).AsArray();
        ASSERT_EQUAL( stops.at(0).AsString(), "Biryulyovo Zapadnoye"s );
        ASSERT_EQUAL( stops.at(1).AsString(), "Biryusinka"s );
        ASSERT_EQUAL( stops.at(2).AsString(), "Universam"s );
        ASSERT_EQUAL( stops.at(3).AsString(), "Biryulyovo Tovarnaya"s );
        ASSERT_EQUAL( stops.at(4).AsString(), "Biryulyovo Passazhirskaya"s );
        ASSERT_EQUAL( stops.at(5).AsString(), "Biryulyovo Zapadnoye"s );
        ASSERT_EQUAL( req.at("is_roundtrip"s).AsBool(), true );
    }
    {
        /*
        {
            "type": "Bus",
            "name": "750",
            "stops": [
                "Tolstopaltsevo",
                "Marushkino",
                "Rasskazovka"
            ],
            "is_roundtrip": false
        }
        */
        const map<string, Node> &req = base_requests.at(node_num++).AsMap();
        ASSERT_EQUAL( req.at("type"s).AsString(), "Bus"s );
        ASSERT_EQUAL( req.at("name"s).AsString(), "750"s );
        const vector<Node> &stops = req.at("stops"s).AsArray();
        ASSERT_EQUAL( stops.at(0).AsString(), "Tolstopaltsevo"s );
        ASSERT_EQUAL( stops.at(1).AsString(), "Marushkino"s );
        ASSERT_EQUAL( stops.at(2).AsString(), "Rasskazovka"s );
        ASSERT_EQUAL( req.at("is_roundtrip"s).AsBool(), false );
    }
    {
        /*
        {
            "type": "Stop",
            "road_distances": {},
            "longitude": 37.333324,
            "name": "Rasskazovka",
            "latitude": 55.632761
        }
        */
        const map<string, Node> &req = base_requests.at(node_num++).AsMap();
        ASSERT_EQUAL( req.at("type"s).AsString(), "Stop"s );
        const map<string, Node> &road_distances = req.at("road_distances"s).AsMap();
        ASSERT_EQUAL( road_distances.size(), 0U );
        ASSERT( AssertDouble(req.at("longitude"s).AsDouble(), 37.333324) );
        ASSERT( AssertDouble(req.at("latitude"s).AsDouble(), 55.632761) );
        ASSERT_EQUAL( req.at("name"s).AsString(), "Rasskazovka"s );
    }
    {
        /*
        {
            "type": "Stop",
            "road_distances": {
                "Rossoshanskaya ulitsa": 7500,
                "Biryusinka": 1800,
                "Universam": 2400
            },
            "longitude": 37.6517,
            "name": "Biryulyovo Zapadnoye",
            "latitude": 55.574371
        }
        */
        const map<string, Node> &req = base_requests.at(node_num++).AsMap();
        ASSERT_EQUAL( req.at("type"s).AsString(), "Stop"s );
        const map<string, Node> &road_distances = req.at("road_distances"s).AsMap();
        ASSERT_EQUAL( road_distances.size(), 3U );
        ASSERT_EQUAL( road_distances.at("Rossoshanskaya ulitsa"s).AsInt(), 7500 );
        ASSERT_EQUAL( road_distances.at("Biryusinka"s).AsInt(), 1800 );
        ASSERT_EQUAL( road_distances.at("Universam"s).AsInt(), 2400 );
        ASSERT( AssertDouble(req.at("longitude"s).AsDouble(), 37.6517) );
        ASSERT( AssertDouble(req.at("latitude"s).AsDouble(), 55.574371) );
        ASSERT_EQUAL( req.at("name"s).AsString(), "Biryulyovo Zapadnoye"s );
    }
    {
        /*
        {
            "type": "Stop",
            "road_distances": {
                "Universam": 750
            },
            "longitude": 37.64839,
            "name": "Biryusinka",
            "latitude": 55.581065
        }
        */
        const map<string, Node> &req = base_requests.at(node_num++).AsMap();
        ASSERT_EQUAL( req.at("type"s).AsString(), "Stop"s );
        const map<string, Node> &road_distances = req.at("road_distances"s).AsMap();
        ASSERT_EQUAL( road_distances.size(), 1U );
        ASSERT_EQUAL( road_distances.at("Universam"s).AsInt(), 750 );
        ASSERT( AssertDouble(req.at("longitude"s).AsDouble(), 37.64839) );
        ASSERT( AssertDouble(req.at("latitude"s).AsDouble(), 55.581065) );
        ASSERT_EQUAL( req.at("name"s).AsString(), "Biryusinka"s );
    }
    {
        /*
        {
            "type": "Stop",
            "road_distances": {
                "Rossoshanskaya ulitsa": 5600,
                "Biryulyovo Tovarnaya": 900
            },
            "longitude": 37.645687,
            "name": "Universam",
            "latitude": 55.587655
        }
        */
        const map<string, Node> &req = base_requests.at(node_num++).AsMap();
        ASSERT_EQUAL( req.at("type"s).AsString(), "Stop"s );
        const map<string, Node> &road_distances = req.at("road_distances"s).AsMap();
        ASSERT_EQUAL( road_distances.size(), 2U );
        ASSERT_EQUAL( road_distances.at("Rossoshanskaya ulitsa"s).AsInt(), 5600 );
        ASSERT_EQUAL( road_distances.at("Biryulyovo Tovarnaya"s).AsInt(), 900 );
        ASSERT( AssertDouble(req.at("longitude"s).AsDouble(), 37.645687) );
        ASSERT( AssertDouble(req.at("latitude"s).AsDouble(), 55.587655) );
        ASSERT_EQUAL( req.at("name"s).AsString(), "Universam"s );
    }
    {
        /*
        {
            "type": "Stop",
            "road_distances": {
                "Biryulyovo Passazhirskaya": 1300
            },
            "longitude": 37.653656,
            "name": "Biryulyovo Tovarnaya",
            "latitude": 55.592028
        }
        */
        const map<string, Node> &req = base_requests.at(node_num++).AsMap();
        ASSERT_EQUAL( req.at("type"s).AsString(), "Stop"s );
        const map<string, Node> &road_distances = req.at("road_distances"s).AsMap();
        ASSERT_EQUAL( road_distances.size(), 1U );
        ASSERT_EQUAL( road_distances.at("Biryulyovo Passazhirskaya"s).AsInt(), 1300 );
        ASSERT( AssertDouble(req.at("longitude"s).AsDouble(), 37.653656) );
        ASSERT( AssertDouble(req.at("latitude"s).AsDouble(), 55.592028) );
        ASSERT_EQUAL( req.at("name"s).AsString(), "Biryulyovo Tovarnaya"s );
    }
    {
        /*
        {
            "type": "Stop",
            "road_distances": {
                "Biryulyovo Zapadnoye": 1200
            },
            "longitude": 37.659164,
            "name": "Biryulyovo Passazhirskaya",
            "latitude": 55.580999
        }
        */
        const map<string, Node> &req = base_requests.at(node_num++).AsMap();
        ASSERT_EQUAL( req.at("type"s).AsString(), "Stop"s );
        const map<string, Node> &road_distances = req.at("road_distances"s).AsMap();
        ASSERT_EQUAL( road_distances.size(), 1U );
        ASSERT_EQUAL( road_distances.at("Biryulyovo Zapadnoye"s).AsInt(), 1200 );
        ASSERT( AssertDouble(req.at("longitude"s).AsDouble(), 37.659164) );
        ASSERT( AssertDouble(req.at("latitude"s).AsDouble(), 55.580999) );
        ASSERT_EQUAL( req.at("name"s).AsString(), "Biryulyovo Passazhirskaya"s );
    }
    {
        /*
        {
            "type": "Bus",
            "name": "828",
            "stops": [
                "Biryulyovo Zapadnoye",
                "Universam",
                "Rossoshanskaya ulitsa",
                "Biryulyovo Zapadnoye"
            ],
            "is_roundtrip": true
        }
        */
        const map<string, Node> &req = base_requests.at(node_num++).AsMap();
        ASSERT_EQUAL( req.at("type"s).AsString(), "Bus"s );
        ASSERT_EQUAL( req.at("name"s).AsString(), "828"s );
        const vector<Node> &stops = req.at("stops"s).AsArray();
        ASSERT_EQUAL( stops.size(), 4U );
        ASSERT_EQUAL( stops.at(0).AsString(), "Biryulyovo Zapadnoye"s );
        ASSERT_EQUAL( stops.at(1).AsString(), "Universam"s );
        ASSERT_EQUAL( stops.at(2).AsString(), "Rossoshanskaya ulitsa"s );
        ASSERT_EQUAL( stops.at(3).AsString(), "Biryulyovo Zapadnoye"s );
        ASSERT_EQUAL( req.at("is_roundtrip"s).AsBool(), true );
    }
    {
        /*
        {
            "type": "Stop",
            "road_distances": {},
            "longitude": 37.605757,
            "name": "Rossoshanskaya ulitsa",
            "latitude": 55.595579
        }
        */
        const map<string, Node> &req = base_requests.at(node_num++).AsMap();
        ASSERT_EQUAL( req.at("type"s).AsString(), "Stop"s );
        const map<string, Node> &road_distances = req.at("road_distances"s).AsMap();
        ASSERT_EQUAL( road_distances.size(), 0U );
        ASSERT( AssertDouble(req.at("longitude"s).AsDouble(), 37.605757) );
        ASSERT( AssertDouble(req.at("latitude"s).AsDouble(), 55.595579) );
        ASSERT_EQUAL( req.at("name"s).AsString(), "Rossoshanskaya ulitsa"s );
    }
    {
        /*
        {
            "type": "Stop",
            "road_distances": {},
            "longitude": 37.603831,
            "name": "Prazhskaya",
            "latitude": 55.611678
        }
        */
        const map<string, Node> &req = base_requests.at(node_num++).AsMap();
        ASSERT_EQUAL( req.at("type"s).AsString(), "Stop"s );
        const map<string, Node> &road_distances = req.at("road_distances"s).AsMap();
        ASSERT_EQUAL( road_distances.size(), 0U );
        ASSERT( AssertDouble(req.at("longitude"s).AsDouble(), 37.603831) );
        ASSERT( AssertDouble(req.at("latitude"s).AsDouble(), 55.611678) );
        ASSERT_EQUAL( req.at("name"s).AsString(), "Prazhskaya"s );
    }

    node_num = 0U;
    const vector<Node> &stat_requests = root.at("stat_requests"s).AsArray();

    {
        /*
        {
            "type": "Bus",
            "name": "256",
            "id": 1965312327
        }
        */
        const map<string, Node> &req = stat_requests.at(node_num++).AsMap();
        ASSERT_EQUAL( req.at("type"s).AsString(), "Bus"s );
        ASSERT_EQUAL( req.at("name"s).AsString(), "256"s );
        ASSERT_EQUAL( req.at("id"s).AsInt(), 1965312327 );
    }
    {
        /*
        {
            "type": "Bus",
            "name": "750",
            "id": 519139350
        },
        */
        const map<string, Node> &req = stat_requests.at(node_num++).AsMap();
        ASSERT_EQUAL( req.at("type"s).AsString(), "Bus"s );
        ASSERT_EQUAL( req.at("name"s).AsString(), "750"s );
        ASSERT_EQUAL( req.at("id"s).AsInt(), 519139350 );
    }
    {
        /*
        {
            "type": "Bus",
            "name": "751",
            "id": 194217464
        }
        */
        const map<string, Node> &req = stat_requests.at(node_num++).AsMap();
        ASSERT_EQUAL( req.at("type"s).AsString(), "Bus"s );
        ASSERT_EQUAL( req.at("name"s).AsString(), "751"s );
        ASSERT_EQUAL( req.at("id"s).AsInt(), 194217464 );
    }
    {
        /*
        {
            "type": "Stop",
            "name": "Samara",
            "id": 746888088
        }
        */
        const map<string, Node> &req = stat_requests.at(node_num++).AsMap();
        ASSERT_EQUAL( req.at("type"s).AsString(), "Stop"s );
        ASSERT_EQUAL( req.at("name"s).AsString(), "Samara"s );
        ASSERT_EQUAL( req.at("id"s).AsInt(), 746888088 );
    }
    {
        /*
        {
            "type": "Stop",
            "name": "Prazhskaya",
            "id": 65100610
        }
        */
        const map<string, Node> &req = stat_requests.at(node_num++).AsMap();
        ASSERT_EQUAL( req.at("type"s).AsString(), "Stop"s );
        ASSERT_EQUAL( req.at("name"s).AsString(), "Prazhskaya"s );
        ASSERT_EQUAL( req.at("id"s).AsInt(), 65100610 );
    }
    {
        /*
        {
            "type": "Stop",
            "name": "Biryulyovo Zapadnoye",
            "id": 1042838872
        }
        */
        const map<string, Node> &req = stat_requests.at(node_num++).AsMap();
        ASSERT_EQUAL( req.at("type"s).AsString(), "Stop"s );
        ASSERT_EQUAL( req.at("name"s).AsString(), "Biryulyovo Zapadnoye"s );
        ASSERT_EQUAL( req.at("id"s).AsInt(), 1042838872 );
    }
}

void TestParse()
{
    {
        istringstream iss(R"({"routing_settings": {"bus_wait_time": 6, "bus_velocity": 40},
  "base_requests": [
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.611087,
      "longitude": 37.20829,
      "name": "Tolstopaltsevo"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.595884,
      "longitude": 37.209755,
      "name": "Marushkino"
    },
    {
      "type": "Bus",
      "name": "256",
      "stops": [
        "Biryulyovo Zapadnoye",
        "Biryusinka",
        "Universam",
        "Biryulyovo Tovarnaya",
        "Biryulyovo Passazhirskaya",
        "Biryulyovo Zapadnoye"
      ],
      "is_roundtrip": true
    },
    {
      "type": "Bus",
      "name": "750",
      "stops": [
        "Tolstopaltsevo",
        "Marushkino",
        "Rasskazovka"
      ],
      "is_roundtrip": false
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.632761,
      "longitude": 37.333324,
      "name": "Rasskazovka"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.574371,
      "longitude": 37.6517,
      "name": "Biryulyovo Zapadnoye"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.581065,
      "longitude": 37.64839,
      "name": "Biryusinka"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.587655,
      "longitude": 37.645687,
      "name": "Universam"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.592028,
      "longitude": 37.653656,
      "name": "Biryulyovo Tovarnaya"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.580999,
      "longitude": 37.659164,
      "name": "Biryulyovo Passazhirskaya"
    }
  ],
  "stat_requests": [
    {
      "type": "Bus",
      "name": "256",
      "id": 0
    },
    {
      "type": "Bus",
      "name": "750",
      "id": 1
    },
    {
      "type": "Bus",
      "name": "751",
      "id": 2
    }
  ]
}
)");

        ostringstream oss;

        Parse(iss, oss);

        istringstream iss_expect(R"([
  {
    "request_id": 0,
    "stop_count": 6,
    "unique_stop_count": 5,
    "route_length": 0,
    "curvature": 0
  },
  {
    "request_id": 1,
    "stop_count": 5,
    "unique_stop_count": 3,
    "route_length": 0,
    "curvature": 0
  },
  {
    "request_id": 2,
    "error_message": "not found"
  }
])");

        string str = oss.str();
        string str_expect = iss_expect.str();

        ASSERT_EQUAL(str, str_expect);
    }
    {
        istringstream iss(R"({"routing_settings": {"bus_wait_time": 6, "bus_velocity": 40},
  "base_requests": [
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.611087,
      "longitude": 37.20829,
      "name": "Tolstopaltsevo"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.595884,
      "longitude": 37.209755,
      "name": "Marushkino"
    },
    {
      "type": "Bus",
      "name": "Malina Kalina",
      "stops": [
        "Biryulyovo Zapadnoye",
        "Biryusinka",
        "Universam",
        "Biryulyovo Tovarnaya",
        "Biryulyovo Passazhirskaya",
        "Biryulyovo Zapadnoye"
      ],
      "is_roundtrip": true
    },
    {
      "type": "Bus",
      "name": "750",
      "stops": [
        "Tolstopaltsevo",
        "Marushkino",
        "Rasskazovka"
      ],
      "is_roundtrip": false
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.632761,
      "longitude": 37.333324,
      "name": "Rasskazovka"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.571231,
      "longitude": 37.6517,
      "name": "Biryulyovo Zapadnoye"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.581065,
      "longitude": 37.64839,
      "name": "Biryusinka"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.587655,
      "longitude": 37.645687,
      "name": "Universam"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.592028,
      "longitude": 37.653656,
      "name": "Biryulyovo Tovarnaya"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.580999,
      "longitude": 37.659164,
      "name": "Biryulyovo Passazhirskaya"
    },
    {
      "type": "Bus",
      "name": "257",
      "stops": [
        "Biryulyovo Zapadnoye",
        "Biryusinka",
        "Universam",
        "Biryulyovo Tovarnaya",
        "Universam",
        "Biryusinka",
        "Biryulyovo Zapadnoye"
      ],
      "is_roundtrip": true
    },
    {
      "type": "Bus",
      "name": "258",
      "stops": [
        "Biryulyovo Zapadnoye",
        "Biryusinka",
        "Universam",
        "Biryulyovo Tovarnaya"
      ],
      "is_roundtrip": false
    }
  ],
  "stat_requests": [
    {
      "type": "Bus",
      "name": "Malina Kalina",
      "id": 1965312320
    },
    {
      "type": "Bus",
      "name": "750",
      "id": 1965312321
    },
    {
      "type": "Bus",
      "name": "751",
      "id": 1965312322
    },
    {
      "type": "Bus",
      "name": "257",
      "id": 1965312323
    },
    {
      "type": "Bus",
      "name": "258",
      "id": 1965312324
    }
  ]
}
)");

        ostringstream oss;

        Parse(iss, oss);

        istringstream iss_expect(R"([
  {
    "request_id": 1965312320,
    "stop_count": 6,
    "unique_stop_count": 5,
    "route_length": 0,
    "curvature": 0
  },
  {
    "request_id": 1965312321,
    "stop_count": 5,
    "unique_stop_count": 3,
    "route_length": 0,
    "curvature": 0
  },
  {
    "request_id": 1965312322,
    "error_message": "not found"
  },
  {
    "request_id": 1965312323,
    "stop_count": 7,
    "unique_stop_count": 4,
    "route_length": 0,
    "curvature": 0
  },
  {
    "request_id": 1965312324,
    "stop_count": 7,
    "unique_stop_count": 4,
    "route_length": 0,
    "curvature": 0
  }
])");

        string str = oss.str();
        string str_expect = iss_expect.str();

        ASSERT_EQUAL(str, str_expect);
    }
    {
        istringstream iss(R"({"routing_settings": {"bus_wait_time": 6, "bus_velocity": 40},
  "base_requests": [
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.611087,
      "longitude": 37.20829,
      "name": "Tolstopaltsevo"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 56.60,
      "longitude": 50.35,
      "name": "Marushkino"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.611087,
      "longitude": 37.20829,
      "name": "Parushka"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 56.60,
      "longitude": 50.35,
      "name": "Sasushka"
    },
    {
      "type": "Bus",
      "name": "256",
      "stops": [
        "Tolstopaltsevo",
        "Marushkino",
        "Tolstopaltsevo"
      ],
      "is_roundtrip": true
    },
    {
      "type": "Bus",
      "name": "750",
      "stops": [
        "Tolstopaltsevo",
        "Marushkino"
      ],
      "is_roundtrip": false
    },
    {
      "type": "Bus",
      "name": "755",
      "stops": [
        "Parushka",
        "Sasushka"
      ],
      "is_roundtrip": false
    },
    {
      "type": "Bus",
      "name": "756",
      "stops": [
        "Parushka",
        "Sasushka"
      ],
      "is_roundtrip": false
    },
    {
      "type": "Bus",
      "name": "757",
      "stops": [
        "Parushka",
        "Sasushka",
        "Tolstopaltsevo",
        "Marushkino"
      ],
      "is_roundtrip": false
    },
    {
      "type": "Bus",
      "name": "758",
      "stops": [
        "Parushka",
        "Sasushka",
        "Marushkino",
        "Tolstopaltsevo"
      ],
      "is_roundtrip": false
    }
  ],
  "stat_requests": [
    {
      "type": "Bus",
      "name": "256",
      "id": 0
    },
    {
      "type": "Bus",
      "name": "750",
      "id": 1
    },
    {
      "type": "Bus",
      "name": "256",
      "id": 2
    },
    {
      "type": "Bus",
      "name": "751",
      "id": 3
    },
    {
      "type": "Bus",
      "name": "755",
      "id": 4
    },
    {
      "type": "Bus",
      "name": "756",
      "id": 5
    },
    {
      "type": "Bus",
      "name": "757",
      "id": 6
    },
    {
      "type": "Bus",
      "name": "758",
      "id": 7
    }
  ]
}
)");

        ostringstream oss;

        Parse(iss, oss);

        istringstream iss_expect(R"([
  {
    "request_id": 0,
    "stop_count": 3,
    "unique_stop_count": 2,
    "route_length": 0,
    "curvature": 0
  },
  {
    "request_id": 1,
    "stop_count": 3,
    "unique_stop_count": 2,
    "route_length": 0,
    "curvature": 0
  },
  {
    "request_id": 2,
    "stop_count": 3,
    "unique_stop_count": 2,
    "route_length": 0,
    "curvature": 0
  },
  {
    "request_id": 3,
    "error_message": "not found"
  },
  {
    "request_id": 4,
    "stop_count": 3,
    "unique_stop_count": 2,
    "route_length": 0,
    "curvature": 0
  },
  {
    "request_id": 5,
    "stop_count": 3,
    "unique_stop_count": 2,
    "route_length": 0,
    "curvature": 0
  },
  {
    "request_id": 6,
    "stop_count": 7,
    "unique_stop_count": 4,
    "route_length": 0,
    "curvature": 0
  },
  {
    "request_id": 7,
    "stop_count": 7,
    "unique_stop_count": 4,
    "route_length": 0,
    "curvature": 0
  }
])");

        string str = oss.str();
        string str_expect = iss_expect.str();

        ASSERT_EQUAL(str, str_expect);
    }
    {
        istringstream iss(R"({"routing_settings": {"bus_wait_time": 6, "bus_velocity": 40},
  "base_requests": [
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 56.60,
      "longitude": 50.31,
      "name": "X1"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 56.60,
      "longitude": 50.32,
      "name": "X2"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 56.60,
      "longitude": 50.33,
      "name": "X3"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 56.60,
      "longitude": 50.34,
      "name": "X4"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 56.60,
      "longitude": 50.35,
      "name": "X5"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 56.60,
      "longitude": 50.36,
      "name": "X6"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 56.60,
      "longitude": 50.37,
      "name": "X7"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 56.60,
      "longitude": 50.38,
      "name": "X8"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 56.60,
      "longitude": 50.39,
      "name": "X9"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 56.60,
      "longitude": 50.40,
      "name": "X10 + X1 + X5 + X9"
    },
    {
      "type": "Bus",
      "name": "X",
      "stops": [
        "X1",
        "X2",
        "X3",
        "X4",
        "X5",
        "X6",
        "X7",
        "X8",
        "X9",
        "X10 + X1 + X5 + X9"
      ],
      "is_roundtrip": false
    }
  ],
  "stat_requests": [
    {
      "type": "Bus",
      "name": "X",
      "id": 0
    },
    {
      "type": "Bus",
      "name": "X",
      "id": 1
    }
  ]
}
)");

        ostringstream oss;

        Parse(iss, oss);

        istringstream iss_expect(R"([
  {
    "request_id": 0,
    "stop_count": 19,
    "unique_stop_count": 10,
    "route_length": 0,
    "curvature": 0
  },
  {
    "request_id": 1,
    "stop_count": 19,
    "unique_stop_count": 10,
    "route_length": 0,
    "curvature": 0
  }
])");

        string str = oss.str();
        string str_expect = iss_expect.str();

        ASSERT_EQUAL(str, str_expect);
    }
    {
        istringstream iss(R"(
{ "routing_settings": {"bus_wait_time": 6, "bus_velocity": 40},
  "base_requests": [
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.611087,
      "longitude": 37.20829,
      "name": "Tolstopaltsevo"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.595884,
      "longitude": 37.209755,
      "name": "Marushkino"
    },
    {
      "type": "Bus",
      "name": "256",
      "stops": [
        "Biryulyovo Zapadnoye",
        "Biryusinka",
        "Universam",
        "Biryulyovo Tovarnaya",
        "Biryulyovo Passazhirskaya",
        "Biryulyovo Zapadnoye"
      ],
      "is_roundtrip": true
    },
    {
      "type": "Bus",
      "name": "750",
      "stops": [
        "Tolstopaltsevo",
        "Marushkino",
        "Rasskazovka"
      ],
      "is_roundtrip": false
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.632761,
      "longitude": 37.333324,
      "name": "Rasskazovka"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.574371,
      "longitude": 37.6517,
      "name": "Biryulyovo Zapadnoye"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.581065,
      "longitude": 37.64839,
      "name": "Biryusinka"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.587655,
      "longitude": 37.645687,
      "name": "Universam"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.592028,
      "longitude": 37.653656,
      "name": "Biryulyovo Tovarnaya"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.580999,
      "longitude": 37.659164,
      "name": "Biryulyovo Passazhirskaya"
    },
    {
      "type": "Bus",
      "name": "828",
      "stops": [
        "Biryulyovo Zapadnoye",
        "Universam",
        "Rossoshanskaya ulitsa",
        "Biryulyovo Zapadnoye"
      ],
      "is_roundtrip": true
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.595579,
      "longitude": 37.605757,
      "name": "Rossoshanskaya ulitsa"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.611678,
      "longitude": 37.603831,
      "name": "Prazhskaya"
    }
  ],
  "stat_requests": [
    {
      "type": "Bus",
      "name": "256",
      "id": 0
    },
    {
      "type": "Bus",
      "name": "750",
      "id": 1
    },
    {
      "type": "Bus",
      "name": "751",
      "id": 2
    },
    {
      "type": "Stop",
      "name": "Samara",
      "id": 3
    },
    {
      "type": "Stop",
      "name": "Prazhskaya",
      "id": 4
    },
    {
      "type": "Stop",
      "name": "Biryulyovo Zapadnoye",
      "id": 5
    }
  ]
}
)");

        ostringstream oss;

        Parse(iss, oss);

        istringstream iss_expect(R"([
  {
    "request_id": 0,
    "stop_count": 6,
    "unique_stop_count": 5,
    "route_length": 0,
    "curvature": 0
  },
  {
    "request_id": 1,
    "stop_count": 5,
    "unique_stop_count": 3,
    "route_length": 0,
    "curvature": 0
  },
  {
    "request_id": 2,
    "error_message": "not found"
  },
  {
    "request_id": 3,
    "error_message": "not found"
  },
  {
    "request_id": 4,
    "buses": []
  },
  {
    "request_id": 5,
    "buses": [
      "256",
      "828"
    ]
  }
])");

        string str = oss.str();
        string str_expect = iss_expect.str();

        ASSERT_EQUAL(str, str_expect);
    }
    {
        istringstream iss(R"({"routing_settings": {"bus_wait_time": 6, "bus_velocity": 40},
  "base_requests": [
    {
      "type": "Stop",
      "road_distances": {
        "Marushkino": 3900
      },
      "latitude": 55.611087,
      "longitude": 37.20829,
      "name": "Tolstopaltsevo"
    },
    {
      "type": "Stop",
      "road_distances": {
        "Rasskazovka": 9900
      },
      "latitude": 55.595884,
      "longitude": 37.209755,
      "name": "Marushkino"
    },
    {
      "type": "Bus",
      "name": "256",
      "stops": [
        "Biryulyovo Zapadnoye",
        "Biryusinka",
        "Universam",
        "Biryulyovo Tovarnaya",
        "Biryulyovo Passazhirskaya",
        "Biryulyovo Zapadnoye"
      ],
      "is_roundtrip": true
    },
    {
      "type": "Bus",
      "name": "750",
      "stops": [
        "Tolstopaltsevo",
        "Marushkino",
        "Rasskazovka"
      ],
      "is_roundtrip": false
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.632761,
      "longitude": 37.333324,
      "name": "Rasskazovka"
    },
    {
      "type": "Stop",
      "road_distances": {
        "Rossoshanskaya ulitsa": 7500,
        "Biryusinka": 1800,
        "Universam": 2400
      },
      "latitude": 55.574371,
      "longitude": 37.6517,
      "name": "Biryulyovo Zapadnoye"
    },
    {
      "type": "Stop",
      "road_distances": {
        "Universam": 750
      },
      "latitude": 55.581065,
      "longitude": 37.64839,
      "name": "Biryusinka"
    },
    {
      "type": "Stop",
      "road_distances": {
        "Rossoshanskaya ulitsa": 5600,
        "Biryulyovo Tovarnaya": 900
      },
      "latitude": 55.587655,
      "longitude": 37.645687,
      "name": "Universam"
    },
    {
      "type": "Stop",
      "road_distances": {
        "Biryulyovo Passazhirskaya": 1300
      },
      "latitude": 55.592028,
      "longitude": 37.653656,
      "name": "Biryulyovo Tovarnaya"
    },
    {
      "type": "Stop",
      "road_distances": {
        "Biryulyovo Zapadnoye": 1200
      },
      "latitude": 55.580999,
      "longitude": 37.659164,
      "name": "Biryulyovo Passazhirskaya"
    },
    {
      "type": "Bus",
      "name": "828",
      "stops": [
        "Biryulyovo Zapadnoye",
        "Universam",
        "Rossoshanskaya ulitsa",
        "Biryulyovo Zapadnoye"
      ],
      "is_roundtrip": true
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.595579,
      "longitude": 37.605757,
      "name": "Rossoshanskaya ulitsa"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.611678,
      "longitude": 37.603831,
      "name": "Prazhskaya"
    }
  ],
  "stat_requests": [
    {
      "type": "Bus",
      "name": "256",
      "id": 0
    },
    {
      "type": "Bus",
      "name": "750",
      "id": 1
    },
    {
      "type": "Bus",
      "name": "751",
      "id": 2
    },
    {
      "type": "Stop",
      "name": "Samara",
      "id": 3
    },
    {
      "type": "Stop",
      "name": "Prazhskaya",
      "id": 4
    },
    {
      "type": "Stop",
      "name": "Biryulyovo Zapadnoye",
      "id": 5
    }
  ]
}
)");

        ostringstream oss;

        Parse(iss, oss);

        istringstream iss_expect(R"([
  {
    "request_id": 0,
    "stop_count": 6,
    "unique_stop_count": 5,
    "route_length": 5950,
    "curvature": 1.36124
  },
  {
    "request_id": 1,
    "stop_count": 5,
    "unique_stop_count": 3,
    "route_length": 27600,
    "curvature": 1.31808
  },
  {
    "request_id": 2,
    "error_message": "not found"
  },
  {
    "request_id": 3,
    "error_message": "not found"
  },
  {
    "request_id": 4,
    "buses": []
  },
  {
    "request_id": 5,
    "buses": [
      "256",
      "828"
    ]
  }
])");

        string str = oss.str();
        string str_expect = iss_expect.str();

        ASSERT_EQUAL(str, str_expect);
    }
    {
        istringstream iss(R"({"routing_settings": {"bus_wait_time": 6, "bus_velocity": 40}, "base_requests": [{"latitude": 55.611087, "longitude": 37.20829, "type": "Stop", "road_distances": {"+++": 3900}, "name": "Tolstopaltsevo"}, {"type": "Stop", "name": "+++", "latitude": 55.595884, "longitude": 37.209755, "road_distances": {"Rasskazovka": 9900}}, {"type": "Bus", "name": "256", "stops": ["Biryulyovo Zapadnoye", "Biryusinka", "Universam", "Biryulyovo Tovarnaya", "Biryulyovo Passazhirskaya", "Biryulyovo Zapadnoye"], "is_roundtrip": true}, {"type": "Bus", "name": "750", "is_roundtrip": false, "stops": ["Tolstopaltsevo", "Marushkino", "Rasskazovka"]}, {"type": "Stop", "name": "Rasskazovka", "latitude": 55.632761, "longitude": 37.333324, "road_distances": {}}, {"type": "Stop", "name": "Biryulyovo Zapadnoye", "latitude": 55.574371, "longitude": 37.6517, "road_distances": {"Biryusinka": 1800, "Universam": 2400, "Rossoshanskaya ulitsa": 7500}}, {"type": "Stop", "name": "Biryusinka", "latitude": 55.581065, "longitude": 37.64839, "road_distances": {"Universam": 750}}, {"type": "Stop", "name": "Universam", "latitude": 55.587655, "longitude": 37.645687, "road_distances": {"Biryulyovo Tovarnaya": 900, "Rossoshanskaya ulitsa": 5600}}, {"type": "Stop", "name": "Biryulyovo Tovarnaya", "latitude": 55.592028, "longitude": 37.653656, "road_distances": {"Biryulyovo Passazhirskaya": 1300}}, {"type": "Stop", "name": "Biryulyovo Passazhirskaya", "latitude": 55.580999, "longitude": 37.659164, "road_distances": {"Biryulyovo Zapadnoye": 1200}}, {"type": "Bus", "name": "828", "stops": ["Biryulyovo Zapadnoye", "Universam", "Rossoshanskaya ulitsa", "Biryulyovo Zapadnoye"], "is_roundtrip": true}, {"type": "Stop", "name": "Rossoshanskaya ulitsa", "latitude": 55.595579, "longitude": 37.605757, "road_distances": {}}, {"type": "Stop", "name": "Prazhskaya", "latitude": 55.611678, "longitude": 37.603831, "road_distances": {}}], "stat_requests": [{"id": 599708471, "type": "Bus", "name": "256"}, {"id": 2059890189, "type": "Bus", "name": "750"}, {"id": 222058974, "type": "Bus", "name": "751"}, {"id": 1038752326, "type": "Stop", "name": "Samara"}, {"id": 986197773, "type": "Stop", "name": "Prazhskaya"}, {"id": 932894250, "type": "Stop", "name": "Biryulyovo Zapadnoye"}]})");

        ostringstream oss;

        Parse(iss, oss);

        istringstream iss_expect(R"([
  {
    "request_id": 599708471,
    "stop_count": 6,
    "unique_stop_count": 5,
    "route_length": 5950,
    "curvature": 1.36124
  },
  {
    "request_id": 2059890189,
    "stop_count": 5,
    "unique_stop_count": 3,
    "route_length": 0,
    "curvature": 0
  },
  {
    "request_id": 222058974,
    "error_message": "not found"
  },
  {
    "request_id": 1038752326,
    "error_message": "not found"
  },
  {
    "request_id": 986197773,
    "buses": []
  },
  {
    "request_id": 932894250,
    "buses": [
      "256",
      "828"
    ]
  }
])");

        string str = oss.str();
        string str_expect = iss_expect.str();

        ASSERT_EQUAL(str, str_expect);
    }
    {
        istringstream iss(R"({"routing_settings": {"bus_wait_time": 6, "bus_velocity": 40}, "base_requests": [{"type": "Stop", "name": "A", "latitude": 0.5, "longitude": -1, "road_distances": {"B": 100000}}, {"type": "Stop", "name": "B", "latitude": 0, "longitude": -1.1, "road_distances": {}}, {"type": "Bus", "name": "256", "stops": ["B", "A"], "is_roundtrip": false}], "stat_requests": [{"id": 1317873202, "type": "Bus", "name": "256"}, {"id": 853807922, "type": "Stop", "name": "A"}, {"id": 1416389015, "type": "Stop", "name": "B"}, {"id": 1021316791, "type": "Stop", "name": "C"}]})");

        ostringstream oss;

        Parse(iss, oss);

        istringstream iss_expect(R"([
  {
    "request_id": 1317873202,
    "stop_count": 3,
    "unique_stop_count": 2,
    "route_length": 200000,
    "curvature": 1.76372
  },
  {
    "request_id": 853807922,
    "buses": [
      "256"
    ]
  },
  {
    "request_id": 1416389015,
    "buses": [
      "256"
    ]
  },
  {
    "request_id": 1021316791,
    "error_message": "not found"
  }
])");

        string str = oss.str();
        string str_expect = iss_expect.str();

        ASSERT_EQUAL(str, str_expect);
    }
    {
        istringstream iss(R"({
  "routing_settings": {
    "bus_wait_time": 6,
    "bus_velocity": 40
  },
  "base_requests": [
    {
      "type": "Bus",
      "name": "297",
      "stops": [
        "Biryulyovo Zapadnoye",
        "Biryulyovo Tovarnaya",
        "Universam",
        "Biryulyovo Zapadnoye"
      ],
      "is_roundtrip": true
    },
    {
      "type": "Bus",
      "name": "635",
      "stops": [
        "Biryulyovo Tovarnaya",
        "Universam",
        "Prazhskaya"
      ],
      "is_roundtrip": false
    },
    {
      "type": "Stop",
      "road_distances": {
        "Biryulyovo Tovarnaya": 2600
      },
      "longitude": 37.6517,
      "name": "Biryulyovo Zapadnoye",
      "latitude": 55.574371
    },
    {
      "type": "Stop",
      "road_distances": {
        "Prazhskaya": 4650,
        "Biryulyovo Tovarnaya": 1380,
        "Biryulyovo Zapadnoye": 2500
      },
      "longitude": 37.645687,
      "name": "Universam",
      "latitude": 55.587655
    },
    {
      "type": "Stop",
      "road_distances": {
        "Universam": 890
      },
      "longitude": 37.653656,
      "name": "Biryulyovo Tovarnaya",
      "latitude": 55.592028
    },
    {
      "type": "Stop",
      "road_distances": {},
      "longitude": 37.603938,
      "name": "Prazhskaya",
      "latitude": 55.611717
    }
  ],
  "stat_requests": [
    {
      "type": "Bus",
      "name": "297",
      "id": 1
    },
    {
      "type": "Bus",
      "name": "635",
      "id": 2
    },
    {
      "type": "Stop",
      "name": "Universam",
      "id": 3
    },
    {
      "type": "Route",
      "from": "Biryulyovo Zapadnoye",
      "to": "Universam",
      "id": 4
    },
    {
      "type": "Route",
      "from": "Biryulyovo Zapadnoye",
      "to": "Prazhskaya",
      "id": 5
    }
  ]
})");

        ostringstream oss;

        Parse(iss, oss);

        istringstream iss_expect(R"([
  {
    "request_id": 1,
    "stop_count": 4,
    "unique_stop_count": 3,
    "route_length": 5990,
    "curvature": 1.42963
  },
  {
    "request_id": 2,
    "stop_count": 5,
    "unique_stop_count": 3,
    "route_length": 11570,
    "curvature": 1.30156
  },
  {
    "request_id": 3,
    "buses": [
      "297",
      "635"
    ]
  },
  {
    "request_id": 4,
    "total_time": 11.235,
    "items": [
        {
            "time": 6,
            "type": "Wait",
            "stop_name": "Biryulyovo Zapadnoye"
        },
        {
            "span_count": 2,
            "bus": "297",
            "type": "Bus",
            "time": 5.235
        }
    ]
  },
  {
    "request_id": 5,
    "total_time": 24.21,
    "items": [
        {
            "time": 6,
            "type": "Wait",
            "stop_name": "Biryulyovo Zapadnoye"
        },
        {
            "span_count": 2,
            "bus": "297",
            "type": "Bus",
            "time": 5.235
        },
        {
            "time": 6,
            "type": "Wait",
            "stop_name": "Universam"
        },
        {
            "span_count": 1,
            "bus": "635",
            "type": "Bus",
            "time": 6.975
        }
    ]
  }
])");

        string str = oss.str();
        string str_expect = iss_expect.str();

        ASSERT_EQUAL(str, str_expect);
    }
    {
        istringstream iss(R"({
  "routing_settings": {
    "bus_wait_time": 2,
    "bus_velocity": 30
  },
  "base_requests": [
    {
      "type": "Bus",
      "name": "297",
      "stops": [
        "Biryulyovo Zapadnoye",
        "Biryulyovo Tovarnaya",
        "Universam",
        "Biryusinka",
        "Apteka",
        "Biryulyovo Zapadnoye"
      ],
      "is_roundtrip": true
    },
    {
      "type": "Bus",
      "name": "635",
      "stops": [
        "Biryulyovo Tovarnaya",
        "Universam",
        "Biryusinka",
        "TETs 26",
        "Pokrovskaya",
        "Prazhskaya"
      ],
      "is_roundtrip": false
    },
    {
      "type": "Bus",
      "name": "828",
      "stops": [
        "Biryulyovo Zapadnoye",
        "TETs 26",
        "Biryusinka",
        "Universam",
        "Pokrovskaya",
        "Rossoshanskaya ulitsa"
      ],
      "is_roundtrip": false
    },
    {
      "type": "Stop",
      "road_distances": {
        "Biryulyovo Tovarnaya": 2600,
        "TETs 26": 1100
      },
      "longitude": 37.6517,
      "name": "Biryulyovo Zapadnoye",
      "latitude": 55.574371
    },
    {
      "type": "Stop",
      "road_distances": {
        "Biryusinka": 760,
        "Biryulyovo Tovarnaya": 1380,
        "Pokrovskaya": 2460
      },
      "longitude": 37.645687,
      "name": "Universam",
      "latitude": 55.587655
    },
    {
      "type": "Stop",
      "road_distances": {
        "Universam": 890
      },
      "longitude": 37.653656,
      "name": "Biryulyovo Tovarnaya",
      "latitude": 55.592028
    },
    {
      "type": "Stop",
      "road_distances": {
        "Apteka": 210,
        "TETs 26": 400
      },
      "longitude": 37.64839,
      "name": "Biryusinka",
      "latitude": 55.581065
    },
    {
      "type": "Stop",
      "road_distances": {
        "Biryulyovo Zapadnoye": 1420
      },
      "longitude": 37.652296,
      "name": "Apteka",
      "latitude": 55.580023
    },
    {
      "type": "Stop",
      "road_distances": {
        "Pokrovskaya": 2850
      },
      "longitude": 37.642258,
      "name": "TETs 26",
      "latitude": 55.580685
    },
    {
      "type": "Stop",
      "road_distances": {
        "Rossoshanskaya ulitsa": 3140
      },
      "longitude": 37.635517,
      "name": "Pokrovskaya",
      "latitude": 55.603601
    },
    {
      "type": "Stop",
      "road_distances": {
        "Pokrovskaya": 3210
      },
      "longitude": 37.605757,
      "name": "Rossoshanskaya ulitsa",
      "latitude": 55.595579
    },
    {
      "type": "Stop",
      "road_distances": {
        "Pokrovskaya": 2260
      },
      "longitude": 37.603938,
      "name": "Prazhskaya",
      "latitude": 55.611717
    },
    {
      "type": "Bus",
      "name": "750",
      "stops": [
        "Tolstopaltsevo",
        "Rasskazovka"
      ],
      "is_roundtrip": false
    },
    {
      "type": "Stop",
      "road_distances": {
        "Rasskazovka": 13800
      },
      "longitude": 37.20829,
      "name": "Tolstopaltsevo",
      "latitude": 55.611087
    },
    {
      "type": "Stop",
      "road_distances": {},
      "longitude": 37.333324,
      "name": "Rasskazovka",
      "latitude": 55.632761
    }
  ],
  "stat_requests": [
    {
      "type": "Bus",
      "name": "297",
      "id": 1
    },
    {
      "type": "Bus",
      "name": "635",
      "id": 2
    },
    {
      "type": "Bus",
      "name": "828",
      "id": 3
    },
    {
      "type": "Stop",
      "name": "Universam",
      "id": 4
    },
    {
      "type": "Route",
      "from": "Biryulyovo Zapadnoye",
      "to": "Apteka",
      "id": 5
    },
    {
      "type": "Route",
      "from": "Biryulyovo Zapadnoye",
      "to": "Pokrovskaya",
      "id": 6
    },
    {
      "type": "Route",
      "from": "Biryulyovo Tovarnaya",
      "to": "Pokrovskaya",
      "id": 7
    },
    {
      "type": "Route",
      "from": "Biryulyovo Tovarnaya",
      "to": "Biryulyovo Zapadnoye",
      "id": 8
    },
    {
      "type": "Route",
      "from": "Biryulyovo Tovarnaya",
      "to": "Prazhskaya",
      "id": 9
    },
    {
      "type": "Route",
      "from": "Apteka",
      "to": "Biryulyovo Tovarnaya",
      "id": 10
    },
    {
      "type": "Route",
      "from": "Biryulyovo Zapadnoye",
      "to": "Tolstopaltsevo",
      "id": 11
    }
  ]
})");

        ostringstream oss;

        Parse(iss, oss);

        istringstream iss_expect(R"([
  {
    "request_id": 1,
    "stop_count": 6,
    "unique_stop_count": 5,
    "route_length": 5880,
    "curvature": 1.36159
  },
  {
    "request_id": 2,
    "stop_count": 11,
    "unique_stop_count": 6,
    "route_length": 14810,
    "curvature": 1.12195
  },
  {
    "request_id": 3,
    "stop_count": 11,
    "unique_stop_count": 6,
    "route_length": 15790,
    "curvature": 1.31245
  },
  {
    "request_id": 4,
    "buses": [
      "297",
      "635",
      "828"
    ]
  },
  {
    "request_id": 5,
    "total_time": 7.42,
    "items": [
        {
            "time": 2,
            "type": "Wait",
            "stop_name": "Biryulyovo Zapadnoye"
        },
        {
            "span_count": 2,
            "bus": "828",
            "type": "Bus",
            "time": 3
        },
        {
            "time": 2,
            "type": "Wait",
            "stop_name": "Biryusinka"
        },
        {
            "span_count": 1,
            "bus": "297",
            "type": "Bus",
            "time": 0.42
        }
    ]
  },
  {
    "request_id": 6,
    "total_time": 11.44,
    "items": [
        {
            "time": 2,
            "type": "Wait",
            "stop_name": "Biryulyovo Zapadnoye"
        },
        {
            "span_count": 4,
            "bus": "828",
            "type": "Bus",
            "time": 9.44
        }
    ]
  },
  {
    "request_id": 7,
    "total_time": 10.7,
    "items": [
        {
            "time": 2,
            "type": "Wait",
            "stop_name": "Biryulyovo Tovarnaya"
        },
        {
            "span_count": 1,
            "bus": "635",
            "type": "Bus",
            "time": 1.78
        },
        {
            "time": 2,
            "type": "Wait",
            "stop_name": "Universam"
        },
        {
            "span_count": 1,
            "bus": "828",
            "type": "Bus",
            "time": 4.92
        }
    ]
  },
  {
    "request_id": 8,
    "total_time": 8.56,
    "items": [
        {
            "time": 2,
            "type": "Wait",
            "stop_name": "Biryulyovo Tovarnaya"
        },
        {
            "span_count": 4,
            "bus": "297",
            "type": "Bus",
            "time": 6.56
        }
    ]
  },
  {
    "request_id": 9,
    "total_time": 16.32,
    "items": [
        {
            "time": 2,
            "type": "Wait",
            "stop_name": "Biryulyovo Tovarnaya"
        },
        {
            "span_count": 5,
            "bus": "635",
            "type": "Bus",
            "time": 14.32
        }
    ]
  },
  {
    "request_id": 10,
    "total_time": 12.04,
    "items": [
        {
            "time": 2,
            "type": "Wait",
            "stop_name": "Apteka"
        },
        {
            "span_count": 1,
            "bus": "297",
            "type": "Bus",
            "time": 2.84
        },
        {
            "time": 2,
            "type": "Wait",
            "stop_name": "Biryulyovo Zapadnoye"
        },
        {
            "span_count": 1,
            "bus": "297",
            "type": "Bus",
            "time": 5.2
        }
    ]
  },
  {
    "request_id": 11,
    "error_message": "not found"
  }
])");

        string str = oss.str();
        string str_expect = iss_expect.str();

        // ASSERT_EQUAL(str, str_expect); // TODO: bring back
    }
    {
        istringstream iss(R"({
  "routing_settings": {
    "bus_velocity": 280,
    "bus_wait_time": 490
  },
  "base_requests": [
    {
      "type": "Stop",
      "latitude": 55.572308,
      "name": "Vnukovo",
      "road_distances": {
        "Kiyevskoye sh 50": 224766,
        "Kiyevskoye sh 10": 736717,
        "Kiyevskoye sh 90": 855121,
        "Troparyovo": 315031,
        "Kiyevskoye sh 20": 85080,
        "Kiyevskoye sh 100": 737402,
        "Kiyevskoye sh 60": 902131,
        "Kiyevskoye sh 30": 426639,
        "Kiyevskoye sh 70": 328591,
        "Kiyevskoye sh 80": 805100,
        "Kiyevskoye sh 40": 141884
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.575724434343435,
      "name": "Kiyevskoye sh 10",
      "road_distances": {
        "Kiyevskoye sh 50": 600182,
        "Kiyevskoye sh 90": 463120,
        "Troparyovo": 525298,
        "Kiyevskoye sh 20": 106788,
        "Kiyevskoye sh 100": 445277,
        "Kiyevskoye sh 60": 550985,
        "Kiyevskoye sh 30": 490517,
        "Kiyevskoye sh 70": 677645,
        "Kiyevskoye sh 80": 728727,
        "Kiyevskoye sh 40": 322450
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.57914086868687,
      "name": "Kiyevskoye sh 20",
      "road_distances": {
        "Kiyevskoye sh 50": 645868,
        "Kiyevskoye sh 90": 407045,
        "Troparyovo": 172308,
        "Kiyevskoye sh 60": 246041,
        "Kiyevskoye sh 100": 937377,
        "Kiyevskoye sh 30": 105345,
        "Kiyevskoye sh 70": 22704,
        "Kiyevskoye sh 80": 235980,
        "Kiyevskoye sh 40": 667602
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.582557303030306,
      "name": "Kiyevskoye sh 30",
      "road_distances": {
        "Kiyevskoye sh 50": 36945,
        "Kiyevskoye sh 90": 509332,
        "Troparyovo": 73536,
        "Kiyevskoye sh 60": 353820,
        "Kiyevskoye sh 100": 152038,
        "Kiyevskoye sh 80": 484228,
        "Kiyevskoye sh 70": 772561,
        "Kiyevskoye sh 40": 910439
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.585973737373735,
      "name": "Kiyevskoye sh 40",
      "road_distances": {
        "Kiyevskoye sh 50": 87601,
        "Kiyevskoye sh 90": 929215,
        "Troparyovo": 934646,
        "Kiyevskoye sh 60": 501890,
        "Kiyevskoye sh 100": 242694,
        "Kiyevskoye sh 80": 692055,
        "Kiyevskoye sh 70": 758953
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.58939017171717,
      "name": "Kiyevskoye sh 50",
      "road_distances": {
        "Kiyevskoye sh 90": 74238,
        "Troparyovo": 547802,
        "Kiyevskoye sh 60": 282245,
        "Kiyevskoye sh 100": 324219,
        "Kiyevskoye sh 80": 479166,
        "Kiyevskoye sh 70": 26703
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.592806606060606,
      "name": "Kiyevskoye sh 60",
      "road_distances": {
        "Kiyevskoye sh 70": 184654,
        "Kiyevskoye sh 80": 535966,
        "Kiyevskoye sh 90": 727877,
        "Troparyovo": 232275,
        "Kiyevskoye sh 100": 602778
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.59622304040404,
      "name": "Kiyevskoye sh 70",
      "road_distances": {
        "Kiyevskoye sh 80": 810005,
        "Kiyevskoye sh 90": 644986,
        "Troparyovo": 840545,
        "Kiyevskoye sh 100": 134053
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.59963947474748,
      "name": "Kiyevskoye sh 80",
      "road_distances": {
        "Kiyevskoye sh 90": 454384,
        "Troparyovo": 836752,
        "Kiyevskoye sh 100": 160260
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.60305590909091,
      "name": "Kiyevskoye sh 90",
      "road_distances": {
        "Troparyovo": 184657,
        "Kiyevskoye sh 100": 70662
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.60647234343434,
      "name": "Kiyevskoye sh 100",
      "road_distances": {
        "Troparyovo": 173788
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.609888777777776,
      "name": "Troparyovo",
      "road_distances": {
        "Pr Vernadskogo 20": 902943,
        "Pr Vernadskogo 10": 836197,
        "Pr Vernadskogo 30": 564884,
        "Pr Vernadskogo 80": 930723,
        "Luzhniki": 887456,
        "Pr Vernadskogo 60": 498213,
        "Pr Vernadskogo 50": 364630,
        "Pr Vernadskogo 40": 980896,
        "Pr Vernadskogo 90": 295332,
        "Pr Vernadskogo 100": 574235,
        "Pr Vernadskogo 70": 597621
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.61330521212121,
      "name": "Pr Vernadskogo 10",
      "road_distances": {
        "Pr Vernadskogo 20": 558592,
        "Pr Vernadskogo 30": 545777,
        "Pr Vernadskogo 80": 160023,
        "Luzhniki": 977111,
        "Pr Vernadskogo 60": 637256,
        "Pr Vernadskogo 50": 713919,
        "Pr Vernadskogo 40": 168166,
        "Pr Vernadskogo 90": 949897,
        "Pr Vernadskogo 100": 348790,
        "Pr Vernadskogo 70": 943314
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.61672164646465,
      "name": "Pr Vernadskogo 20",
      "road_distances": {
        "Pr Vernadskogo 30": 851499,
        "Pr Vernadskogo 80": 550584,
        "Luzhniki": 200615,
        "Pr Vernadskogo 60": 786189,
        "Pr Vernadskogo 50": 578956,
        "Pr Vernadskogo 40": 496844,
        "Pr Vernadskogo 90": 602501,
        "Pr Vernadskogo 100": 859709,
        "Pr Vernadskogo 70": 711943
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.62013808080808,
      "name": "Pr Vernadskogo 30",
      "road_distances": {
        "Pr Vernadskogo 80": 733468,
        "Luzhniki": 952098,
        "Pr Vernadskogo 60": 339208,
        "Pr Vernadskogo 50": 498886,
        "Pr Vernadskogo 40": 141064,
        "Pr Vernadskogo 90": 524049,
        "Pr Vernadskogo 100": 562657,
        "Pr Vernadskogo 70": 142678
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.62355451515152,
      "name": "Pr Vernadskogo 40",
      "road_distances": {
        "Pr Vernadskogo 80": 808295,
        "Luzhniki": 100270,
        "Pr Vernadskogo 60": 866946,
        "Pr Vernadskogo 50": 295640,
        "Pr Vernadskogo 90": 117432,
        "Pr Vernadskogo 100": 747819,
        "Pr Vernadskogo 70": 67558
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.62697094949495,
      "name": "Pr Vernadskogo 50",
      "road_distances": {
        "Pr Vernadskogo 80": 622572,
        "Pr Vernadskogo 100": 883706,
        "Luzhniki": 265883,
        "Pr Vernadskogo 60": 810434,
        "Pr Vernadskogo 90": 213567,
        "Pr Vernadskogo 70": 196610
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.63038738383838,
      "name": "Pr Vernadskogo 60",
      "road_distances": {
        "Pr Vernadskogo 90": 950262,
        "Pr Vernadskogo 100": 848332,
        "Pr Vernadskogo 80": 406520,
        "Luzhniki": 107368,
        "Pr Vernadskogo 70": 631894
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.63380381818182,
      "name": "Pr Vernadskogo 70",
      "road_distances": {
        "Pr Vernadskogo 90": 268917,
        "Pr Vernadskogo 80": 471955,
        "Luzhniki": 987823,
        "Pr Vernadskogo 100": 951875
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.63722025252525,
      "name": "Pr Vernadskogo 80",
      "road_distances": {
        "Pr Vernadskogo 90": 426514,
        "Pr Vernadskogo 100": 246386,
        "Luzhniki": 143938
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.64063668686869,
      "name": "Pr Vernadskogo 90",
      "road_distances": {
        "Pr Vernadskogo 100": 661195,
        "Luzhniki": 158399
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.644053121212124,
      "name": "Pr Vernadskogo 100",
      "road_distances": {
        "Luzhniki": 255798
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.64746955555555,
      "name": "Luzhniki",
      "road_distances": {
        "Komsomolskiy pr 70": 832566,
        "Komsomolskiy pr 50": 337894,
        "Komsomolskiy pr 30": 811711,
        "Komsomolskiy pr 10": 417641,
        "Komsomolskiy pr 80": 699119,
        "Komsomolskiy pr 100": 259353,
        "Komsomolskiy pr 40": 75025,
        "Komsomolskiy pr 60": 466038,
        "Komsomolskiy pr 20": 156200,
        "Komsomolskiy pr 90": 452348,
        "Yandex": 415895
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.65088598989899,
      "name": "Komsomolskiy pr 10",
      "road_distances": {
        "Komsomolskiy pr 70": 418608,
        "Komsomolskiy pr 50": 744074,
        "Komsomolskiy pr 30": 316051,
        "Komsomolskiy pr 80": 66706,
        "Komsomolskiy pr 100": 457844,
        "Komsomolskiy pr 40": 181773,
        "Komsomolskiy pr 60": 693784,
        "Komsomolskiy pr 20": 668171,
        "Komsomolskiy pr 90": 924652,
        "Yandex": 94288
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.654302424242424,
      "name": "Komsomolskiy pr 20",
      "road_distances": {
        "Komsomolskiy pr 60": 762905,
        "Komsomolskiy pr 50": 593963,
        "Komsomolskiy pr 30": 840262,
        "Komsomolskiy pr 80": 852222,
        "Komsomolskiy pr 100": 909761,
        "Komsomolskiy pr 40": 290776,
        "Yandex": 438867,
        "Komsomolskiy pr 70": 994402,
        "Komsomolskiy pr 90": 900384
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.65771885858586,
      "name": "Komsomolskiy pr 30",
      "road_distances": {
        "Komsomolskiy pr 60": 579265,
        "Komsomolskiy pr 50": 256707,
        "Komsomolskiy pr 80": 574750,
        "Komsomolskiy pr 100": 99385,
        "Komsomolskiy pr 40": 596911,
        "Yandex": 531071,
        "Komsomolskiy pr 70": 201563,
        "Komsomolskiy pr 90": 54708
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.661135292929295,
      "name": "Komsomolskiy pr 40",
      "road_distances": {
        "Komsomolskiy pr 50": 99840,
        "Komsomolskiy pr 100": 856,
        "Komsomolskiy pr 80": 896799,
        "Yandex": 943218,
        "Komsomolskiy pr 60": 874416,
        "Komsomolskiy pr 70": 309796,
        "Komsomolskiy pr 90": 923003
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.66455172727273,
      "name": "Komsomolskiy pr 50",
      "road_distances": {
        "Yandex": 39057,
        "Komsomolskiy pr 80": 751286,
        "Komsomolskiy pr 100": 827885,
        "Komsomolskiy pr 60": 438685,
        "Komsomolskiy pr 70": 625510,
        "Komsomolskiy pr 90": 92725
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.66796816161616,
      "name": "Komsomolskiy pr 60",
      "road_distances": {
        "Komsomolskiy pr 100": 253791,
        "Komsomolskiy pr 80": 499445,
        "Yandex": 827258,
        "Komsomolskiy pr 70": 356876,
        "Komsomolskiy pr 90": 203878
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.671384595959594,
      "name": "Komsomolskiy pr 70",
      "road_distances": {
        "Komsomolskiy pr 100": 488797,
        "Komsomolskiy pr 80": 40581,
        "Yandex": 210620,
        "Komsomolskiy pr 90": 767607
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.67480103030303,
      "name": "Komsomolskiy pr 80",
      "road_distances": {
        "Komsomolskiy pr 100": 281520,
        "Yandex": 733075,
        "Komsomolskiy pr 90": 570403
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.678217464646465,
      "name": "Komsomolskiy pr 90",
      "road_distances": {
        "Komsomolskiy pr 100": 899761,
        "Yandex": 970128
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.6816338989899,
      "name": "Komsomolskiy pr 100",
      "road_distances": {
        "Yandex": 83325
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.685050333333336,
      "name": "Yandex",
      "road_distances": {
        "Ostozhenka 40": 324113,
        "Ostozhenka 20": 42160,
        "Kremlin": 811470,
        "Ostozhenka 30": 732305,
        "Ostozhenka 10": 362162,
        "Ostozhenka 70": 118763,
        "Ostozhenka 60": 808895,
        "Ostozhenka 50": 384075,
        "Ostozhenka 80": 978567,
        "Ostozhenka 90": 264159,
        "Ostozhenka 100": 947696
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.68846676767677,
      "name": "Ostozhenka 10",
      "road_distances": {
        "Ostozhenka 40": 732922,
        "Ostozhenka 20": 229719,
        "Kremlin": 107709,
        "Ostozhenka 30": 41161,
        "Ostozhenka 70": 287355,
        "Ostozhenka 60": 409419,
        "Ostozhenka 50": 552514,
        "Ostozhenka 80": 682993,
        "Ostozhenka 90": 678183,
        "Ostozhenka 100": 738362
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.6918832020202,
      "name": "Ostozhenka 20",
      "road_distances": {
        "Ostozhenka 40": 380209,
        "Kremlin": 4315,
        "Ostozhenka 30": 992461,
        "Ostozhenka 70": 186673,
        "Ostozhenka 60": 180834,
        "Ostozhenka 50": 775692,
        "Ostozhenka 80": 445144,
        "Ostozhenka 90": 257670,
        "Ostozhenka 100": 862281
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.695299636363636,
      "name": "Ostozhenka 30",
      "road_distances": {
        "Ostozhenka 40": 965224,
        "Kremlin": 154795,
        "Ostozhenka 70": 554544,
        "Ostozhenka 60": 441618,
        "Ostozhenka 50": 700566,
        "Ostozhenka 80": 450794,
        "Ostozhenka 90": 918612,
        "Ostozhenka 100": 727560
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.69871607070707,
      "name": "Ostozhenka 40",
      "road_distances": {
        "Kremlin": 640973,
        "Ostozhenka 70": 910453,
        "Ostozhenka 60": 887844,
        "Ostozhenka 50": 158535,
        "Ostozhenka 80": 225862,
        "Ostozhenka 90": 815686,
        "Ostozhenka 100": 100776
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.70213250505051,
      "name": "Ostozhenka 50",
      "road_distances": {
        "Kremlin": 41057,
        "Ostozhenka 70": 868764,
        "Ostozhenka 60": 349871,
        "Ostozhenka 80": 792848,
        "Ostozhenka 90": 308860,
        "Ostozhenka 100": 792079
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.70554893939394,
      "name": "Ostozhenka 60",
      "road_distances": {
        "Kremlin": 461448,
        "Ostozhenka 90": 456590,
        "Ostozhenka 100": 191933,
        "Ostozhenka 80": 418588,
        "Ostozhenka 70": 801900
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.70896537373738,
      "name": "Ostozhenka 70",
      "road_distances": {
        "Kremlin": 801770,
        "Ostozhenka 90": 36336,
        "Ostozhenka 100": 361807,
        "Ostozhenka 80": 205203
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.712381808080806,
      "name": "Ostozhenka 80",
      "road_distances": {
        "Kremlin": 27882,
        "Ostozhenka 90": 834657,
        "Ostozhenka 100": 752170
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.71579824242424,
      "name": "Ostozhenka 90",
      "road_distances": {
        "Kremlin": 110754,
        "Ostozhenka 100": 646264
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.71921467676768,
      "name": "Ostozhenka 100",
      "road_distances": {
        "Kremlin": 626138
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.72263111111111,
      "name": "Kremlin",
      "road_distances": {
        "Mokhovaya ul 30": 454480,
        "Mokhovaya ul 80": 167343,
        "Mokhovaya ul 90": 475071,
        "Mokhovaya ul 20": 476712,
        "Mokhovaya ul 40": 616020,
        "Manezh": 279323,
        "Mokhovaya ul 100": 367294,
        "Mokhovaya ul 50": 536791,
        "Mokhovaya ul 10": 762623,
        "Mokhovaya ul 60": 201055,
        "Mokhovaya ul 70": 966883
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.72604754545455,
      "name": "Mokhovaya ul 10",
      "road_distances": {
        "Mokhovaya ul 30": 538446,
        "Mokhovaya ul 80": 72545,
        "Mokhovaya ul 90": 279217,
        "Mokhovaya ul 20": 124758,
        "Mokhovaya ul 40": 110126,
        "Manezh": 800695,
        "Mokhovaya ul 100": 828880,
        "Mokhovaya ul 50": 890127,
        "Mokhovaya ul 70": 988292,
        "Mokhovaya ul 60": 641793
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.729463979797984,
      "name": "Mokhovaya ul 20",
      "road_distances": {
        "Mokhovaya ul 30": 159183,
        "Mokhovaya ul 80": 176142,
        "Mokhovaya ul 90": 255753,
        "Mokhovaya ul 40": 801657,
        "Manezh": 55661,
        "Mokhovaya ul 100": 567473,
        "Mokhovaya ul 50": 349599,
        "Mokhovaya ul 70": 733921,
        "Mokhovaya ul 60": 647386
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.73288041414141,
      "name": "Mokhovaya ul 30",
      "road_distances": {
        "Mokhovaya ul 80": 675116,
        "Mokhovaya ul 100": 780836,
        "Mokhovaya ul 40": 782997,
        "Manezh": 741715,
        "Mokhovaya ul 90": 189985,
        "Mokhovaya ul 50": 588824,
        "Mokhovaya ul 70": 261962,
        "Mokhovaya ul 60": 182143
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.73629684848485,
      "name": "Mokhovaya ul 40",
      "road_distances": {
        "Mokhovaya ul 80": 314425,
        "Mokhovaya ul 90": 139294,
        "Manezh": 698907,
        "Mokhovaya ul 100": 395507,
        "Mokhovaya ul 50": 908127,
        "Mokhovaya ul 70": 782227,
        "Mokhovaya ul 60": 796139
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.73971328282828,
      "name": "Mokhovaya ul 50",
      "road_distances": {
        "Mokhovaya ul 80": 770135,
        "Mokhovaya ul 90": 964709,
        "Manezh": 686991,
        "Mokhovaya ul 100": 349823,
        "Mokhovaya ul 70": 400886,
        "Mokhovaya ul 60": 309102
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.74312971717172,
      "name": "Mokhovaya ul 60",
      "road_distances": {
        "Mokhovaya ul 80": 7260,
        "Manezh": 91851,
        "Mokhovaya ul 100": 709655,
        "Mokhovaya ul 70": 397978,
        "Mokhovaya ul 90": 807854
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.746546151515155,
      "name": "Mokhovaya ul 70",
      "road_distances": {
        "Mokhovaya ul 80": 364767,
        "Manezh": 246153,
        "Mokhovaya ul 100": 981369,
        "Mokhovaya ul 90": 822740
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.74996258585859,
      "name": "Mokhovaya ul 80",
      "road_distances": {
        "Manezh": 483836,
        "Mokhovaya ul 100": 281098,
        "Mokhovaya ul 90": 604053
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.75337902020202,
      "name": "Mokhovaya ul 90",
      "road_distances": {
        "Manezh": 567896,
        "Mokhovaya ul 100": 465494
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.756795454545454,
      "name": "Mokhovaya ul 100",
      "road_distances": {
        "Manezh": 522483
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.76021188888889,
      "name": "Manezh",
      "road_distances": {
        "Tverskaya ul 40": 860735,
        "Tverskaya ul 30": 175562,
        "Tverskaya ul 80": 58089,
        "Tverskaya ul 90": 603642,
        "Tverskaya ul 70": 885126,
        "Tverskaya ul 50": 724958,
        "Tverskaya ul 10": 139136,
        "Tverskaya ul 20": 76221,
        "Tverskaya ul 60": 549250,
        "Tverskaya ul 100": 462502,
        "Belorusskiy vokzal": 461914
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.763628323232325,
      "name": "Tverskaya ul 10",
      "road_distances": {
        "Tverskaya ul 40": 22320,
        "Tverskaya ul 30": 557638,
        "Tverskaya ul 80": 763662,
        "Tverskaya ul 90": 585997,
        "Tverskaya ul 70": 665612,
        "Tverskaya ul 50": 786366,
        "Tverskaya ul 20": 417668,
        "Tverskaya ul 60": 332733,
        "Tverskaya ul 100": 437397,
        "Belorusskiy vokzal": 495769
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.76704475757576,
      "name": "Tverskaya ul 20",
      "road_distances": {
        "Tverskaya ul 40": 457919,
        "Tverskaya ul 30": 536005,
        "Tverskaya ul 80": 751338,
        "Tverskaya ul 90": 550769,
        "Tverskaya ul 70": 373575,
        "Tverskaya ul 50": 732373,
        "Tverskaya ul 60": 739597,
        "Tverskaya ul 100": 853597,
        "Belorusskiy vokzal": 677417
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.770461191919196,
      "name": "Tverskaya ul 30",
      "road_distances": {
        "Tverskaya ul 40": 772389,
        "Tverskaya ul 50": 606556,
        "Tverskaya ul 80": 633142,
        "Tverskaya ul 90": 623210,
        "Tverskaya ul 70": 789251,
        "Tverskaya ul 60": 482644,
        "Tverskaya ul 100": 761134,
        "Belorusskiy vokzal": 412105
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.773877626262625,
      "name": "Tverskaya ul 40",
      "road_distances": {
        "Tverskaya ul 50": 5786,
        "Tverskaya ul 80": 526715,
        "Tverskaya ul 90": 100219,
        "Tverskaya ul 70": 981809,
        "Tverskaya ul 60": 916317,
        "Tverskaya ul 100": 305087,
        "Belorusskiy vokzal": 259145
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.77729406060606,
      "name": "Tverskaya ul 50",
      "road_distances": {
        "Tverskaya ul 80": 763869,
        "Tverskaya ul 90": 338049,
        "Tverskaya ul 70": 645067,
        "Tverskaya ul 60": 80909,
        "Tverskaya ul 100": 219713,
        "Belorusskiy vokzal": 813809
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.780710494949496,
      "name": "Tverskaya ul 60",
      "road_distances": {
        "Tverskaya ul 100": 382432,
        "Tverskaya ul 90": 398699,
        "Belorusskiy vokzal": 58383,
        "Tverskaya ul 80": 904939,
        "Tverskaya ul 70": 733974
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.78412692929293,
      "name": "Tverskaya ul 70",
      "road_distances": {
        "Tverskaya ul 80": 274018,
        "Tverskaya ul 90": 181417,
        "Tverskaya ul 100": 66076,
        "Belorusskiy vokzal": 979052
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.78754336363637,
      "name": "Tverskaya ul 80",
      "road_distances": {
        "Tverskaya ul 100": 317112,
        "Tverskaya ul 90": 691494,
        "Belorusskiy vokzal": 980356
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.7909597979798,
      "name": "Tverskaya ul 90",
      "road_distances": {
        "Tverskaya ul 100": 756291,
        "Belorusskiy vokzal": 740024
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.79437623232323,
      "name": "Tverskaya ul 100",
      "road_distances": {
        "Belorusskiy vokzal": 61392
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.797792666666666,
      "name": "Belorusskiy vokzal",
      "road_distances": {
        "Leningradskiy pr 30": 226044,
        "Leningradskiy pr 60": 596886,
        "Sokol": 350970,
        "Leningradskiy pr 70": 451599,
        "Leningradskiy pr 20": 686447,
        "Leningradskiy pr 40": 103944,
        "Leningradskiy pr 10": 283661,
        "Leningradskiy pr 90": 687681,
        "Leningradskiy pr 100": 899309,
        "Leningradskiy pr 50": 553418,
        "Leningradskiy pr 80": 692102
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.8012091010101,
      "name": "Leningradskiy pr 10",
      "road_distances": {
        "Leningradskiy pr 30": 542992,
        "Leningradskiy pr 60": 591490,
        "Sokol": 724104,
        "Leningradskiy pr 70": 324787,
        "Leningradskiy pr 20": 987980,
        "Leningradskiy pr 40": 388536,
        "Leningradskiy pr 90": 711251,
        "Leningradskiy pr 100": 222321,
        "Leningradskiy pr 50": 615982,
        "Leningradskiy pr 80": 815005
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.80462553535354,
      "name": "Leningradskiy pr 20",
      "road_distances": {
        "Leningradskiy pr 30": 427335,
        "Leningradskiy pr 60": 926860,
        "Sokol": 819473,
        "Leningradskiy pr 70": 140359,
        "Leningradskiy pr 50": 463650,
        "Leningradskiy pr 40": 338993,
        "Leningradskiy pr 90": 271574,
        "Leningradskiy pr 100": 235595,
        "Leningradskiy pr 80": 615161
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.80804196969697,
      "name": "Leningradskiy pr 30",
      "road_distances": {
        "Leningradskiy pr 60": 70515,
        "Sokol": 261558,
        "Leningradskiy pr 70": 731473,
        "Leningradskiy pr 50": 383668,
        "Leningradskiy pr 40": 746960,
        "Leningradskiy pr 90": 274494,
        "Leningradskiy pr 100": 115733,
        "Leningradskiy pr 80": 308837
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.81145840404041,
      "name": "Leningradskiy pr 40",
      "road_distances": {
        "Leningradskiy pr 60": 551357,
        "Sokol": 984185,
        "Leningradskiy pr 70": 264607,
        "Leningradskiy pr 80": 428193,
        "Leningradskiy pr 90": 661006,
        "Leningradskiy pr 100": 991279,
        "Leningradskiy pr 50": 529232
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.814874838383844,
      "name": "Leningradskiy pr 50",
      "road_distances": {
        "Leningradskiy pr 60": 393057,
        "Sokol": 876682,
        "Leningradskiy pr 70": 450512,
        "Leningradskiy pr 90": 556401,
        "Leningradskiy pr 100": 423491,
        "Leningradskiy pr 80": 311152
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.81829127272727,
      "name": "Leningradskiy pr 60",
      "road_distances": {
        "Leningradskiy pr 80": 427475,
        "Leningradskiy pr 90": 23990,
        "Leningradskiy pr 100": 218029,
        "Leningradskiy pr 70": 629079,
        "Sokol": 459041
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.82170770707071,
      "name": "Leningradskiy pr 70",
      "road_distances": {
        "Leningradskiy pr 80": 529294,
        "Leningradskiy pr 90": 495861,
        "Leningradskiy pr 100": 979024,
        "Sokol": 464139
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.82512414141414,
      "name": "Leningradskiy pr 80",
      "road_distances": {
        "Leningradskiy pr 90": 444521,
        "Sokol": 438456,
        "Leningradskiy pr 100": 836379
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.82854057575758,
      "name": "Leningradskiy pr 90",
      "road_distances": {
        "Sokol": 327680,
        "Leningradskiy pr 100": 999589
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.831957010101014,
      "name": "Leningradskiy pr 100",
      "road_distances": {
        "Sokol": 541447
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.83537344444445,
      "name": "Sokol",
      "road_distances": {
        "Leningradskoye sh 70": 666867,
        "Leningradskoye sh 20": 773493,
        "Leningradskoye sh 40": 316451,
        "Leningradskoye sh 50": 518203,
        "Leningradskoye sh 30": 345265,
        "Leningradskoye sh 10": 987134,
        "Khimki": 588282,
        "Leningradskoye sh 60": 804914,
        "Leningradskoye sh 80": 172159,
        "Leningradskoye sh 100": 247194,
        "Leningradskoye sh 90": 560888
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.83878987878788,
      "name": "Leningradskoye sh 10",
      "road_distances": {
        "Leningradskoye sh 70": 742393,
        "Leningradskoye sh 20": 1972,
        "Leningradskoye sh 40": 57794,
        "Leningradskoye sh 50": 849192,
        "Leningradskoye sh 30": 360084,
        "Khimki": 735651,
        "Leningradskoye sh 60": 971821,
        "Leningradskoye sh 80": 551027,
        "Leningradskoye sh 100": 312548,
        "Leningradskoye sh 90": 465365
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.842206313131314,
      "name": "Leningradskoye sh 20",
      "road_distances": {
        "Leningradskoye sh 70": 652424,
        "Leningradskoye sh 50": 828392,
        "Leningradskoye sh 40": 35252,
        "Leningradskoye sh 100": 685507,
        "Khimki": 414356,
        "Leningradskoye sh 60": 637716,
        "Leningradskoye sh 80": 78006,
        "Leningradskoye sh 30": 712653,
        "Leningradskoye sh 90": 852100
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.84562274747475,
      "name": "Leningradskoye sh 30",
      "road_distances": {
        "Leningradskoye sh 70": 809784,
        "Leningradskoye sh 40": 566350,
        "Leningradskoye sh 50": 288596,
        "Khimki": 452213,
        "Leningradskoye sh 60": 982376,
        "Leningradskoye sh 80": 539816,
        "Leningradskoye sh 100": 43422,
        "Leningradskoye sh 90": 630538
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.849039181818185,
      "name": "Leningradskoye sh 40",
      "road_distances": {
        "Leningradskoye sh 70": 18371,
        "Leningradskoye sh 90": 937231,
        "Leningradskoye sh 50": 40186,
        "Khimki": 395913,
        "Leningradskoye sh 60": 513623,
        "Leningradskoye sh 80": 220259,
        "Leningradskoye sh 100": 517423
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.85245561616162,
      "name": "Leningradskoye sh 50",
      "road_distances": {
        "Leningradskoye sh 70": 563913,
        "Leningradskoye sh 90": 694895,
        "Leningradskoye sh 60": 618685,
        "Khimki": 348222,
        "Leningradskoye sh 80": 396719,
        "Leningradskoye sh 100": 555241
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.855872050505056,
      "name": "Leningradskoye sh 60",
      "road_distances": {
        "Leningradskoye sh 70": 480330,
        "Khimki": 712387,
        "Leningradskoye sh 90": 886456,
        "Leningradskoye sh 80": 491096,
        "Leningradskoye sh 100": 882251
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.859288484848484,
      "name": "Leningradskoye sh 70",
      "road_distances": {
        "Leningradskoye sh 100": 190763,
        "Khimki": 205876,
        "Leningradskoye sh 80": 366398,
        "Leningradskoye sh 90": 204567
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.86270491919192,
      "name": "Leningradskoye sh 80",
      "road_distances": {
        "Leningradskoye sh 100": 751113,
        "Khimki": 249460,
        "Leningradskoye sh 90": 214169
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.866121353535355,
      "name": "Leningradskoye sh 90",
      "road_distances": {
        "Leningradskoye sh 100": 866890,
        "Khimki": 461541
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.86953778787879,
      "name": "Leningradskoye sh 100",
      "road_distances": {
        "Khimki": 281057
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.872954222222226,
      "name": "Khimki",
      "road_distances": {
        "Mezhdunarodnoye sh 100": 713929,
        "Mezhdunarodnoye sh 50": 80076,
        "Mezhdunarodnoye sh 20": 922777,
        "Mezhdunarodnoye sh 60": 484363,
        "Mezhdunarodnoye sh 10": 843751,
        "Mezhdunarodnoye sh 90": 493534,
        "Mezhdunarodnoye sh 30": 851892,
        "Sheremetyevo": 108496,
        "Mezhdunarodnoye sh 70": 229766,
        "Mezhdunarodnoye sh 40": 554280,
        "Mezhdunarodnoye sh 80": 390251
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.87637065656566,
      "name": "Mezhdunarodnoye sh 10",
      "road_distances": {
        "Mezhdunarodnoye sh 100": 347996,
        "Mezhdunarodnoye sh 50": 33896,
        "Mezhdunarodnoye sh 20": 18659,
        "Mezhdunarodnoye sh 60": 768709,
        "Mezhdunarodnoye sh 80": 386689,
        "Mezhdunarodnoye sh 30": 734538,
        "Sheremetyevo": 179450,
        "Mezhdunarodnoye sh 70": 740613,
        "Mezhdunarodnoye sh 40": 314135,
        "Mezhdunarodnoye sh 90": 584358
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.87978709090909,
      "name": "Mezhdunarodnoye sh 20",
      "road_distances": {
        "Mezhdunarodnoye sh 100": 910327,
        "Mezhdunarodnoye sh 50": 907700,
        "Mezhdunarodnoye sh 60": 947882,
        "Mezhdunarodnoye sh 80": 818414,
        "Mezhdunarodnoye sh 30": 712898,
        "Sheremetyevo": 954116,
        "Mezhdunarodnoye sh 70": 408992,
        "Mezhdunarodnoye sh 40": 816881,
        "Mezhdunarodnoye sh 90": 123185
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.883203525252526,
      "name": "Mezhdunarodnoye sh 30",
      "road_distances": {
        "Mezhdunarodnoye sh 100": 212137,
        "Mezhdunarodnoye sh 50": 976459,
        "Mezhdunarodnoye sh 60": 652253,
        "Mezhdunarodnoye sh 80": 844496,
        "Sheremetyevo": 927971,
        "Mezhdunarodnoye sh 70": 719362,
        "Mezhdunarodnoye sh 40": 482164,
        "Mezhdunarodnoye sh 90": 125821
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.88661995959596,
      "name": "Mezhdunarodnoye sh 40",
      "road_distances": {
        "Mezhdunarodnoye sh 100": 611679,
        "Mezhdunarodnoye sh 50": 394263,
        "Mezhdunarodnoye sh 60": 549004,
        "Mezhdunarodnoye sh 80": 515259,
        "Sheremetyevo": 549026,
        "Mezhdunarodnoye sh 70": 68393,
        "Mezhdunarodnoye sh 90": 265872
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.8900363939394,
      "name": "Mezhdunarodnoye sh 50",
      "road_distances": {
        "Mezhdunarodnoye sh 100": 531233,
        "Mezhdunarodnoye sh 70": 675532,
        "Mezhdunarodnoye sh 80": 249853,
        "Sheremetyevo": 462262,
        "Mezhdunarodnoye sh 60": 386287,
        "Mezhdunarodnoye sh 90": 795034
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.89345282828283,
      "name": "Mezhdunarodnoye sh 60",
      "road_distances": {
        "Sheremetyevo": 323957,
        "Mezhdunarodnoye sh 100": 605616,
        "Mezhdunarodnoye sh 70": 955350,
        "Mezhdunarodnoye sh 80": 881838,
        "Mezhdunarodnoye sh 90": 255008
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.89686926262627,
      "name": "Mezhdunarodnoye sh 70",
      "road_distances": {
        "Sheremetyevo": 767129,
        "Mezhdunarodnoye sh 100": 234880,
        "Mezhdunarodnoye sh 80": 666046,
        "Mezhdunarodnoye sh 90": 538917
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.900285696969696,
      "name": "Mezhdunarodnoye sh 80",
      "road_distances": {
        "Sheremetyevo": 653227,
        "Mezhdunarodnoye sh 100": 859392,
        "Mezhdunarodnoye sh 90": 509551
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.90370213131313,
      "name": "Mezhdunarodnoye sh 90",
      "road_distances": {
        "Sheremetyevo": 778255,
        "Mezhdunarodnoye sh 100": 303409
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.90711856565657,
      "name": "Mezhdunarodnoye sh 100",
      "road_distances": {
        "Sheremetyevo": 807092
      },
      "longitude": 37.58821
    },
    {
      "type": "Stop",
      "latitude": 55.910535,
      "name": "Sheremetyevo",
      "road_distances": {},
      "longitude": 37.58821
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "1",
      "stops": [
        "Vnukovo",
        "Kiyevskoye sh 20",
        "Kiyevskoye sh 30",
        "Kiyevskoye sh 70",
        "Troparyovo",
        "Luzhniki",
        "Komsomolskiy pr 30",
        "Komsomolskiy pr 40",
        "Komsomolskiy pr 50",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 40",
        "Mokhovaya ul 60",
        "Mokhovaya ul 70",
        "Mokhovaya ul 80",
        "Mokhovaya ul 90",
        "Mokhovaya ul 100",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 80",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 10",
        "Mezhdunarodnoye sh 100",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "2k",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 10",
        "Pr Vernadskogo 70",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 50",
        "Ostozhenka 90",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 10",
        "Tverskaya ul 20",
        "Belorusskiy vokzal",
        "Sokol",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "3k",
      "stops": [
        "Vnukovo",
        "Kiyevskoye sh 10",
        "Kiyevskoye sh 20",
        "Kiyevskoye sh 30",
        "Kiyevskoye sh 40",
        "Kiyevskoye sh 50",
        "Kiyevskoye sh 60",
        "Kiyevskoye sh 70",
        "Kiyevskoye sh 80",
        "Kiyevskoye sh 100",
        "Troparyovo",
        "Luzhniki",
        "Komsomolskiy pr 10",
        "Komsomolskiy pr 40",
        "Komsomolskiy pr 50",
        "Komsomolskiy pr 60",
        "Komsomolskiy pr 70",
        "Komsomolskiy pr 80",
        "Komsomolskiy pr 90",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 10",
        "Mokhovaya ul 20",
        "Mokhovaya ul 30",
        "Mokhovaya ul 40",
        "Mokhovaya ul 50",
        "Mokhovaya ul 70",
        "Mokhovaya ul 80",
        "Mokhovaya ul 100",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 10",
        "Leningradskiy pr 20",
        "Leningradskiy pr 30",
        "Leningradskiy pr 40",
        "Leningradskiy pr 60",
        "Leningradskiy pr 70",
        "Leningradskiy pr 80",
        "Leningradskiy pr 90",
        "Leningradskiy pr 100",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 20",
        "Mezhdunarodnoye sh 40",
        "Mezhdunarodnoye sh 50",
        "Mezhdunarodnoye sh 60",
        "Mezhdunarodnoye sh 90",
        "Mezhdunarodnoye sh 100",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "4",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 50",
        "Pr Vernadskogo 70",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 10",
        "Ostozhenka 100",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 20",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 40",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "5k",
      "stops": [
        "Vnukovo",
        "Kiyevskoye sh 70",
        "Troparyovo",
        "Luzhniki",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 90",
        "Manezh",
        "Belorusskiy vokzal",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 20",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "6k",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 100",
        "Luzhniki",
        "Yandex",
        "Kremlin",
        "Manezh",
        "Belorusskiy vokzal",
        "Sokol",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "7k",
      "stops": [
        "Vnukovo",
        "Kiyevskoye sh 30",
        "Kiyevskoye sh 40",
        "Kiyevskoye sh 50",
        "Kiyevskoye sh 70",
        "Kiyevskoye sh 100",
        "Troparyovo",
        "Luzhniki",
        "Komsomolskiy pr 10",
        "Komsomolskiy pr 20",
        "Komsomolskiy pr 40",
        "Komsomolskiy pr 60",
        "Komsomolskiy pr 90",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 10",
        "Mokhovaya ul 20",
        "Mokhovaya ul 30",
        "Mokhovaya ul 40",
        "Mokhovaya ul 50",
        "Mokhovaya ul 60",
        "Mokhovaya ul 80",
        "Mokhovaya ul 90",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 10",
        "Leningradskiy pr 20",
        "Leningradskiy pr 60",
        "Leningradskiy pr 70",
        "Leningradskiy pr 90",
        "Leningradskiy pr 100",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 20",
        "Mezhdunarodnoye sh 30",
        "Mezhdunarodnoye sh 40",
        "Mezhdunarodnoye sh 60",
        "Mezhdunarodnoye sh 70",
        "Mezhdunarodnoye sh 80",
        "Mezhdunarodnoye sh 90",
        "Mezhdunarodnoye sh 100",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "8",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 30",
        "Pr Vernadskogo 40",
        "Pr Vernadskogo 60",
        "Pr Vernadskogo 70",
        "Pr Vernadskogo 90",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 20",
        "Ostozhenka 30",
        "Ostozhenka 40",
        "Ostozhenka 50",
        "Ostozhenka 90",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 20",
        "Tverskaya ul 30",
        "Tverskaya ul 60",
        "Tverskaya ul 90",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 10",
        "Leningradskoye sh 20",
        "Leningradskoye sh 30",
        "Leningradskoye sh 40",
        "Leningradskoye sh 70",
        "Leningradskoye sh 90",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "9k",
      "stops": [
        "Vnukovo",
        "Kiyevskoye sh 10",
        "Kiyevskoye sh 20",
        "Kiyevskoye sh 30",
        "Kiyevskoye sh 40",
        "Kiyevskoye sh 50",
        "Kiyevskoye sh 60",
        "Kiyevskoye sh 70",
        "Kiyevskoye sh 100",
        "Troparyovo",
        "Luzhniki",
        "Komsomolskiy pr 10",
        "Komsomolskiy pr 20",
        "Komsomolskiy pr 30",
        "Komsomolskiy pr 40",
        "Komsomolskiy pr 50",
        "Komsomolskiy pr 70",
        "Komsomolskiy pr 80",
        "Komsomolskiy pr 90",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 10",
        "Mokhovaya ul 30",
        "Mokhovaya ul 40",
        "Mokhovaya ul 50",
        "Mokhovaya ul 60",
        "Mokhovaya ul 70",
        "Mokhovaya ul 80",
        "Mokhovaya ul 90",
        "Mokhovaya ul 100",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 10",
        "Leningradskiy pr 30",
        "Leningradskiy pr 40",
        "Leningradskiy pr 50",
        "Leningradskiy pr 60",
        "Leningradskiy pr 70",
        "Leningradskiy pr 80",
        "Leningradskiy pr 90",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 10",
        "Mezhdunarodnoye sh 20",
        "Mezhdunarodnoye sh 30",
        "Mezhdunarodnoye sh 60",
        "Mezhdunarodnoye sh 70",
        "Mezhdunarodnoye sh 90",
        "Mezhdunarodnoye sh 100",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "10k",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Luzhniki",
        "Yandex",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 40",
        "Belorusskiy vokzal",
        "Sokol",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "11",
      "stops": [
        "Vnukovo",
        "Kiyevskoye sh 30",
        "Kiyevskoye sh 60",
        "Kiyevskoye sh 70",
        "Kiyevskoye sh 80",
        "Kiyevskoye sh 90",
        "Kiyevskoye sh 100",
        "Troparyovo",
        "Luzhniki",
        "Komsomolskiy pr 20",
        "Komsomolskiy pr 30",
        "Komsomolskiy pr 70",
        "Komsomolskiy pr 80",
        "Komsomolskiy pr 90",
        "Komsomolskiy pr 100",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 20",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 30",
        "Leningradskiy pr 60",
        "Leningradskiy pr 80",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 10",
        "Mezhdunarodnoye sh 60",
        "Mezhdunarodnoye sh 70",
        "Mezhdunarodnoye sh 80",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "12",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 10",
        "Pr Vernadskogo 20",
        "Pr Vernadskogo 30",
        "Pr Vernadskogo 40",
        "Pr Vernadskogo 50",
        "Pr Vernadskogo 60",
        "Pr Vernadskogo 70",
        "Pr Vernadskogo 90",
        "Pr Vernadskogo 100",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 10",
        "Ostozhenka 20",
        "Ostozhenka 30",
        "Ostozhenka 40",
        "Ostozhenka 50",
        "Ostozhenka 70",
        "Ostozhenka 80",
        "Ostozhenka 90",
        "Ostozhenka 100",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 10",
        "Tverskaya ul 20",
        "Tverskaya ul 30",
        "Tverskaya ul 40",
        "Tverskaya ul 50",
        "Tverskaya ul 60",
        "Tverskaya ul 70",
        "Tverskaya ul 80",
        "Tverskaya ul 90",
        "Tverskaya ul 100",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 10",
        "Leningradskoye sh 20",
        "Leningradskoye sh 30",
        "Leningradskoye sh 40",
        "Leningradskoye sh 50",
        "Leningradskoye sh 70",
        "Leningradskoye sh 80",
        "Leningradskoye sh 90",
        "Leningradskoye sh 100",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "13",
      "stops": [
        "Vnukovo",
        "Kiyevskoye sh 20",
        "Kiyevskoye sh 30",
        "Kiyevskoye sh 60",
        "Kiyevskoye sh 80",
        "Kiyevskoye sh 90",
        "Kiyevskoye sh 100",
        "Troparyovo",
        "Luzhniki",
        "Komsomolskiy pr 10",
        "Komsomolskiy pr 30",
        "Komsomolskiy pr 50",
        "Komsomolskiy pr 100",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 20",
        "Mokhovaya ul 40",
        "Mokhovaya ul 50",
        "Mokhovaya ul 90",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 10",
        "Leningradskiy pr 20",
        "Leningradskiy pr 40",
        "Leningradskiy pr 60",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 20",
        "Mezhdunarodnoye sh 50",
        "Mezhdunarodnoye sh 60",
        "Mezhdunarodnoye sh 70",
        "Mezhdunarodnoye sh 100",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "14",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 10",
        "Pr Vernadskogo 20",
        "Pr Vernadskogo 40",
        "Pr Vernadskogo 60",
        "Pr Vernadskogo 70",
        "Pr Vernadskogo 100",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 10",
        "Ostozhenka 30",
        "Ostozhenka 40",
        "Ostozhenka 50",
        "Ostozhenka 60",
        "Ostozhenka 70",
        "Ostozhenka 80",
        "Ostozhenka 100",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 10",
        "Tverskaya ul 30",
        "Tverskaya ul 40",
        "Tverskaya ul 50",
        "Tverskaya ul 90",
        "Tverskaya ul 100",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 30",
        "Leningradskoye sh 40",
        "Leningradskoye sh 50",
        "Leningradskoye sh 80",
        "Leningradskoye sh 90",
        "Leningradskoye sh 100",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "15",
      "stops": [
        "Vnukovo",
        "Kiyevskoye sh 80",
        "Troparyovo",
        "Luzhniki",
        "Komsomolskiy pr 20",
        "Komsomolskiy pr 70",
        "Komsomolskiy pr 90",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 20",
        "Mokhovaya ul 40",
        "Mokhovaya ul 80",
        "Mokhovaya ul 100",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 10",
        "Leningradskiy pr 40",
        "Leningradskiy pr 70",
        "Leningradskiy pr 100",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 20",
        "Mezhdunarodnoye sh 60",
        "Mezhdunarodnoye sh 70",
        "Mezhdunarodnoye sh 90",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "16k",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 10",
        "Pr Vernadskogo 30",
        "Pr Vernadskogo 40",
        "Pr Vernadskogo 90",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 10",
        "Ostozhenka 20",
        "Ostozhenka 50",
        "Ostozhenka 60",
        "Ostozhenka 70",
        "Ostozhenka 100",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 20",
        "Tverskaya ul 30",
        "Tverskaya ul 40",
        "Tverskaya ul 50",
        "Tverskaya ul 60",
        "Tverskaya ul 70",
        "Tverskaya ul 80",
        "Tverskaya ul 90",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 10",
        "Leningradskoye sh 20",
        "Leningradskoye sh 40",
        "Leningradskoye sh 50",
        "Leningradskoye sh 60",
        "Leningradskoye sh 90",
        "Leningradskoye sh 100",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "17",
      "stops": [
        "Vnukovo",
        "Kiyevskoye sh 10",
        "Kiyevskoye sh 20",
        "Kiyevskoye sh 40",
        "Kiyevskoye sh 50",
        "Kiyevskoye sh 60",
        "Kiyevskoye sh 70",
        "Kiyevskoye sh 90",
        "Kiyevskoye sh 100",
        "Troparyovo",
        "Luzhniki",
        "Komsomolskiy pr 40",
        "Komsomolskiy pr 50",
        "Komsomolskiy pr 60",
        "Komsomolskiy pr 90",
        "Komsomolskiy pr 100",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 20",
        "Mokhovaya ul 30",
        "Mokhovaya ul 40",
        "Mokhovaya ul 70",
        "Mokhovaya ul 80",
        "Mokhovaya ul 90",
        "Mokhovaya ul 100",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 20",
        "Leningradskiy pr 30",
        "Leningradskiy pr 50",
        "Leningradskiy pr 60",
        "Leningradskiy pr 70",
        "Leningradskiy pr 100",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 50",
        "Mezhdunarodnoye sh 60",
        "Mezhdunarodnoye sh 70",
        "Mezhdunarodnoye sh 80",
        "Mezhdunarodnoye sh 100",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "18",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 10",
        "Pr Vernadskogo 20",
        "Pr Vernadskogo 30",
        "Pr Vernadskogo 40",
        "Pr Vernadskogo 70",
        "Pr Vernadskogo 90",
        "Pr Vernadskogo 100",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 10",
        "Ostozhenka 20",
        "Ostozhenka 40",
        "Ostozhenka 50",
        "Ostozhenka 60",
        "Ostozhenka 70",
        "Ostozhenka 80",
        "Ostozhenka 90",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 10",
        "Tverskaya ul 20",
        "Tverskaya ul 40",
        "Tverskaya ul 60",
        "Tverskaya ul 70",
        "Tverskaya ul 90",
        "Tverskaya ul 100",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 10",
        "Leningradskoye sh 20",
        "Leningradskoye sh 30",
        "Leningradskoye sh 40",
        "Leningradskoye sh 50",
        "Leningradskoye sh 60",
        "Leningradskoye sh 70",
        "Leningradskoye sh 90",
        "Leningradskoye sh 100",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "19k",
      "stops": [
        "Vnukovo",
        "Kiyevskoye sh 80",
        "Troparyovo",
        "Luzhniki",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 80",
        "Mokhovaya ul 90",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 70",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 90",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "20",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 10",
        "Pr Vernadskogo 100",
        "Luzhniki",
        "Yandex",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 10",
        "Tverskaya ul 20",
        "Tverskaya ul 70",
        "Tverskaya ul 80",
        "Tverskaya ul 90",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 20",
        "Leningradskoye sh 30",
        "Leningradskoye sh 40",
        "Leningradskoye sh 50",
        "Leningradskoye sh 60",
        "Leningradskoye sh 80",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "21k",
      "stops": [
        "Vnukovo",
        "Kiyevskoye sh 10",
        "Troparyovo",
        "Luzhniki",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 80",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 90",
        "Sokol",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "22k",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 30",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 50",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 50",
        "Tverskaya ul 60",
        "Tverskaya ul 90",
        "Tverskaya ul 100",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 90",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "23k",
      "stops": [
        "Vnukovo",
        "Kiyevskoye sh 20",
        "Kiyevskoye sh 40",
        "Kiyevskoye sh 80",
        "Kiyevskoye sh 100",
        "Troparyovo",
        "Luzhniki",
        "Komsomolskiy pr 20",
        "Komsomolskiy pr 30",
        "Komsomolskiy pr 40",
        "Komsomolskiy pr 60",
        "Komsomolskiy pr 70",
        "Komsomolskiy pr 100",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 20",
        "Mokhovaya ul 30",
        "Mokhovaya ul 50",
        "Mokhovaya ul 60",
        "Mokhovaya ul 80",
        "Mokhovaya ul 90",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 10",
        "Leningradskiy pr 20",
        "Leningradskiy pr 50",
        "Leningradskiy pr 60",
        "Leningradskiy pr 70",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 20",
        "Mezhdunarodnoye sh 30",
        "Mezhdunarodnoye sh 40",
        "Mezhdunarodnoye sh 50",
        "Mezhdunarodnoye sh 70",
        "Mezhdunarodnoye sh 80",
        "Mezhdunarodnoye sh 90",
        "Mezhdunarodnoye sh 100",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "24k",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 60",
        "Pr Vernadskogo 100",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 40",
        "Ostozhenka 50",
        "Ostozhenka 70",
        "Ostozhenka 100",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 20",
        "Tverskaya ul 30",
        "Tverskaya ul 40",
        "Tverskaya ul 80",
        "Tverskaya ul 90",
        "Tverskaya ul 100",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 40",
        "Leningradskoye sh 60",
        "Leningradskoye sh 70",
        "Leningradskoye sh 80",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "25k",
      "stops": [
        "Vnukovo",
        "Kiyevskoye sh 90",
        "Troparyovo",
        "Luzhniki",
        "Komsomolskiy pr 70",
        "Komsomolskiy pr 90",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 10",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 10",
        "Leningradskiy pr 30",
        "Leningradskiy pr 60",
        "Leningradskiy pr 70",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 100",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "26",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 40",
        "Pr Vernadskogo 80",
        "Pr Vernadskogo 100",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 20",
        "Ostozhenka 30",
        "Ostozhenka 90",
        "Ostozhenka 100",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 50",
        "Tverskaya ul 100",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 10",
        "Leningradskoye sh 30",
        "Leningradskoye sh 40",
        "Leningradskoye sh 50",
        "Leningradskoye sh 70",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "27k",
      "stops": [
        "Vnukovo",
        "Kiyevskoye sh 60",
        "Troparyovo",
        "Luzhniki",
        "Komsomolskiy pr 90",
        "Komsomolskiy pr 100",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 70",
        "Mokhovaya ul 90",
        "Manezh",
        "Belorusskiy vokzal",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 80",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "28",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 10",
        "Pr Vernadskogo 20",
        "Pr Vernadskogo 30",
        "Pr Vernadskogo 40",
        "Pr Vernadskogo 50",
        "Pr Vernadskogo 60",
        "Pr Vernadskogo 70",
        "Pr Vernadskogo 80",
        "Pr Vernadskogo 90",
        "Pr Vernadskogo 100",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 10",
        "Ostozhenka 20",
        "Ostozhenka 30",
        "Ostozhenka 40",
        "Ostozhenka 50",
        "Ostozhenka 60",
        "Ostozhenka 70",
        "Ostozhenka 80",
        "Ostozhenka 90",
        "Ostozhenka 100",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 10",
        "Tverskaya ul 20",
        "Tverskaya ul 30",
        "Tverskaya ul 40",
        "Tverskaya ul 50",
        "Tverskaya ul 60",
        "Tverskaya ul 70",
        "Tverskaya ul 80",
        "Tverskaya ul 90",
        "Tverskaya ul 100",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 10",
        "Leningradskoye sh 20",
        "Leningradskoye sh 30",
        "Leningradskoye sh 40",
        "Leningradskoye sh 50",
        "Leningradskoye sh 60",
        "Leningradskoye sh 70",
        "Leningradskoye sh 80",
        "Leningradskoye sh 90",
        "Leningradskoye sh 100",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "29k",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Luzhniki",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 20",
        "Mokhovaya ul 30",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 40",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 30",
        "Mezhdunarodnoye sh 90",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "30",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 10",
        "Pr Vernadskogo 40",
        "Pr Vernadskogo 50",
        "Pr Vernadskogo 60",
        "Pr Vernadskogo 70",
        "Pr Vernadskogo 80",
        "Pr Vernadskogo 90",
        "Pr Vernadskogo 100",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 20",
        "Ostozhenka 30",
        "Ostozhenka 40",
        "Ostozhenka 50",
        "Ostozhenka 60",
        "Ostozhenka 70",
        "Ostozhenka 80",
        "Ostozhenka 100",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 10",
        "Tverskaya ul 20",
        "Tverskaya ul 30",
        "Tverskaya ul 40",
        "Tverskaya ul 50",
        "Tverskaya ul 60",
        "Tverskaya ul 70",
        "Tverskaya ul 80",
        "Tverskaya ul 90",
        "Tverskaya ul 100",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 10",
        "Leningradskoye sh 20",
        "Leningradskoye sh 40",
        "Leningradskoye sh 60",
        "Leningradskoye sh 70",
        "Leningradskoye sh 80",
        "Leningradskoye sh 90",
        "Leningradskoye sh 100",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "31k",
      "stops": [
        "Vnukovo",
        "Kiyevskoye sh 30",
        "Kiyevskoye sh 40",
        "Kiyevskoye sh 50",
        "Kiyevskoye sh 60",
        "Kiyevskoye sh 70",
        "Kiyevskoye sh 80",
        "Kiyevskoye sh 90",
        "Kiyevskoye sh 100",
        "Troparyovo",
        "Luzhniki",
        "Komsomolskiy pr 10",
        "Komsomolskiy pr 30",
        "Komsomolskiy pr 50",
        "Komsomolskiy pr 60",
        "Komsomolskiy pr 70",
        "Komsomolskiy pr 80",
        "Komsomolskiy pr 100",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 10",
        "Mokhovaya ul 30",
        "Mokhovaya ul 40",
        "Mokhovaya ul 50",
        "Mokhovaya ul 60",
        "Mokhovaya ul 80",
        "Mokhovaya ul 90",
        "Mokhovaya ul 100",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 20",
        "Leningradskiy pr 30",
        "Leningradskiy pr 60",
        "Leningradskiy pr 70",
        "Leningradskiy pr 80",
        "Leningradskiy pr 90",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 20",
        "Mezhdunarodnoye sh 40",
        "Mezhdunarodnoye sh 50",
        "Mezhdunarodnoye sh 60",
        "Mezhdunarodnoye sh 80",
        "Mezhdunarodnoye sh 90",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "32",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 10",
        "Pr Vernadskogo 40",
        "Pr Vernadskogo 50",
        "Pr Vernadskogo 60",
        "Pr Vernadskogo 70",
        "Pr Vernadskogo 80",
        "Pr Vernadskogo 90",
        "Pr Vernadskogo 100",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 10",
        "Ostozhenka 20",
        "Ostozhenka 30",
        "Ostozhenka 40",
        "Ostozhenka 50",
        "Ostozhenka 60",
        "Ostozhenka 80",
        "Ostozhenka 90",
        "Ostozhenka 100",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 40",
        "Tverskaya ul 50",
        "Tverskaya ul 60",
        "Tverskaya ul 70",
        "Tverskaya ul 80",
        "Tverskaya ul 90",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 10",
        "Leningradskoye sh 20",
        "Leningradskoye sh 30",
        "Leningradskoye sh 40",
        "Leningradskoye sh 50",
        "Leningradskoye sh 60",
        "Leningradskoye sh 70",
        "Leningradskoye sh 80",
        "Leningradskoye sh 90",
        "Leningradskoye sh 100",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "33k",
      "stops": [
        "Vnukovo",
        "Kiyevskoye sh 20",
        "Kiyevskoye sh 30",
        "Kiyevskoye sh 40",
        "Kiyevskoye sh 50",
        "Kiyevskoye sh 70",
        "Kiyevskoye sh 90",
        "Troparyovo",
        "Luzhniki",
        "Komsomolskiy pr 20",
        "Komsomolskiy pr 30",
        "Komsomolskiy pr 40",
        "Komsomolskiy pr 50",
        "Komsomolskiy pr 80",
        "Komsomolskiy pr 90",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 10",
        "Mokhovaya ul 20",
        "Mokhovaya ul 30",
        "Mokhovaya ul 70",
        "Mokhovaya ul 90",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 10",
        "Leningradskiy pr 20",
        "Leningradskiy pr 40",
        "Leningradskiy pr 50",
        "Leningradskiy pr 70",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 10",
        "Mezhdunarodnoye sh 40",
        "Mezhdunarodnoye sh 50",
        "Mezhdunarodnoye sh 60",
        "Mezhdunarodnoye sh 70",
        "Mezhdunarodnoye sh 80",
        "Mezhdunarodnoye sh 90",
        "Mezhdunarodnoye sh 100",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "34",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 10",
        "Pr Vernadskogo 20",
        "Pr Vernadskogo 30",
        "Pr Vernadskogo 40",
        "Pr Vernadskogo 50",
        "Pr Vernadskogo 60",
        "Pr Vernadskogo 70",
        "Pr Vernadskogo 80",
        "Pr Vernadskogo 90",
        "Pr Vernadskogo 100",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 10",
        "Ostozhenka 20",
        "Ostozhenka 30",
        "Ostozhenka 50",
        "Ostozhenka 60",
        "Ostozhenka 70",
        "Ostozhenka 80",
        "Ostozhenka 90",
        "Ostozhenka 100",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 10",
        "Tverskaya ul 20",
        "Tverskaya ul 30",
        "Tverskaya ul 40",
        "Tverskaya ul 50",
        "Tverskaya ul 60",
        "Tverskaya ul 70",
        "Tverskaya ul 80",
        "Tverskaya ul 90",
        "Tverskaya ul 100",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 10",
        "Leningradskoye sh 20",
        "Leningradskoye sh 30",
        "Leningradskoye sh 40",
        "Leningradskoye sh 50",
        "Leningradskoye sh 60",
        "Leningradskoye sh 70",
        "Leningradskoye sh 80",
        "Leningradskoye sh 90",
        "Leningradskoye sh 100",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "35",
      "stops": [
        "Vnukovo",
        "Kiyevskoye sh 10",
        "Kiyevskoye sh 20",
        "Kiyevskoye sh 50",
        "Kiyevskoye sh 90",
        "Kiyevskoye sh 100",
        "Troparyovo",
        "Luzhniki",
        "Komsomolskiy pr 20",
        "Komsomolskiy pr 50",
        "Komsomolskiy pr 70",
        "Komsomolskiy pr 80",
        "Komsomolskiy pr 90",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 10",
        "Mokhovaya ul 20",
        "Mokhovaya ul 70",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 30",
        "Leningradskiy pr 40",
        "Leningradskiy pr 60",
        "Leningradskiy pr 70",
        "Leningradskiy pr 80",
        "Leningradskiy pr 90",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 10",
        "Mezhdunarodnoye sh 20",
        "Mezhdunarodnoye sh 30",
        "Mezhdunarodnoye sh 40",
        "Mezhdunarodnoye sh 50",
        "Mezhdunarodnoye sh 60",
        "Mezhdunarodnoye sh 80",
        "Mezhdunarodnoye sh 90",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "36",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 30",
        "Pr Vernadskogo 50",
        "Pr Vernadskogo 80",
        "Pr Vernadskogo 100",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 20",
        "Ostozhenka 50",
        "Ostozhenka 80",
        "Ostozhenka 100",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 10",
        "Tverskaya ul 20",
        "Tverskaya ul 50",
        "Tverskaya ul 70",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 10",
        "Leningradskoye sh 20",
        "Leningradskoye sh 50",
        "Leningradskoye sh 90",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "37k",
      "stops": [
        "Vnukovo",
        "Kiyevskoye sh 10",
        "Kiyevskoye sh 20",
        "Kiyevskoye sh 30",
        "Kiyevskoye sh 40",
        "Kiyevskoye sh 50",
        "Kiyevskoye sh 60",
        "Kiyevskoye sh 80",
        "Kiyevskoye sh 100",
        "Troparyovo",
        "Luzhniki",
        "Komsomolskiy pr 20",
        "Komsomolskiy pr 30",
        "Komsomolskiy pr 40",
        "Komsomolskiy pr 50",
        "Komsomolskiy pr 60",
        "Komsomolskiy pr 70",
        "Komsomolskiy pr 80",
        "Komsomolskiy pr 90",
        "Komsomolskiy pr 100",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 20",
        "Mokhovaya ul 30",
        "Mokhovaya ul 40",
        "Mokhovaya ul 60",
        "Mokhovaya ul 70",
        "Mokhovaya ul 100",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 30",
        "Leningradskiy pr 50",
        "Leningradskiy pr 60",
        "Leningradskiy pr 90",
        "Leningradskiy pr 100",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 10",
        "Mezhdunarodnoye sh 30",
        "Mezhdunarodnoye sh 60",
        "Mezhdunarodnoye sh 70",
        "Mezhdunarodnoye sh 80",
        "Mezhdunarodnoye sh 90",
        "Mezhdunarodnoye sh 100",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "38k",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 10",
        "Pr Vernadskogo 20",
        "Pr Vernadskogo 30",
        "Pr Vernadskogo 50",
        "Pr Vernadskogo 70",
        "Pr Vernadskogo 80",
        "Pr Vernadskogo 90",
        "Pr Vernadskogo 100",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 10",
        "Ostozhenka 20",
        "Ostozhenka 40",
        "Ostozhenka 50",
        "Ostozhenka 60",
        "Ostozhenka 70",
        "Ostozhenka 80",
        "Ostozhenka 100",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 10",
        "Tverskaya ul 20",
        "Tverskaya ul 30",
        "Tverskaya ul 40",
        "Tverskaya ul 50",
        "Tverskaya ul 60",
        "Tverskaya ul 70",
        "Tverskaya ul 90",
        "Tverskaya ul 100",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 30",
        "Leningradskoye sh 80",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "39k",
      "stops": [
        "Vnukovo",
        "Kiyevskoye sh 10",
        "Kiyevskoye sh 20",
        "Kiyevskoye sh 40",
        "Kiyevskoye sh 50",
        "Kiyevskoye sh 60",
        "Kiyevskoye sh 80",
        "Kiyevskoye sh 90",
        "Kiyevskoye sh 100",
        "Troparyovo",
        "Luzhniki",
        "Komsomolskiy pr 30",
        "Komsomolskiy pr 50",
        "Komsomolskiy pr 60",
        "Komsomolskiy pr 70",
        "Komsomolskiy pr 100",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 10",
        "Mokhovaya ul 100",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 10",
        "Leningradskiy pr 30",
        "Leningradskiy pr 40",
        "Leningradskiy pr 50",
        "Leningradskiy pr 60",
        "Leningradskiy pr 70",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 10",
        "Mezhdunarodnoye sh 30",
        "Mezhdunarodnoye sh 40",
        "Mezhdunarodnoye sh 50",
        "Mezhdunarodnoye sh 60",
        "Mezhdunarodnoye sh 80",
        "Mezhdunarodnoye sh 90",
        "Mezhdunarodnoye sh 100",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "40k",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 10",
        "Pr Vernadskogo 30",
        "Pr Vernadskogo 50",
        "Pr Vernadskogo 80",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 10",
        "Ostozhenka 20",
        "Ostozhenka 80",
        "Ostozhenka 90",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 10",
        "Tverskaya ul 20",
        "Tverskaya ul 30",
        "Tverskaya ul 50",
        "Tverskaya ul 70",
        "Tverskaya ul 80",
        "Tverskaya ul 100",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 10",
        "Leningradskoye sh 40",
        "Leningradskoye sh 50",
        "Leningradskoye sh 60",
        "Leningradskoye sh 70",
        "Leningradskoye sh 100",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "41",
      "stops": [
        "Vnukovo",
        "Kiyevskoye sh 20",
        "Kiyevskoye sh 30",
        "Kiyevskoye sh 40",
        "Kiyevskoye sh 70",
        "Kiyevskoye sh 90",
        "Kiyevskoye sh 100",
        "Troparyovo",
        "Luzhniki",
        "Komsomolskiy pr 10",
        "Komsomolskiy pr 20",
        "Komsomolskiy pr 30",
        "Komsomolskiy pr 40",
        "Komsomolskiy pr 50",
        "Komsomolskiy pr 60",
        "Komsomolskiy pr 70",
        "Komsomolskiy pr 80",
        "Komsomolskiy pr 100",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 20",
        "Mokhovaya ul 30",
        "Mokhovaya ul 60",
        "Mokhovaya ul 80",
        "Mokhovaya ul 90",
        "Mokhovaya ul 100",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 20",
        "Leningradskiy pr 30",
        "Leningradskiy pr 50",
        "Leningradskiy pr 70",
        "Leningradskiy pr 80",
        "Leningradskiy pr 90",
        "Leningradskiy pr 100",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 10",
        "Mezhdunarodnoye sh 20",
        "Mezhdunarodnoye sh 30",
        "Mezhdunarodnoye sh 50",
        "Mezhdunarodnoye sh 70",
        "Mezhdunarodnoye sh 80",
        "Mezhdunarodnoye sh 90",
        "Mezhdunarodnoye sh 100",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "42",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 80",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 40",
        "Kremlin",
        "Manezh",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 40",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "43k",
      "stops": [
        "Vnukovo",
        "Kiyevskoye sh 50",
        "Troparyovo",
        "Luzhniki",
        "Komsomolskiy pr 10",
        "Komsomolskiy pr 50",
        "Komsomolskiy pr 80",
        "Komsomolskiy pr 100",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 20",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 20",
        "Leningradskiy pr 50",
        "Sokol",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "44",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 30",
        "Pr Vernadskogo 50",
        "Pr Vernadskogo 60",
        "Pr Vernadskogo 80",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 20",
        "Ostozhenka 70",
        "Ostozhenka 80",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 90",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 100",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "45k",
      "stops": [
        "Vnukovo",
        "Kiyevskoye sh 30",
        "Kiyevskoye sh 40",
        "Kiyevskoye sh 50",
        "Kiyevskoye sh 70",
        "Kiyevskoye sh 90",
        "Kiyevskoye sh 100",
        "Troparyovo",
        "Luzhniki",
        "Komsomolskiy pr 20",
        "Komsomolskiy pr 30",
        "Komsomolskiy pr 40",
        "Komsomolskiy pr 50",
        "Komsomolskiy pr 70",
        "Komsomolskiy pr 80",
        "Komsomolskiy pr 90",
        "Komsomolskiy pr 100",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 20",
        "Mokhovaya ul 30",
        "Mokhovaya ul 40",
        "Mokhovaya ul 50",
        "Mokhovaya ul 80",
        "Mokhovaya ul 90",
        "Mokhovaya ul 100",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 40",
        "Leningradskiy pr 70",
        "Leningradskiy pr 90",
        "Leningradskiy pr 100",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 10",
        "Mezhdunarodnoye sh 20",
        "Mezhdunarodnoye sh 30",
        "Mezhdunarodnoye sh 40",
        "Mezhdunarodnoye sh 50",
        "Mezhdunarodnoye sh 60",
        "Mezhdunarodnoye sh 70",
        "Mezhdunarodnoye sh 80",
        "Mezhdunarodnoye sh 90",
        "Mezhdunarodnoye sh 100",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "46k",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 10",
        "Pr Vernadskogo 20",
        "Pr Vernadskogo 30",
        "Pr Vernadskogo 40",
        "Pr Vernadskogo 50",
        "Pr Vernadskogo 60",
        "Pr Vernadskogo 70",
        "Pr Vernadskogo 80",
        "Pr Vernadskogo 90",
        "Pr Vernadskogo 100",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 10",
        "Ostozhenka 40",
        "Ostozhenka 60",
        "Ostozhenka 70",
        "Ostozhenka 80",
        "Ostozhenka 90",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 20",
        "Tverskaya ul 40",
        "Tverskaya ul 50",
        "Tverskaya ul 60",
        "Tverskaya ul 70",
        "Tverskaya ul 90",
        "Tverskaya ul 100",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 20",
        "Leningradskoye sh 40",
        "Leningradskoye sh 50",
        "Leningradskoye sh 60",
        "Leningradskoye sh 90",
        "Leningradskoye sh 100",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "47k",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Luzhniki",
        "Komsomolskiy pr 100",
        "Yandex",
        "Kremlin",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 50",
        "Leningradskiy pr 60",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 30",
        "Mezhdunarodnoye sh 60",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "48k",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 10",
        "Pr Vernadskogo 20",
        "Pr Vernadskogo 30",
        "Pr Vernadskogo 40",
        "Pr Vernadskogo 50",
        "Pr Vernadskogo 60",
        "Pr Vernadskogo 70",
        "Pr Vernadskogo 80",
        "Pr Vernadskogo 90",
        "Pr Vernadskogo 100",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 10",
        "Ostozhenka 20",
        "Ostozhenka 30",
        "Ostozhenka 40",
        "Ostozhenka 50",
        "Ostozhenka 60",
        "Ostozhenka 70",
        "Ostozhenka 80",
        "Ostozhenka 90",
        "Ostozhenka 100",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 10",
        "Tverskaya ul 20",
        "Tverskaya ul 30",
        "Tverskaya ul 40",
        "Tverskaya ul 50",
        "Tverskaya ul 60",
        "Tverskaya ul 70",
        "Tverskaya ul 80",
        "Tverskaya ul 90",
        "Tverskaya ul 100",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 10",
        "Leningradskoye sh 20",
        "Leningradskoye sh 30",
        "Leningradskoye sh 40",
        "Leningradskoye sh 50",
        "Leningradskoye sh 60",
        "Leningradskoye sh 70",
        "Leningradskoye sh 80",
        "Leningradskoye sh 90",
        "Leningradskoye sh 100",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "49",
      "stops": [
        "Vnukovo",
        "Kiyevskoye sh 90",
        "Troparyovo",
        "Luzhniki",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 40",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 30",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 10",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "50",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 100",
        "Luzhniki",
        "Yandex",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 80",
        "Tverskaya ul 100",
        "Belorusskiy vokzal",
        "Sokol",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "51k",
      "stops": [
        "Vnukovo",
        "Kiyevskoye sh 20",
        "Kiyevskoye sh 30",
        "Kiyevskoye sh 40",
        "Kiyevskoye sh 60",
        "Kiyevskoye sh 90",
        "Kiyevskoye sh 100",
        "Troparyovo",
        "Luzhniki",
        "Komsomolskiy pr 20",
        "Komsomolskiy pr 40",
        "Komsomolskiy pr 50",
        "Komsomolskiy pr 60",
        "Komsomolskiy pr 80",
        "Komsomolskiy pr 90",
        "Komsomolskiy pr 100",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 10",
        "Mokhovaya ul 20",
        "Mokhovaya ul 40",
        "Mokhovaya ul 50",
        "Mokhovaya ul 60",
        "Mokhovaya ul 80",
        "Mokhovaya ul 90",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 10",
        "Leningradskiy pr 40",
        "Leningradskiy pr 50",
        "Leningradskiy pr 60",
        "Leningradskiy pr 70",
        "Leningradskiy pr 80",
        "Leningradskiy pr 90",
        "Leningradskiy pr 100",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 10",
        "Mezhdunarodnoye sh 20",
        "Mezhdunarodnoye sh 30",
        "Mezhdunarodnoye sh 40",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "52",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 10",
        "Pr Vernadskogo 20",
        "Pr Vernadskogo 30",
        "Pr Vernadskogo 40",
        "Pr Vernadskogo 50",
        "Pr Vernadskogo 60",
        "Pr Vernadskogo 70",
        "Pr Vernadskogo 80",
        "Pr Vernadskogo 90",
        "Pr Vernadskogo 100",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 10",
        "Ostozhenka 20",
        "Ostozhenka 40",
        "Ostozhenka 50",
        "Ostozhenka 60",
        "Ostozhenka 70",
        "Ostozhenka 90",
        "Ostozhenka 100",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 30",
        "Tverskaya ul 40",
        "Tverskaya ul 50",
        "Tverskaya ul 60",
        "Tverskaya ul 70",
        "Tverskaya ul 80",
        "Tverskaya ul 90",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 10",
        "Leningradskoye sh 20",
        "Leningradskoye sh 30",
        "Leningradskoye sh 50",
        "Leningradskoye sh 60",
        "Leningradskoye sh 70",
        "Leningradskoye sh 80",
        "Leningradskoye sh 90",
        "Leningradskoye sh 100",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "53k",
      "stops": [
        "Vnukovo",
        "Kiyevskoye sh 50",
        "Kiyevskoye sh 60",
        "Kiyevskoye sh 70",
        "Kiyevskoye sh 80",
        "Kiyevskoye sh 90",
        "Troparyovo",
        "Luzhniki",
        "Komsomolskiy pr 10",
        "Komsomolskiy pr 20",
        "Komsomolskiy pr 30",
        "Komsomolskiy pr 50",
        "Komsomolskiy pr 70",
        "Komsomolskiy pr 80",
        "Komsomolskiy pr 90",
        "Komsomolskiy pr 100",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 10",
        "Mokhovaya ul 50",
        "Mokhovaya ul 80",
        "Mokhovaya ul 90",
        "Mokhovaya ul 100",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 60",
        "Leningradskiy pr 90",
        "Leningradskiy pr 100",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 70",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "54",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 70",
        "Pr Vernadskogo 80",
        "Pr Vernadskogo 90",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 10",
        "Ostozhenka 30",
        "Ostozhenka 70",
        "Ostozhenka 80",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 10",
        "Tverskaya ul 60",
        "Tverskaya ul 90",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 20",
        "Leningradskoye sh 40",
        "Leningradskoye sh 90",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "55k",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Luzhniki",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 20",
        "Mokhovaya ul 80",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 90",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 40",
        "Mezhdunarodnoye sh 70",
        "Mezhdunarodnoye sh 100",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "56k",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 10",
        "Pr Vernadskogo 20",
        "Pr Vernadskogo 30",
        "Pr Vernadskogo 40",
        "Pr Vernadskogo 50",
        "Pr Vernadskogo 60",
        "Pr Vernadskogo 70",
        "Pr Vernadskogo 80",
        "Pr Vernadskogo 90",
        "Pr Vernadskogo 100",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 10",
        "Ostozhenka 20",
        "Ostozhenka 30",
        "Ostozhenka 40",
        "Ostozhenka 50",
        "Ostozhenka 60",
        "Ostozhenka 70",
        "Ostozhenka 80",
        "Ostozhenka 100",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 10",
        "Tverskaya ul 20",
        "Tverskaya ul 30",
        "Tverskaya ul 40",
        "Tverskaya ul 50",
        "Tverskaya ul 60",
        "Tverskaya ul 70",
        "Tverskaya ul 80",
        "Tverskaya ul 90",
        "Tverskaya ul 100",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 10",
        "Leningradskoye sh 20",
        "Leningradskoye sh 30",
        "Leningradskoye sh 40",
        "Leningradskoye sh 60",
        "Leningradskoye sh 70",
        "Leningradskoye sh 80",
        "Leningradskoye sh 90",
        "Leningradskoye sh 100",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "57k",
      "stops": [
        "Vnukovo",
        "Kiyevskoye sh 10",
        "Kiyevskoye sh 40",
        "Troparyovo",
        "Luzhniki",
        "Komsomolskiy pr 10",
        "Komsomolskiy pr 20",
        "Komsomolskiy pr 30",
        "Komsomolskiy pr 90",
        "Komsomolskiy pr 100",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 10",
        "Mokhovaya ul 20",
        "Mokhovaya ul 30",
        "Mokhovaya ul 50",
        "Mokhovaya ul 60",
        "Mokhovaya ul 70",
        "Mokhovaya ul 90",
        "Mokhovaya ul 100",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 20",
        "Leningradskiy pr 30",
        "Leningradskiy pr 40",
        "Leningradskiy pr 50",
        "Leningradskiy pr 60",
        "Leningradskiy pr 80",
        "Leningradskiy pr 90",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 20",
        "Mezhdunarodnoye sh 40",
        "Mezhdunarodnoye sh 50",
        "Mezhdunarodnoye sh 60",
        "Mezhdunarodnoye sh 70",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "58",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 10",
        "Pr Vernadskogo 20",
        "Pr Vernadskogo 30",
        "Pr Vernadskogo 50",
        "Pr Vernadskogo 60",
        "Pr Vernadskogo 70",
        "Pr Vernadskogo 100",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 20",
        "Ostozhenka 30",
        "Ostozhenka 40",
        "Ostozhenka 50",
        "Ostozhenka 60",
        "Ostozhenka 70",
        "Ostozhenka 80",
        "Ostozhenka 90",
        "Ostozhenka 100",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 10",
        "Tverskaya ul 20",
        "Tverskaya ul 30",
        "Tverskaya ul 50",
        "Tverskaya ul 60",
        "Tverskaya ul 70",
        "Tverskaya ul 80",
        "Tverskaya ul 90",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 10",
        "Leningradskoye sh 20",
        "Leningradskoye sh 40",
        "Leningradskoye sh 50",
        "Leningradskoye sh 60",
        "Leningradskoye sh 70",
        "Leningradskoye sh 80",
        "Leningradskoye sh 90",
        "Leningradskoye sh 100",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "59",
      "stops": [
        "Vnukovo",
        "Kiyevskoye sh 10",
        "Kiyevskoye sh 20",
        "Kiyevskoye sh 30",
        "Kiyevskoye sh 40",
        "Kiyevskoye sh 50",
        "Kiyevskoye sh 60",
        "Kiyevskoye sh 70",
        "Kiyevskoye sh 80",
        "Kiyevskoye sh 90",
        "Troparyovo",
        "Luzhniki",
        "Komsomolskiy pr 10",
        "Komsomolskiy pr 30",
        "Komsomolskiy pr 40",
        "Komsomolskiy pr 50",
        "Komsomolskiy pr 60",
        "Komsomolskiy pr 70",
        "Komsomolskiy pr 80",
        "Komsomolskiy pr 90",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 10",
        "Mokhovaya ul 30",
        "Mokhovaya ul 40",
        "Mokhovaya ul 70",
        "Mokhovaya ul 80",
        "Mokhovaya ul 100",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 10",
        "Leningradskiy pr 20",
        "Leningradskiy pr 30",
        "Leningradskiy pr 40",
        "Leningradskiy pr 50",
        "Leningradskiy pr 60",
        "Leningradskiy pr 70",
        "Leningradskiy pr 80",
        "Leningradskiy pr 90",
        "Leningradskiy pr 100",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 20",
        "Mezhdunarodnoye sh 30",
        "Mezhdunarodnoye sh 50",
        "Mezhdunarodnoye sh 80",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "60",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 40",
        "Pr Vernadskogo 50",
        "Pr Vernadskogo 60",
        "Pr Vernadskogo 70",
        "Pr Vernadskogo 80",
        "Pr Vernadskogo 90",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 30",
        "Ostozhenka 50",
        "Ostozhenka 80",
        "Ostozhenka 90",
        "Ostozhenka 100",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 10",
        "Tverskaya ul 20",
        "Tverskaya ul 40",
        "Tverskaya ul 50",
        "Tverskaya ul 60",
        "Tverskaya ul 90",
        "Tverskaya ul 100",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 10",
        "Leningradskoye sh 60",
        "Leningradskoye sh 70",
        "Leningradskoye sh 80",
        "Leningradskoye sh 90",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "61k",
      "stops": [
        "Vnukovo",
        "Kiyevskoye sh 50",
        "Kiyevskoye sh 90",
        "Kiyevskoye sh 100",
        "Troparyovo",
        "Luzhniki",
        "Komsomolskiy pr 10",
        "Komsomolskiy pr 20",
        "Komsomolskiy pr 40",
        "Komsomolskiy pr 50",
        "Komsomolskiy pr 70",
        "Komsomolskiy pr 90",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 10",
        "Mokhovaya ul 30",
        "Mokhovaya ul 40",
        "Mokhovaya ul 50",
        "Mokhovaya ul 60",
        "Mokhovaya ul 100",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 10",
        "Leningradskiy pr 50",
        "Leningradskiy pr 80",
        "Leningradskiy pr 100",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 40",
        "Mezhdunarodnoye sh 60",
        "Mezhdunarodnoye sh 70",
        "Mezhdunarodnoye sh 90",
        "Mezhdunarodnoye sh 100",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "62",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 10",
        "Pr Vernadskogo 20",
        "Pr Vernadskogo 30",
        "Pr Vernadskogo 40",
        "Pr Vernadskogo 50",
        "Pr Vernadskogo 60",
        "Pr Vernadskogo 70",
        "Pr Vernadskogo 90",
        "Pr Vernadskogo 100",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 10",
        "Ostozhenka 20",
        "Ostozhenka 30",
        "Ostozhenka 40",
        "Ostozhenka 60",
        "Ostozhenka 70",
        "Ostozhenka 80",
        "Ostozhenka 90",
        "Ostozhenka 100",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 10",
        "Tverskaya ul 20",
        "Tverskaya ul 30",
        "Tverskaya ul 40",
        "Tverskaya ul 50",
        "Tverskaya ul 60",
        "Tverskaya ul 90",
        "Tverskaya ul 100",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 10",
        "Leningradskoye sh 20",
        "Leningradskoye sh 40",
        "Leningradskoye sh 50",
        "Leningradskoye sh 70",
        "Leningradskoye sh 80",
        "Leningradskoye sh 90",
        "Leningradskoye sh 100",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "63",
      "stops": [
        "Vnukovo",
        "Kiyevskoye sh 10",
        "Kiyevskoye sh 20",
        "Kiyevskoye sh 40",
        "Kiyevskoye sh 50",
        "Kiyevskoye sh 70",
        "Kiyevskoye sh 80",
        "Kiyevskoye sh 90",
        "Kiyevskoye sh 100",
        "Troparyovo",
        "Luzhniki",
        "Komsomolskiy pr 10",
        "Komsomolskiy pr 30",
        "Komsomolskiy pr 40",
        "Komsomolskiy pr 50",
        "Komsomolskiy pr 60",
        "Komsomolskiy pr 80",
        "Komsomolskiy pr 100",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 10",
        "Mokhovaya ul 30",
        "Mokhovaya ul 40",
        "Mokhovaya ul 50",
        "Mokhovaya ul 60",
        "Mokhovaya ul 70",
        "Mokhovaya ul 80",
        "Mokhovaya ul 100",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 10",
        "Leningradskiy pr 20",
        "Leningradskiy pr 30",
        "Leningradskiy pr 50",
        "Leningradskiy pr 60",
        "Leningradskiy pr 80",
        "Leningradskiy pr 100",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 10",
        "Mezhdunarodnoye sh 20",
        "Mezhdunarodnoye sh 30",
        "Mezhdunarodnoye sh 40",
        "Mezhdunarodnoye sh 60",
        "Mezhdunarodnoye sh 70",
        "Mezhdunarodnoye sh 80",
        "Mezhdunarodnoye sh 90",
        "Mezhdunarodnoye sh 100",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "64k",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 30",
        "Pr Vernadskogo 70",
        "Pr Vernadskogo 100",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 40",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 90",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 20",
        "Leningradskoye sh 60",
        "Leningradskoye sh 70",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "65k",
      "stops": [
        "Vnukovo",
        "Kiyevskoye sh 10",
        "Kiyevskoye sh 30",
        "Kiyevskoye sh 50",
        "Kiyevskoye sh 80",
        "Troparyovo",
        "Luzhniki",
        "Komsomolskiy pr 10",
        "Komsomolskiy pr 20",
        "Komsomolskiy pr 30",
        "Komsomolskiy pr 40",
        "Komsomolskiy pr 50",
        "Komsomolskiy pr 60",
        "Komsomolskiy pr 70",
        "Komsomolskiy pr 80",
        "Komsomolskiy pr 90",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 10",
        "Mokhovaya ul 20",
        "Mokhovaya ul 40",
        "Mokhovaya ul 50",
        "Mokhovaya ul 60",
        "Mokhovaya ul 70",
        "Mokhovaya ul 100",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 10",
        "Leningradskiy pr 20",
        "Leningradskiy pr 30",
        "Leningradskiy pr 40",
        "Leningradskiy pr 50",
        "Leningradskiy pr 60",
        "Leningradskiy pr 70",
        "Leningradskiy pr 80",
        "Leningradskiy pr 90",
        "Leningradskiy pr 100",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 20",
        "Mezhdunarodnoye sh 30",
        "Mezhdunarodnoye sh 50",
        "Mezhdunarodnoye sh 60",
        "Mezhdunarodnoye sh 70",
        "Mezhdunarodnoye sh 80",
        "Mezhdunarodnoye sh 90",
        "Mezhdunarodnoye sh 100",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "66",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 50",
        "Pr Vernadskogo 100",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 40",
        "Ostozhenka 60",
        "Ostozhenka 100",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 70",
        "Tverskaya ul 90",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 10",
        "Leningradskoye sh 30",
        "Leningradskoye sh 50",
        "Leningradskoye sh 70",
        "Leningradskoye sh 90",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "67",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Luzhniki",
        "Yandex",
        "Kremlin",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 90",
        "Sokol",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "68k",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 10",
        "Pr Vernadskogo 20",
        "Pr Vernadskogo 30",
        "Pr Vernadskogo 40",
        "Pr Vernadskogo 50",
        "Pr Vernadskogo 60",
        "Pr Vernadskogo 70",
        "Pr Vernadskogo 80",
        "Pr Vernadskogo 90",
        "Pr Vernadskogo 100",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 10",
        "Ostozhenka 20",
        "Ostozhenka 30",
        "Ostozhenka 40",
        "Ostozhenka 50",
        "Ostozhenka 60",
        "Ostozhenka 70",
        "Ostozhenka 80",
        "Ostozhenka 90",
        "Ostozhenka 100",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 10",
        "Tverskaya ul 20",
        "Tverskaya ul 30",
        "Tverskaya ul 40",
        "Tverskaya ul 50",
        "Tverskaya ul 70",
        "Tverskaya ul 80",
        "Tverskaya ul 90",
        "Tverskaya ul 100",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 10",
        "Leningradskoye sh 20",
        "Leningradskoye sh 30",
        "Leningradskoye sh 40",
        "Leningradskoye sh 50",
        "Leningradskoye sh 60",
        "Leningradskoye sh 70",
        "Leningradskoye sh 80",
        "Leningradskoye sh 90",
        "Leningradskoye sh 100",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "69k",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Luzhniki",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 50",
        "Manezh",
        "Belorusskiy vokzal",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 20",
        "Mezhdunarodnoye sh 90",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "70k",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 10",
        "Pr Vernadskogo 20",
        "Pr Vernadskogo 40",
        "Pr Vernadskogo 50",
        "Pr Vernadskogo 60",
        "Pr Vernadskogo 80",
        "Pr Vernadskogo 90",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 30",
        "Ostozhenka 70",
        "Ostozhenka 80",
        "Ostozhenka 90",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 30",
        "Tverskaya ul 40",
        "Tverskaya ul 50",
        "Tverskaya ul 60",
        "Tverskaya ul 70",
        "Tverskaya ul 80",
        "Tverskaya ul 90",
        "Tverskaya ul 100",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 20",
        "Leningradskoye sh 30",
        "Leningradskoye sh 90",
        "Leningradskoye sh 100",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "71k",
      "stops": [
        "Vnukovo",
        "Kiyevskoye sh 80",
        "Troparyovo",
        "Luzhniki",
        "Yandex",
        "Kremlin",
        "Manezh",
        "Belorusskiy vokzal",
        "Sokol",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "72k",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 10",
        "Pr Vernadskogo 20",
        "Pr Vernadskogo 30",
        "Pr Vernadskogo 40",
        "Pr Vernadskogo 50",
        "Pr Vernadskogo 60",
        "Pr Vernadskogo 70",
        "Pr Vernadskogo 80",
        "Pr Vernadskogo 90",
        "Pr Vernadskogo 100",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 10",
        "Ostozhenka 50",
        "Ostozhenka 60",
        "Ostozhenka 80",
        "Ostozhenka 100",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 10",
        "Tverskaya ul 30",
        "Tverskaya ul 40",
        "Tverskaya ul 60",
        "Tverskaya ul 70",
        "Tverskaya ul 100",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 10",
        "Leningradskoye sh 30",
        "Leningradskoye sh 40",
        "Leningradskoye sh 70",
        "Leningradskoye sh 80",
        "Leningradskoye sh 90",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "73k",
      "stops": [
        "Vnukovo",
        "Kiyevskoye sh 10",
        "Troparyovo",
        "Luzhniki",
        "Komsomolskiy pr 20",
        "Komsomolskiy pr 60",
        "Komsomolskiy pr 70",
        "Komsomolskiy pr 100",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 10",
        "Mokhovaya ul 30",
        "Mokhovaya ul 40",
        "Mokhovaya ul 100",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 30",
        "Leningradskiy pr 70",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 10",
        "Mezhdunarodnoye sh 40",
        "Mezhdunarodnoye sh 80",
        "Mezhdunarodnoye sh 100",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "74k",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 10",
        "Pr Vernadskogo 20",
        "Pr Vernadskogo 30",
        "Pr Vernadskogo 40",
        "Pr Vernadskogo 50",
        "Pr Vernadskogo 60",
        "Pr Vernadskogo 70",
        "Pr Vernadskogo 80",
        "Pr Vernadskogo 90",
        "Pr Vernadskogo 100",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 10",
        "Ostozhenka 20",
        "Ostozhenka 30",
        "Ostozhenka 40",
        "Ostozhenka 50",
        "Ostozhenka 60",
        "Ostozhenka 70",
        "Ostozhenka 80",
        "Ostozhenka 90",
        "Ostozhenka 100",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 10",
        "Tverskaya ul 20",
        "Tverskaya ul 30",
        "Tverskaya ul 40",
        "Tverskaya ul 50",
        "Tverskaya ul 60",
        "Tverskaya ul 70",
        "Tverskaya ul 80",
        "Tverskaya ul 90",
        "Tverskaya ul 100",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 10",
        "Leningradskoye sh 20",
        "Leningradskoye sh 30",
        "Leningradskoye sh 40",
        "Leningradskoye sh 50",
        "Leningradskoye sh 60",
        "Leningradskoye sh 70",
        "Leningradskoye sh 80",
        "Leningradskoye sh 90",
        "Leningradskoye sh 100",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "75k",
      "stops": [
        "Vnukovo",
        "Kiyevskoye sh 30",
        "Kiyevskoye sh 50",
        "Kiyevskoye sh 60",
        "Kiyevskoye sh 70",
        "Kiyevskoye sh 80",
        "Kiyevskoye sh 90",
        "Troparyovo",
        "Luzhniki",
        "Komsomolskiy pr 10",
        "Komsomolskiy pr 30",
        "Komsomolskiy pr 50",
        "Komsomolskiy pr 60",
        "Komsomolskiy pr 70",
        "Komsomolskiy pr 80",
        "Komsomolskiy pr 90",
        "Komsomolskiy pr 100",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 10",
        "Mokhovaya ul 20",
        "Mokhovaya ul 60",
        "Mokhovaya ul 70",
        "Mokhovaya ul 90",
        "Mokhovaya ul 100",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 10",
        "Leningradskiy pr 20",
        "Leningradskiy pr 30",
        "Leningradskiy pr 40",
        "Leningradskiy pr 50",
        "Leningradskiy pr 60",
        "Leningradskiy pr 80",
        "Leningradskiy pr 90",
        "Leningradskiy pr 100",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 10",
        "Mezhdunarodnoye sh 20",
        "Mezhdunarodnoye sh 30",
        "Mezhdunarodnoye sh 50",
        "Mezhdunarodnoye sh 60",
        "Mezhdunarodnoye sh 70",
        "Mezhdunarodnoye sh 80",
        "Mezhdunarodnoye sh 90",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "76k",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 10",
        "Pr Vernadskogo 30",
        "Pr Vernadskogo 40",
        "Pr Vernadskogo 50",
        "Pr Vernadskogo 60",
        "Pr Vernadskogo 70",
        "Pr Vernadskogo 90",
        "Pr Vernadskogo 100",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 10",
        "Ostozhenka 20",
        "Ostozhenka 30",
        "Ostozhenka 40",
        "Ostozhenka 50",
        "Ostozhenka 60",
        "Ostozhenka 70",
        "Ostozhenka 80",
        "Ostozhenka 90",
        "Ostozhenka 100",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 10",
        "Tverskaya ul 20",
        "Tverskaya ul 30",
        "Tverskaya ul 40",
        "Tverskaya ul 50",
        "Tverskaya ul 60",
        "Tverskaya ul 70",
        "Tverskaya ul 80",
        "Tverskaya ul 100",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 20",
        "Leningradskoye sh 30",
        "Leningradskoye sh 40",
        "Leningradskoye sh 50",
        "Leningradskoye sh 60",
        "Leningradskoye sh 70",
        "Leningradskoye sh 90",
        "Leningradskoye sh 100",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "77k",
      "stops": [
        "Vnukovo",
        "Kiyevskoye sh 70",
        "Troparyovo",
        "Luzhniki",
        "Komsomolskiy pr 10",
        "Komsomolskiy pr 20",
        "Komsomolskiy pr 50",
        "Komsomolskiy pr 100",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 30",
        "Mokhovaya ul 70",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 60",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 80",
        "Mezhdunarodnoye sh 90",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "78k",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 10",
        "Pr Vernadskogo 30",
        "Pr Vernadskogo 50",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 60",
        "Ostozhenka 80",
        "Ostozhenka 90",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 40",
        "Tverskaya ul 60",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 30",
        "Leningradskoye sh 50",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "79",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Luzhniki",
        "Komsomolskiy pr 100",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 90",
        "Mokhovaya ul 100",
        "Manezh",
        "Belorusskiy vokzal",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 70",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "80k",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 30",
        "Pr Vernadskogo 60",
        "Pr Vernadskogo 80",
        "Pr Vernadskogo 100",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 40",
        "Ostozhenka 60",
        "Ostozhenka 90",
        "Ostozhenka 100",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 10",
        "Tverskaya ul 50",
        "Tverskaya ul 60",
        "Tverskaya ul 70",
        "Tverskaya ul 80",
        "Tverskaya ul 90",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 20",
        "Leningradskoye sh 40",
        "Leningradskoye sh 90",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "81",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Luzhniki",
        "Komsomolskiy pr 10",
        "Komsomolskiy pr 30",
        "Komsomolskiy pr 50",
        "Komsomolskiy pr 60",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 100",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 10",
        "Sokol",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "82k",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Luzhniki",
        "Yandex",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 70",
        "Tverskaya ul 90",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 30",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "83k",
      "stops": [
        "Vnukovo",
        "Kiyevskoye sh 20",
        "Kiyevskoye sh 70",
        "Troparyovo",
        "Luzhniki",
        "Komsomolskiy pr 30",
        "Yandex",
        "Kremlin",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 40",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 10",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "84k",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 30",
        "Pr Vernadskogo 50",
        "Pr Vernadskogo 60",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 20",
        "Ostozhenka 40",
        "Ostozhenka 70",
        "Ostozhenka 90",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 10",
        "Tverskaya ul 20",
        "Tverskaya ul 30",
        "Tverskaya ul 50",
        "Tverskaya ul 60",
        "Tverskaya ul 100",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 30",
        "Leningradskoye sh 40",
        "Leningradskoye sh 50",
        "Leningradskoye sh 60",
        "Leningradskoye sh 70",
        "Leningradskoye sh 80",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "85k",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Luzhniki",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 30",
        "Mokhovaya ul 60",
        "Mokhovaya ul 90",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 20",
        "Leningradskiy pr 70",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 30",
        "Mezhdunarodnoye sh 90",
        "Mezhdunarodnoye sh 100",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "86k",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 80",
        "Luzhniki",
        "Yandex",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 30",
        "Tverskaya ul 50",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 10",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "87k",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Luzhniki",
        "Yandex",
        "Kremlin",
        "Manezh",
        "Belorusskiy vokzal",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 30",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "88k",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 20",
        "Pr Vernadskogo 30",
        "Pr Vernadskogo 40",
        "Pr Vernadskogo 50",
        "Pr Vernadskogo 80",
        "Pr Vernadskogo 90",
        "Pr Vernadskogo 100",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 10",
        "Ostozhenka 40",
        "Ostozhenka 70",
        "Ostozhenka 90",
        "Ostozhenka 100",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 10",
        "Tverskaya ul 40",
        "Tverskaya ul 70",
        "Tverskaya ul 80",
        "Tverskaya ul 90",
        "Tverskaya ul 100",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 20",
        "Leningradskoye sh 40",
        "Leningradskoye sh 50",
        "Leningradskoye sh 60",
        "Leningradskoye sh 70",
        "Leningradskoye sh 80",
        "Leningradskoye sh 90",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "89",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Luzhniki",
        "Komsomolskiy pr 10",
        "Komsomolskiy pr 50",
        "Komsomolskiy pr 70",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 20",
        "Mokhovaya ul 100",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 60",
        "Sokol",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "90k",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 10",
        "Pr Vernadskogo 20",
        "Pr Vernadskogo 30",
        "Pr Vernadskogo 50",
        "Pr Vernadskogo 90",
        "Pr Vernadskogo 100",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 20",
        "Ostozhenka 30",
        "Ostozhenka 40",
        "Ostozhenka 60",
        "Ostozhenka 80",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 10",
        "Tverskaya ul 20",
        "Tverskaya ul 40",
        "Tverskaya ul 60",
        "Tverskaya ul 70",
        "Tverskaya ul 90",
        "Tverskaya ul 100",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 10",
        "Leningradskoye sh 40",
        "Leningradskoye sh 60",
        "Leningradskoye sh 80",
        "Leningradskoye sh 90",
        "Leningradskoye sh 100",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "91",
      "stops": [
        "Vnukovo",
        "Kiyevskoye sh 40",
        "Kiyevskoye sh 50",
        "Kiyevskoye sh 60",
        "Kiyevskoye sh 70",
        "Troparyovo",
        "Luzhniki",
        "Komsomolskiy pr 20",
        "Komsomolskiy pr 30",
        "Komsomolskiy pr 60",
        "Komsomolskiy pr 70",
        "Komsomolskiy pr 80",
        "Komsomolskiy pr 90",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 10",
        "Mokhovaya ul 20",
        "Mokhovaya ul 60",
        "Mokhovaya ul 70",
        "Mokhovaya ul 80",
        "Mokhovaya ul 90",
        "Mokhovaya ul 100",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 20",
        "Leningradskiy pr 40",
        "Leningradskiy pr 60",
        "Leningradskiy pr 80",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 10",
        "Mezhdunarodnoye sh 50",
        "Mezhdunarodnoye sh 60",
        "Mezhdunarodnoye sh 70",
        "Mezhdunarodnoye sh 80",
        "Mezhdunarodnoye sh 90",
        "Mezhdunarodnoye sh 100",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "92k",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 10",
        "Pr Vernadskogo 20",
        "Pr Vernadskogo 40",
        "Pr Vernadskogo 90",
        "Pr Vernadskogo 100",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 10",
        "Ostozhenka 20",
        "Ostozhenka 40",
        "Ostozhenka 50",
        "Ostozhenka 70",
        "Ostozhenka 80",
        "Ostozhenka 90",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 10",
        "Tverskaya ul 20",
        "Tverskaya ul 40",
        "Tverskaya ul 50",
        "Tverskaya ul 70",
        "Tverskaya ul 90",
        "Tverskaya ul 100",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 30",
        "Leningradskoye sh 70",
        "Leningradskoye sh 80",
        "Leningradskoye sh 90",
        "Leningradskoye sh 100",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "93k",
      "stops": [
        "Vnukovo",
        "Kiyevskoye sh 50",
        "Troparyovo",
        "Luzhniki",
        "Komsomolskiy pr 20",
        "Komsomolskiy pr 40",
        "Komsomolskiy pr 50",
        "Komsomolskiy pr 60",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 10",
        "Mokhovaya ul 80",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 10",
        "Leningradskiy pr 80",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 80",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "94k",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 30",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 80",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 20",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 10",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "95",
      "stops": [
        "Vnukovo",
        "Kiyevskoye sh 10",
        "Kiyevskoye sh 20",
        "Kiyevskoye sh 30",
        "Kiyevskoye sh 40",
        "Kiyevskoye sh 50",
        "Kiyevskoye sh 60",
        "Kiyevskoye sh 80",
        "Kiyevskoye sh 90",
        "Troparyovo",
        "Luzhniki",
        "Komsomolskiy pr 20",
        "Komsomolskiy pr 40",
        "Komsomolskiy pr 60",
        "Komsomolskiy pr 80",
        "Komsomolskiy pr 90",
        "Komsomolskiy pr 100",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 10",
        "Mokhovaya ul 20",
        "Mokhovaya ul 30",
        "Mokhovaya ul 40",
        "Mokhovaya ul 50",
        "Mokhovaya ul 60",
        "Mokhovaya ul 70",
        "Mokhovaya ul 80",
        "Mokhovaya ul 90",
        "Mokhovaya ul 100",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 10",
        "Leningradskiy pr 20",
        "Leningradskiy pr 40",
        "Leningradskiy pr 50",
        "Leningradskiy pr 80",
        "Leningradskiy pr 90",
        "Leningradskiy pr 100",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 10",
        "Mezhdunarodnoye sh 30",
        "Mezhdunarodnoye sh 40",
        "Mezhdunarodnoye sh 50",
        "Mezhdunarodnoye sh 60",
        "Mezhdunarodnoye sh 70",
        "Mezhdunarodnoye sh 80",
        "Mezhdunarodnoye sh 90",
        "Mezhdunarodnoye sh 100",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "96",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 40",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 10",
        "Ostozhenka 50",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 40",
        "Belorusskiy vokzal",
        "Sokol",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "97",
      "stops": [
        "Vnukovo",
        "Kiyevskoye sh 10",
        "Kiyevskoye sh 30",
        "Kiyevskoye sh 40",
        "Kiyevskoye sh 80",
        "Kiyevskoye sh 90",
        "Troparyovo",
        "Luzhniki",
        "Komsomolskiy pr 10",
        "Komsomolskiy pr 30",
        "Komsomolskiy pr 40",
        "Komsomolskiy pr 50",
        "Komsomolskiy pr 60",
        "Komsomolskiy pr 70",
        "Komsomolskiy pr 80",
        "Komsomolskiy pr 90",
        "Komsomolskiy pr 100",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 20",
        "Mokhovaya ul 30",
        "Mokhovaya ul 60",
        "Mokhovaya ul 70",
        "Mokhovaya ul 80",
        "Mokhovaya ul 90",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 20",
        "Leningradskiy pr 30",
        "Leningradskiy pr 50",
        "Leningradskiy pr 60",
        "Leningradskiy pr 70",
        "Leningradskiy pr 80",
        "Sokol",
        "Khimki",
        "Mezhdunarodnoye sh 10",
        "Mezhdunarodnoye sh 60",
        "Mezhdunarodnoye sh 80",
        "Mezhdunarodnoye sh 90",
        "Mezhdunarodnoye sh 100",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "98",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 40",
        "Pr Vernadskogo 50",
        "Pr Vernadskogo 70",
        "Pr Vernadskogo 100",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 10",
        "Ostozhenka 20",
        "Ostozhenka 30",
        "Ostozhenka 80",
        "Ostozhenka 100",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 50",
        "Tverskaya ul 60",
        "Tverskaya ul 80",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 10",
        "Leningradskoye sh 30",
        "Leningradskoye sh 50",
        "Leningradskoye sh 80",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "99k",
      "stops": [
        "Vnukovo",
        "Kiyevskoye sh 50",
        "Kiyevskoye sh 60",
        "Kiyevskoye sh 80",
        "Kiyevskoye sh 100",
        "Troparyovo",
        "Luzhniki",
        "Komsomolskiy pr 20",
        "Komsomolskiy pr 80",
        "Yandex",
        "Kremlin",
        "Mokhovaya ul 50",
        "Mokhovaya ul 60",
        "Manezh",
        "Belorusskiy vokzal",
        "Leningradskiy pr 20",
        "Leningradskiy pr 60",
        "Leningradskiy pr 80",
        "Sokol",
        "Khimki",
        "Sheremetyevo"
      ]
    },
    {
      "type": "Bus",
      "is_roundtrip": false,
      "name": "100k",
      "stops": [
        "Vnukovo",
        "Troparyovo",
        "Pr Vernadskogo 10",
        "Pr Vernadskogo 20",
        "Pr Vernadskogo 30",
        "Pr Vernadskogo 40",
        "Pr Vernadskogo 50",
        "Pr Vernadskogo 60",
        "Pr Vernadskogo 70",
        "Pr Vernadskogo 80",
        "Pr Vernadskogo 90",
        "Pr Vernadskogo 100",
        "Luzhniki",
        "Yandex",
        "Ostozhenka 10",
        "Ostozhenka 20",
        "Ostozhenka 30",
        "Ostozhenka 40",
        "Ostozhenka 50",
        "Ostozhenka 60",
        "Ostozhenka 70",
        "Ostozhenka 80",
        "Ostozhenka 90",
        "Ostozhenka 100",
        "Kremlin",
        "Manezh",
        "Tverskaya ul 10",
        "Tverskaya ul 30",
        "Tverskaya ul 40",
        "Tverskaya ul 70",
        "Tverskaya ul 80",
        "Tverskaya ul 90",
        "Tverskaya ul 100",
        "Belorusskiy vokzal",
        "Sokol",
        "Leningradskoye sh 10",
        "Leningradskoye sh 40",
        "Leningradskoye sh 50",
        "Leningradskoye sh 60",
        "Leningradskoye sh 70",
        "Leningradskoye sh 80",
        "Leningradskoye sh 90",
        "Leningradskoye sh 100",
        "Khimki",
        "Sheremetyevo"
      ]
    }
  ],
  "stat_requests": [
    {
      "type": "Route",
      "id": 1607592307,
      "to": "Leningradskiy pr 80",
      "from": "Komsomolskiy pr 20"
    },
    {
      "type": "Route",
      "id": 1918086003,
      "to": "Komsomolskiy pr 80",
      "from": "Tverskaya ul 20"
    },
    {
      "type": "Route",
      "id": 1705878213,
      "to": "Mezhdunarodnoye sh 100",
      "from": "Leningradskoye sh 10"
    },
    {
      "type": "Route",
      "id": 1032258749,
      "to": "Leningradskiy pr 60",
      "from": "Komsomolskiy pr 40"
    },
    {
      "type": "Route",
      "id": 862750239,
      "to": "Ostozhenka 20",
      "from": "Leningradskoye sh 20"
    },
    {
      "type": "Route",
      "id": 1465104363,
      "to": "Pr Vernadskogo 20",
      "from": "Tverskaya ul 60"
    },
    {
      "type": "Route",
      "id": 8645483,
      "to": "Manezh",
      "from": "Leningradskiy pr 20"
    },
    {
      "type": "Stop",
      "id": 1695724973,
      "name": "Pr Vernadskogo 90"
    },
    {
      "type": "Route",
      "id": 1513592365,
      "to": "Kiyevskoye sh 40",
      "from": "Ostozhenka 60"
    },
    {
      "type": "Route",
      "id": 141699407,
      "to": "Kremlin",
      "from": "Tverskaya ul 70"
    },
    {
      "type": "Route",
      "id": 220233995,
      "to": "Mokhovaya ul 40",
      "from": "Mokhovaya ul 20"
    },
    {
      "type": "Route",
      "id": 875126660,
      "to": "Ostozhenka 100",
      "from": "Pr Vernadskogo 10"
    },
    {
      "type": "Route",
      "id": 1054087408,
      "to": "Mokhovaya ul 80",
      "from": "Mokhovaya ul 10"
    },
    {
      "type": "Bus",
      "id": 1529735027,
      "name": "50"
    },
    {
      "type": "Bus",
      "id": 884992097,
      "name": "81"
    },
    {
      "type": "Bus",
      "id": 2102040328,
      "name": "67"
    },
    {
      "type": "Route",
      "id": 1107201585,
      "to": "Leningradskiy pr 80",
      "from": "Pr Vernadskogo 50"
    },
    {
      "type": "Stop",
      "id": 1321456988,
      "name": "Mezhdunarodnoye sh 100"
    },
    {
      "type": "Route",
      "id": 1054310039,
      "to": "Pr Vernadskogo 60",
      "from": "Komsomolskiy pr 80"
    },
    {
      "type": "Route",
      "id": 2031874307,
      "to": "Komsomolskiy pr 70",
      "from": "Leningradskoye sh 80"
    },
    {
      "type": "Route",
      "id": 1732222142,
      "to": "Manezh",
      "from": "Ostozhenka 70"
    },
    {
      "type": "Bus",
      "id": 819430422,
      "name": "19k"
    },
    {
      "type": "Route",
      "id": 1355593539,
      "to": "Mezhdunarodnoye sh 20",
      "from": "Mokhovaya ul 80"
    },
    {
      "type": "Route",
      "id": 1583266593,
      "to": "Ostozhenka 90",
      "from": "Kiyevskoye sh 20"
    },
    {
      "type": "Stop",
      "id": 341956560,
      "name": "Leningradskoye sh 50"
    },
    {
      "type": "Route",
      "id": 2068492255,
      "to": "Mokhovaya ul 20",
      "from": "Tverskaya ul 50"
    },
    {
      "type": "Stop",
      "id": 1273991077,
      "name": "Kiyevskoye sh 10"
    },
    {
      "type": "Route",
      "id": 1217157226,
      "to": "Leningradskoye sh 40",
      "from": "Tverskaya ul 30"
    },
    {
      "type": "Bus",
      "id": 590459346,
      "name": "43k"
    },
    {
      "type": "Route",
      "id": 1429827679,
      "to": "Manezh",
      "from": "Komsomolskiy pr 100"
    },
    {
      "type": "Stop",
      "id": 1491490499,
      "name": "Belorusskiy vokzal"
    },
    {
      "type": "Route",
      "id": 1919890905,
      "to": "Kiyevskoye sh 90",
      "from": "Ostozhenka 50"
    },
    {
      "type": "Stop",
      "id": 802437680,
      "name": "Kiyevskoye sh 20"
    },
    {
      "type": "Route",
      "id": 651169569,
      "to": "Komsomolskiy pr 60",
      "from": "Pr Vernadskogo 60"
    },
    {
      "type": "Route",
      "id": 1693772082,
      "to": "Tverskaya ul 50",
      "from": "Kiyevskoye sh 100"
    },
    {
      "type": "Bus",
      "id": 291021923,
      "name": "68k"
    },
    {
      "type": "Stop",
      "id": 524832460,
      "name": "Leningradskiy pr 30"
    },
    {
      "type": "Route",
      "id": 87416303,
      "to": "Ostozhenka 30",
      "from": "Kiyevskoye sh 10"
    },
    {
      "type": "Route",
      "id": 881531046,
      "to": "Leningradskiy pr 50",
      "from": "Ostozhenka 100"
    },
    {
      "type": "Bus",
      "id": 106969856,
      "name": "41"
    },
    {
      "type": "Stop",
      "id": 2050326951,
      "name": "Ostozhenka 10"
    },
    {
      "type": "Stop",
      "id": 285559190,
      "name": "Kiyevskoye sh 100"
    },
    {
      "type": "Route",
      "id": 439829207,
      "to": "Tverskaya ul 40",
      "from": "Leningradskoye sh 90"
    },
    {
      "type": "Bus",
      "id": 1500331975,
      "name": "100k"
    },
    {
      "type": "Bus",
      "id": 39536936,
      "name": "60"
    },
    {
      "type": "Stop",
      "id": 1879866889,
      "name": "Pr Vernadskogo 10"
    },
    {
      "type": "Route",
      "id": 92708127,
      "to": "Komsomolskiy pr 70",
      "from": "Pr Vernadskogo 100"
    },
    {
      "type": "Bus",
      "id": 1081123661,
      "name": "65k"
    },
    {
      "type": "Route",
      "id": 1246578769,
      "to": "Kiyevskoye sh 70",
      "from": "Luzhniki"
    },
    {
      "type": "Bus",
      "id": 1888137148,
      "name": "84k"
    },
    {
      "type": "Stop",
      "id": 1605701212,
      "name": "Komsomolskiy pr 10"
    },
    {
      "type": "Route",
      "id": 2030586329,
      "to": "Pr Vernadskogo 10",
      "from": "Vnukovo"
    },
    {
      "type": "Bus",
      "id": 1862530785,
      "name": "12"
    },
    {
      "type": "Route",
      "id": 1092075187,
      "to": "Ostozhenka 70",
      "from": "Pr Vernadskogo 50"
    },
    {
      "type": "Route",
      "id": 181832133,
      "to": "Belorusskiy vokzal",
      "from": "Ostozhenka 30"
    },
    {
      "type": "Route",
      "id": 2069182629,
      "to": "Leningradskiy pr 80",
      "from": "Pr Vernadskogo 90"
    },
    {
      "type": "Bus",
      "id": 231005000,
      "name": "90k"
    },
    {
      "type": "Stop",
      "id": 1321456988,
      "name": "Mezhdunarodnoye sh 100"
    },
    {
      "type": "Bus",
      "id": 1952512570,
      "name": "62"
    },
    {
      "type": "Route",
      "id": 2031174840,
      "to": "Komsomolskiy pr 100",
      "from": "Mokhovaya ul 30"
    },
    {
      "type": "Route",
      "id": 587491449,
      "to": "Leningradskiy pr 50",
      "from": "Khimki"
    },
    {
      "type": "Route",
      "id": 335032881,
      "to": "Ostozhenka 80",
      "from": "Kiyevskoye sh 60"
    },
    {
      "type": "Route",
      "id": 707243465,
      "to": "Troparyovo",
      "from": "Mokhovaya ul 70"
    },
    {
      "type": "Route",
      "id": 947490979,
      "to": "Tverskaya ul 70",
      "from": "Komsomolskiy pr 30"
    },
    {
      "type": "Bus",
      "id": 1184095094,
      "name": "64k"
    },
    {
      "type": "Route",
      "id": 1968457456,
      "to": "Leningradskiy pr 60",
      "from": "Tverskaya ul 80"
    },
    {
      "type": "Route",
      "id": 161043286,
      "to": "Ostozhenka 80",
      "from": "Mokhovaya ul 10"
    },
    {
      "type": "Route",
      "id": 1442399506,
      "to": "Kremlin",
      "from": "Leningradskoye sh 40"
    },
    {
      "type": "Route",
      "id": 134455258,
      "to": "Leningradskoye sh 100",
      "from": "Kiyevskoye sh 80"
    },
    {
      "type": "Route",
      "id": 247768132,
      "to": "Pr Vernadskogo 30",
      "from": "Kiyevskoye sh 60"
    },
    {
      "type": "Bus",
      "id": 2117221344,
      "name": "1sayfjRVzvkgsqDco"
    },
    {
      "type": "Route",
      "id": 339137699,
      "to": "Pr Vernadskogo 40",
      "from": "Pr Vernadskogo 90"
    },
    {
      "type": "Bus",
      "id": 54397640,
      "name": "78k"
    },
    {
      "type": "Route",
      "id": 510690410,
      "to": "Pr Vernadskogo 60",
      "from": "Manezh"
    },
    {
      "type": "Route",
      "id": 62560639,
      "to": "Pr Vernadskogo 60",
      "from": "Leningradskiy pr 10"
    },
    {
      "type": "Route",
      "id": 1908565471,
      "to": "Tverskaya ul 80",
      "from": "Leningradskiy pr 50"
    },
    {
      "type": "Bus",
      "id": 1028047723,
      "name": "20"
    },
    {
      "type": "Bus",
      "id": 1858698038,
      "name": "XhigObwaGQAR5pwd"
    },
    {
      "type": "Route",
      "id": 223231983,
      "to": "Pr Vernadskogo 90",
      "from": "Leningradskoye sh 100"
    },
    {
      "type": "Bus",
      "id": 1391108907,
      "name": "4"
    },
    {
      "type": "Bus",
      "id": 1221092998,
      "name": "99k"
    },
    {
      "type": "Route",
      "id": 1859967130,
      "to": "Leningradskoye sh 70",
      "from": "Mezhdunarodnoye sh 60"
    },
    {
      "type": "Stop",
      "id": 1945272220,
      "name": "dGkut9AZ15Y"
    },
    {
      "type": "Stop",
      "id": 2095250019,
      "name": "Tverskaya ul 50"
    },
    {
      "type": "Route",
      "id": 2043999979,
      "to": "Pr Vernadskogo 50",
      "from": "Belorusskiy vokzal"
    },
    {
      "type": "Route",
      "id": 1628005533,
      "to": "Mezhdunarodnoye sh 10",
      "from": "Tverskaya ul 70"
    },
    {
      "type": "Route",
      "id": 1529886021,
      "to": "Tverskaya ul 100",
      "from": "Sokol"
    },
    {
      "type": "Bus",
      "id": 1183335362,
      "name": "87k"
    },
    {
      "type": "Bus",
      "id": 1148160947,
      "name": "57k"
    },
    {
      "type": "Route",
      "id": 1952819726,
      "to": "Kiyevskoye sh 10",
      "from": "Ostozhenka 90"
    },
    {
      "type": "Bus",
      "id": 54397640,
      "name": "78k"
    },
    {
      "type": "Bus",
      "id": 1301480814,
      "name": "9k"
    },
    {
      "type": "Bus",
      "id": 625048715,
      "name": "85k"
    },
    {
      "type": "Route",
      "id": 1923787123,
      "to": "Leningradskiy pr 10",
      "from": "Komsomolskiy pr 10"
    },
    {
      "type": "Stop",
      "id": 463018877,
      "name": "twXJnOM xRWmUPyG"
    },
    {
      "type": "Bus",
      "id": 1057638203,
      "name": "wYIw Re9xb0noUKJj95EVS5"
    },
    {
      "type": "Route",
      "id": 1172290271,
      "to": "Mokhovaya ul 70",
      "from": "Mezhdunarodnoye sh 70"
    },
    {
      "type": "Stop",
      "id": 1071355180,
      "name": "Pr Vernadskogo 20"
    },
    {
      "type": "Route",
      "id": 669228348,
      "to": "Ostozhenka 20",
      "from": "Komsomolskiy pr 50"
    },
    {
      "type": "Route",
      "id": 29031172,
      "to": "Leningradskoye sh 10",
      "from": "Pr Vernadskogo 60"
    },
    {
      "type": "Route",
      "id": 587424192,
      "to": "Mokhovaya ul 90",
      "from": "Pr Vernadskogo 70"
    },
    {
      "type": "Route",
      "id": 484466453,
      "to": "Komsomolskiy pr 70",
      "from": "Mokhovaya ul 80"
    },
    {
      "type": "Route",
      "id": 532579776,
      "to": "Leningradskiy pr 40",
      "from": "Pr Vernadskogo 50"
    },
    {
      "type": "Stop",
      "id": 1215181049,
      "name": "Tverskaya ul 40"
    },
    {
      "type": "Route",
      "id": 1886985347,
      "to": "Ostozhenka 60",
      "from": "Komsomolskiy pr 50"
    },
    {
      "type": "Bus",
      "id": 1320378198,
      "name": "32"
    },
    {
      "type": "Route",
      "id": 1908301590,
      "to": "Ostozhenka 10",
      "from": "Leningradskiy pr 20"
    },
    {
      "type": "Stop",
      "id": 331121298,
      "name": "Mokhovaya ul 20"
    },
    {
      "type": "Stop",
      "id": 1748756235,
      "name": "Komsomolskiy pr 100"
    },
    {
      "type": "Route",
      "id": 1224988054,
      "to": "Manezh",
      "from": "Kiyevskoye sh 70"
    },
    {
      "type": "Route",
      "id": 677718707,
      "to": "Troparyovo",
      "from": "Tverskaya ul 40"
    },
    {
      "type": "Route",
      "id": 470944509,
      "to": "Vnukovo",
      "from": "Belorusskiy vokzal"
    },
    {
      "type": "Route",
      "id": 1516474884,
      "to": "Tverskaya ul 40",
      "from": "Tverskaya ul 60"
    },
    {
      "type": "Bus",
      "id": 884992097,
      "name": "81"
    },
    {
      "type": "Route",
      "id": 850990014,
      "to": "Yandex",
      "from": "Kiyevskoye sh 70"
    },
    {
      "type": "Route",
      "id": 1920216273,
      "to": "Mezhdunarodnoye sh 10",
      "from": "Kiyevskoye sh 40"
    },
    {
      "type": "Bus",
      "id": 1532241955,
      "name": "7k"
    },
    {
      "type": "Stop",
      "id": 1071355180,
      "name": "Pr Vernadskogo 20"
    },
    {
      "type": "Route",
      "id": 1394960544,
      "to": "Komsomolskiy pr 70",
      "from": "Mezhdunarodnoye sh 60"
    },
    {
      "type": "Bus",
      "id": 1028047723,
      "name": "20"
    },
    {
      "type": "Stop",
      "id": 1396785474,
      "name": "Ostozhenka 40"
    },
    {
      "type": "Route",
      "id": 1455103859,
      "to": "Kiyevskoye sh 70",
      "from": "Mokhovaya ul 100"
    },
    {
      "type": "Stop",
      "id": 978815997,
      "name": "Mokhovaya ul 30"
    },
    {
      "type": "Bus",
      "id": 1221092998,
      "name": "99k"
    },
    {
      "type": "Route",
      "id": 1308286286,
      "to": "Troparyovo",
      "from": "Mezhdunarodnoye sh 30"
    },
    {
      "type": "Bus",
      "id": 1679261573,
      "name": "74k"
    },
    {
      "type": "Route",
      "id": 1629572506,
      "to": "Leningradskoye sh 60",
      "from": "Leningradskiy pr 40"
    },
    {
      "type": "Bus",
      "id": 2085998310,
      "name": "29k"
    },
    {
      "type": "Route",
      "id": 168754802,
      "to": "Leningradskiy pr 70",
      "from": "Ostozhenka 80"
    },
    {
      "type": "Bus",
      "id": 243636714,
      "name": "39k"
    },
    {
      "type": "Bus",
      "id": 2122883674,
      "name": "lF"
    },
    {
      "type": "Bus",
      "id": 243636714,
      "name": "39k"
    },
    {
      "type": "Stop",
      "id": 570491200,
      "name": "Leningradskoye sh 90"
    },
    {
      "type": "Bus",
      "id": 819430422,
      "name": "19k"
    },
    {
      "type": "Bus",
      "id": 557108693,
      "name": "61k"
    },
    {
      "type": "Stop",
      "id": 331121298,
      "name": "Mokhovaya ul 20"
    },
    {
      "type": "Route",
      "id": 847724057,
      "to": "Leningradskoye sh 80",
      "from": "Kiyevskoye sh 80"
    },
    {
      "type": "Bus",
      "id": 2086679848,
      "name": "73k"
    },
    {
      "type": "Route",
      "id": 796352811,
      "to": "Leningradskoye sh 40",
      "from": "Khimki"
    },
    {
      "type": "Route",
      "id": 1472692962,
      "to": "Ostozhenka 20",
      "from": "Kiyevskoye sh 20"
    },
    {
      "type": "Route",
      "id": 483170960,
      "to": "Luzhniki",
      "from": "Pr Vernadskogo 60"
    },
    {
      "type": "Stop",
      "id": 1278313517,
      "name": "h76yENQ9Dz0"
    },
    {
      "type": "Route",
      "id": 1517753101,
      "to": "Sokol",
      "from": "Leningradskiy pr 20"
    },
    {
      "type": "Route",
      "id": 1039031391,
      "to": "Ostozhenka 20",
      "from": "Leningradskoye sh 30"
    },
    {
      "type": "Route",
      "id": 220631773,
      "to": "Ostozhenka 50",
      "from": "Kiyevskoye sh 20"
    },
    {
      "type": "Stop",
      "id": 335501637,
      "name": "u2i"
    },
    {
      "type": "Route",
      "id": 1478459814,
      "to": "Leningradskiy pr 60",
      "from": "Tverskaya ul 40"
    },
    {
      "type": "Bus",
      "id": 1028047723,
      "name": "20"
    },
    {
      "type": "Route",
      "id": 117546750,
      "to": "Leningradskiy pr 40",
      "from": "Komsomolskiy pr 20"
    },
    {
      "type": "Stop",
      "id": 132064624,
      "name": "Komsomolskiy pr 50"
    },
    {
      "type": "Route",
      "id": 447331558,
      "to": "Leningradskoye sh 40",
      "from": "Tverskaya ul 80"
    },
    {
      "type": "Route",
      "id": 1742918735,
      "to": "Kiyevskoye sh 30",
      "from": "Pr Vernadskogo 60"
    },
    {
      "type": "Route",
      "id": 2108676344,
      "to": "Sheremetyevo",
      "from": "Komsomolskiy pr 60"
    },
    {
      "type": "Route",
      "id": 642211950,
      "to": "Kiyevskoye sh 80",
      "from": "Tverskaya ul 70"
    },
    {
      "type": "Route",
      "id": 723406951,
      "to": "Leningradskoye sh 40",
      "from": "Komsomolskiy pr 30"
    },
    {
      "type": "Bus",
      "id": 548886,
      "name": "91"
    },
    {
      "type": "Stop",
      "id": 1766990604,
      "name": "Leningradskiy pr 10"
    },
    {
      "type": "Bus",
      "id": 1500331975,
      "name": "100k"
    },
    {
      "type": "Bus",
      "id": 214763116,
      "name": "27k"
    },
    {
      "type": "Route",
      "id": 186505327,
      "to": "Kiyevskoye sh 80",
      "from": "Mezhdunarodnoye sh 20"
    },
    {
      "type": "Route",
      "id": 1826026596,
      "to": "Tverskaya ul 100",
      "from": "Mokhovaya ul 10"
    },
    {
      "type": "Route",
      "id": 1837842048,
      "to": "Leningradskoye sh 50",
      "from": "Kiyevskoye sh 30"
    },
    {
      "type": "Bus",
      "id": 1222823833,
      "name": "55k"
    },
    {
      "type": "Route",
      "id": 1522054261,
      "to": "Mezhdunarodnoye sh 90",
      "from": "Pr Vernadskogo 80"
    },
    {
      "type": "Stop",
      "id": 889685809,
      "name": "J4MgIuGg2bnsxJ78K"
    },
    {
      "type": "Route",
      "id": 1553836250,
      "to": "Khimki",
      "from": "Leningradskiy pr 70"
    },
    {
      "type": "Route",
      "id": 900302727,
      "to": "Leningradskoye sh 100",
      "from": "Mokhovaya ul 80"
    },
    {
      "type": "Bus",
      "id": 328219103,
      "name": "DMQPegPK9dnNfCLqf32cI5gJi"
    },
    {
      "type": "Route",
      "id": 1538872156,
      "to": "Leningradskiy pr 40",
      "from": "Komsomolskiy pr 10"
    },
    {
      "type": "Stop",
      "id": 1695724973,
      "name": "Pr Vernadskogo 90"
    },
    {
      "type": "Route",
      "id": 1670800962,
      "to": "Pr Vernadskogo 20",
      "from": "Tverskaya ul 30"
    },
    {
      "type": "Route",
      "id": 1237775266,
      "to": "Tverskaya ul 60",
      "from": "Komsomolskiy pr 80"
    },
    {
      "type": "Route",
      "id": 1878424184,
      "to": "Ostozhenka 20",
      "from": "Tverskaya ul 10"
    },
    {
      "type": "Route",
      "id": 1849188174,
      "to": "Leningradskoye sh 80",
      "from": "Kiyevskoye sh 20"
    },
    {
      "type": "Route",
      "id": 1807593798,
      "to": "Kiyevskoye sh 80",
      "from": "Mezhdunarodnoye sh 90"
    },
    {
      "type": "Route",
      "id": 722432766,
      "to": "Komsomolskiy pr 20",
      "from": "Pr Vernadskogo 90"
    },
    {
      "type": "Route",
      "id": 669407984,
      "to": "Tverskaya ul 80",
      "from": "Leningradskoye sh 20"
    },
    {
      "type": "Route",
      "id": 1613995387,
      "to": "Kiyevskoye sh 60",
      "from": "Pr Vernadskogo 40"
    },
    {
      "type": "Bus",
      "id": 557108693,
      "name": "61k"
    },
    {
      "type": "Bus",
      "id": 1529735027,
      "name": "50"
    },
    {
      "type": "Bus",
      "id": 995718152,
      "name": "tNDXfI Sm3X5hZSlCXry"
    },
    {
      "type": "Route",
      "id": 711125431,
      "to": "Ostozhenka 30",
      "from": "Komsomolskiy pr 60"
    },
    {
      "type": "Route",
      "id": 2031874307,
      "to": "Komsomolskiy pr 70",
      "from": "Leningradskoye sh 80"
    },
    {
      "type": "Route",
      "id": 1593512972,
      "to": "Ostozhenka 10",
      "from": "Tverskaya ul 60"
    },
    {
      "type": "Route",
      "id": 222363249,
      "to": "Vnukovo",
      "from": "Troparyovo"
    },
    {
      "type": "Stop",
      "id": 900561951,
      "name": "7DCVOh19x"
    },
    {
      "type": "Route",
      "id": 1270847989,
      "to": "Tverskaya ul 20",
      "from": "Tverskaya ul 100"
    },
    {
      "type": "Route",
      "id": 1832188393,
      "to": "Kiyevskoye sh 100",
      "from": "Tverskaya ul 50"
    },
    {
      "type": "Bus",
      "id": 106969856,
      "name": "41"
    },
    {
      "type": "Route",
      "id": 2004075791,
      "to": "Leningradskiy pr 40",
      "from": "Vnukovo"
    },
    {
      "type": "Bus",
      "id": 2102040328,
      "name": "67"
    },
    {
      "type": "Stop",
      "id": 1695724973,
      "name": "Pr Vernadskogo 90"
    },
    {
      "type": "Route",
      "id": 33562730,
      "to": "Tverskaya ul 80",
      "from": "Komsomolskiy pr 50"
    },
    {
      "type": "Route",
      "id": 1180843953,
      "to": "Leningradskoye sh 60",
      "from": "Kiyevskoye sh 20"
    },
    {
      "type": "Route",
      "id": 1480864644,
      "to": "Mokhovaya ul 70",
      "from": "Kiyevskoye sh 90"
    },
    {
      "type": "Bus",
      "id": 1679261573,
      "name": "74k"
    },
    {
      "type": "Route",
      "id": 1011616695,
      "to": "Sheremetyevo",
      "from": "Leningradskiy pr 40"
    },
    {
      "type": "Route",
      "id": 1197295348,
      "to": "Pr Vernadskogo 40",
      "from": "Leningradskoye sh 30"
    },
    {
      "type": "Route",
      "id": 434082681,
      "to": "Mokhovaya ul 100",
      "from": "Kiyevskoye sh 50"
    },
    {
      "type": "Route",
      "id": 1987500663,
      "to": "Mokhovaya ul 60",
      "from": "Komsomolskiy pr 70"
    },
    {
      "type": "Stop",
      "id": 1901744419,
      "name": "Leningradskoye sh 60"
    },
    {
      "type": "Bus",
      "id": 95205841,
      "name": "58"
    },
    {
      "type": "Route",
      "id": 335846665,
      "to": "Komsomolskiy pr 100",
      "from": "Mezhdunarodnoye sh 10"
    },
    {
      "type": "Route",
      "id": 1985940448,
      "to": "Pr Vernadskogo 50",
      "from": "Mezhdunarodnoye sh 30"
    },
    {
      "type": "Stop",
      "id": 1438491283,
      "name": "Yandex"
    },
    {
      "type": "Stop",
      "id": 1766990604,
      "name": "Leningradskiy pr 10"
    },
    {
      "type": "Route",
      "id": 1546407652,
      "to": "Mezhdunarodnoye sh 30",
      "from": "Mokhovaya ul 100"
    },
    {
      "type": "Stop",
      "id": 20361745,
      "name": "Fp3"
    },
    {
      "type": "Route",
      "id": 451242450,
      "to": "Kiyevskoye sh 90",
      "from": "Leningradskiy pr 70"
    },
    {
      "type": "Route",
      "id": 1707623395,
      "to": "Pr Vernadskogo 80",
      "from": "Pr Vernadskogo 70"
    },
    {
      "type": "Route",
      "id": 2115725309,
      "to": "Mezhdunarodnoye sh 10",
      "from": "Leningradskiy pr 100"
    },
    {
      "type": "Stop",
      "id": 1464743061,
      "name": "Mokhovaya ul 90"
    },
    {
      "type": "Route",
      "id": 1419401223,
      "to": "Pr Vernadskogo 50",
      "from": "Tverskaya ul 30"
    },
    {
      "type": "Route",
      "id": 2049509115,
      "to": "Komsomolskiy pr 10",
      "from": "Mokhovaya ul 80"
    },
    {
      "type": "Stop",
      "id": 1882423715,
      "name": "Kremlin"
    },
    {
      "type": "Route",
      "id": 2145528349,
      "to": "Tverskaya ul 70",
      "from": "Mezhdunarodnoye sh 100"
    },
    {
      "type": "Route",
      "id": 1088585216,
      "to": "Leningradskiy pr 10",
      "from": "Mokhovaya ul 50"
    },
    {
      "type": "Route",
      "id": 645552844,
      "to": "Mezhdunarodnoye sh 80",
      "from": "Ostozhenka 80"
    },
    {
      "type": "Route",
      "id": 136345038,
      "to": "Kremlin",
      "from": "Kiyevskoye sh 50"
    },
    {
      "type": "Route",
      "id": 200613354,
      "to": "Luzhniki",
      "from": "Leningradskoye sh 60"
    },
    {
      "type": "Stop",
      "id": 1695724973,
      "name": "Pr Vernadskogo 90"
    },
    {
      "type": "Route",
      "id": 640671042,
      "to": "Ostozhenka 60",
      "from": "Kiyevskoye sh 10"
    },
    {
      "type": "Stop",
      "id": 1462553819,
      "name": "MRW6b IainNoiJFo5aEYM9O"
    },
    {
      "type": "Route",
      "id": 730035118,
      "to": "Ostozhenka 80",
      "from": "Ostozhenka 30"
    },
    {
      "type": "Stop",
      "id": 1695724973,
      "name": "Pr Vernadskogo 90"
    },
    {
      "type": "Route",
      "id": 562618897,
      "to": "Mokhovaya ul 30",
      "from": "Leningradskiy pr 60"
    },
    {
      "type": "Bus",
      "id": 1994418887,
      "name": "49"
    },
    {
      "type": "Route",
      "id": 634588812,
      "to": "Mokhovaya ul 100",
      "from": "Mokhovaya ul 70"
    },
    {
      "type": "Route",
      "id": 1707166026,
      "to": "Tverskaya ul 80",
      "from": "Mezhdunarodnoye sh 30"
    },
    {
      "type": "Route",
      "id": 441322862,
      "to": "Mokhovaya ul 60",
      "from": "Mezhdunarodnoye sh 10"
    },
    {
      "type": "Route",
      "id": 1508032752,
      "to": "Troparyovo",
      "from": "Leningradskiy pr 90"
    },
    {
      "type": "Route",
      "id": 272548083,
      "to": "Leningradskiy pr 80",
      "from": "Luzhniki"
    },
    {
      "type": "Route",
      "id": 506835991,
      "to": "Ostozhenka 20",
      "from": "Sokol"
    },
    {
      "type": "Route",
      "id": 141538956,
      "to": "Pr Vernadskogo 60",
      "from": "Leningradskoye sh 30"
    },
    {
      "type": "Route",
      "id": 2052394587,
      "to": "Leningradskoye sh 20",
      "from": "Khimki"
    },
    {
      "type": "Route",
      "id": 1598565171,
      "to": "Mokhovaya ul 80",
      "from": "Leningradskiy pr 30"
    },
    {
      "type": "Bus",
      "id": 2086679848,
      "name": "73k"
    },
    {
      "type": "Stop",
      "id": 942518202,
      "name": "F kSE2 a5Ywibrw"
    },
    {
      "type": "Route",
      "id": 894489750,
      "to": "Tverskaya ul 10",
      "from": "Mezhdunarodnoye sh 70"
    },
    {
      "type": "Bus",
      "id": 361149198,
      "name": "ndlwesbc7y"
    },
    {
      "type": "Stop",
      "id": 312162747,
      "name": "Mezhdunarodnoye sh 40"
    },
    {
      "type": "Route",
      "id": 628932205,
      "to": "Pr Vernadskogo 20",
      "from": "Pr Vernadskogo 100"
    },
    {
      "type": "Route",
      "id": 1976474729,
      "to": "Leningradskoye sh 50",
      "from": "Tverskaya ul 50"
    },
    {
      "type": "Bus",
      "id": 291021923,
      "name": "68k"
    },
    {
      "type": "Route",
      "id": 206011669,
      "to": "Tverskaya ul 30",
      "from": "Leningradskoye sh 50"
    },
    {
      "type": "Bus",
      "id": 1320378198,
      "name": "32"
    },
    {
      "type": "Route",
      "id": 876284213,
      "to": "Mokhovaya ul 50",
      "from": "Ostozhenka 30"
    },
    {
      "type": "Stop",
      "id": 1791067641,
      "name": "Mokhovaya ul 40"
    },
    {
      "type": "Stop",
      "id": 306795514,
      "name": "Leningradskiy pr 70"
    },
    {
      "type": "Route",
      "id": 1469151298,
      "to": "Mezhdunarodnoye sh 70",
      "from": "Leningradskoye sh 100"
    },
    {
      "type": "Route",
      "id": 249294801,
      "to": "Mezhdunarodnoye sh 100",
      "from": "Leningradskoye sh 20"
    },
    {
      "type": "Route",
      "id": 159448217,
      "to": "Kremlin",
      "from": "Komsomolskiy pr 100"
    },
    {
      "type": "Route",
      "id": 527331814,
      "to": "Komsomolskiy pr 30",
      "from": "Mezhdunarodnoye sh 20"
    },
    {
      "type": "Route",
      "id": 48051212,
      "to": "Mokhovaya ul 90",
      "from": "Tverskaya ul 70"
    },
    {
      "type": "Route",
      "id": 789791172,
      "to": "Mezhdunarodnoye sh 50",
      "from": "Mokhovaya ul 40"
    },
    {
      "type": "Route",
      "id": 2007347305,
      "to": "Leningradskoye sh 30",
      "from": "Kiyevskoye sh 70"
    },
    {
      "type": "Route",
      "id": 1757351094,
      "to": "Komsomolskiy pr 100",
      "from": "Mokhovaya ul 60"
    },
    {
      "type": "Stop",
      "id": 1396785474,
      "name": "Ostozhenka 40"
    },
    {
      "type": "Bus",
      "id": 1496236952,
      "name": "0hA"
    },
    {
      "type": "Stop",
      "id": 765063867,
      "name": "f"
    },
    {
      "type": "Route",
      "id": 2028583616,
      "to": "Kremlin",
      "from": "Pr Vernadskogo 80"
    },
    {
      "type": "Bus",
      "id": 877779756,
      "name": "54"
    },
    {
      "type": "Bus",
      "id": 549153993,
      "name": "37k"
    },
    {
      "type": "Bus",
      "id": 54397640,
      "name": "78k"
    },
    {
      "type": "Route",
      "id": 390440441,
      "to": "Mezhdunarodnoye sh 40",
      "from": "Mokhovaya ul 50"
    },
    {
      "type": "Route",
      "id": 265679047,
      "to": "Belorusskiy vokzal",
      "from": "Mokhovaya ul 50"
    },
    {
      "type": "Bus",
      "id": 2064455607,
      "name": "76k"
    },
    {
      "type": "Route",
      "id": 1363718714,
      "to": "Mokhovaya ul 90",
      "from": "Tverskaya ul 50"
    },
    {
      "type": "Stop",
      "id": 721281380,
      "name": "Leningradskiy pr 50"
    },
    {
      "type": "Stop",
      "id": 1034325494,
      "name": "Mezhdunarodnoye sh 10"
    },
    {
      "type": "Stop",
      "id": 1834107789,
      "name": "Tverskaya ul 100"
    },
    {
      "type": "Route",
      "id": 2109769776,
      "to": "Komsomolskiy pr 70",
      "from": "Komsomolskiy pr 50"
    },
    {
      "type": "Stop",
      "id": 1242118002,
      "name": "Sokol"
    },
    {
      "type": "Bus",
      "id": 1679261573,
      "name": "74k"
    },
    {
      "type": "Route",
      "id": 936111601,
      "to": "Leningradskiy pr 100",
      "from": "Kiyevskoye sh 70"
    },
    {
      "type": "Stop",
      "id": 1166353463,
      "name": "mPIf7dVwF53KOVSI9R4MMg"
    },
    {
      "type": "Stop",
      "id": 1692384196,
      "name": "Mezhdunarodnoye sh 80"
    },
    {
      "type": "Stop",
      "id": 1839841385,
      "name": "Tverskaya ul 20"
    },
    {
      "type": "Stop",
      "id": 1603176856,
      "name": "Leningradskoye sh 80"
    },
    {
      "type": "Stop",
      "id": 2017557436,
      "name": "Komsomolskiy pr 90"
    },
    {
      "type": "Route",
      "id": 1428136225,
      "to": "Leningradskoye sh 50",
      "from": "Leningradskoye sh 50"
    },
    {
      "type": "Bus",
      "id": 903337307,
      "name": "56k"
    },
    {
      "type": "Stop",
      "id": 440933152,
      "name": "Leningradskoye sh 20"
    },
    {
      "type": "Stop",
      "id": 1971821751,
      "name": "Komsomolskiy pr 30"
    },
    {
      "type": "Route",
      "id": 409199091,
      "to": "Pr Vernadskogo 10",
      "from": "Mezhdunarodnoye sh 100"
    },
    {
      "type": "Stop",
      "id": 699793610,
      "name": "Sheremetyevo"
    },
    {
      "type": "Bus",
      "id": 291021923,
      "name": "68k"
    },
    {
      "type": "Route",
      "id": 323931917,
      "to": "Komsomolskiy pr 50",
      "from": "Yandex"
    },
    {
      "type": "Route",
      "id": 2116316696,
      "to": "Leningradskiy pr 70",
      "from": "Belorusskiy vokzal"
    },
    {
      "type": "Route",
      "id": 2000214623,
      "to": "Mokhovaya ul 70",
      "from": "Leningradskoye sh 10"
    },
    {
      "type": "Route",
      "id": 411345156,
      "to": "Komsomolskiy pr 90",
      "from": "Sheremetyevo"
    },
    {
      "type": "Stop",
      "id": 1847557780,
      "name": "Mezhdunarodnoye sh 30"
    },
    {
      "type": "Route",
      "id": 752353680,
      "to": "Kremlin",
      "from": "Ostozhenka 70"
    },
    {
      "type": "Route",
      "id": 1389705516,
      "to": "Komsomolskiy pr 70",
      "from": "Kiyevskoye sh 10"
    },
    {
      "type": "Route",
      "id": 1105880470,
      "to": "Komsomolskiy pr 80",
      "from": "Tverskaya ul 30"
    },
    {
      "type": "Route",
      "id": 1493114441,
      "to": "Belorusskiy vokzal",
      "from": "Leningradskoye sh 30"
    },
    {
      "type": "Route",
      "id": 1209721409,
      "to": "Mokhovaya ul 100",
      "from": "Leningradskoye sh 40"
    },
    {
      "type": "Stop",
      "id": 833334897,
      "name": "Mezhdunarodnoye sh 90"
    },
    {
      "type": "Route",
      "id": 1596046930,
      "to": "Kiyevskoye sh 40",
      "from": "Tverskaya ul 70"
    },
    {
      "type": "Route",
      "id": 1886066110,
      "to": "Pr Vernadskogo 50",
      "from": "Vnukovo"
    },
    {
      "type": "Route",
      "id": 1438441150,
      "to": "Kiyevskoye sh 80",
      "from": "Tverskaya ul 100"
    },
    {
      "type": "Bus",
      "id": 877779756,
      "name": "54"
    },
    {
      "type": "Route",
      "id": 436491761,
      "to": "Komsomolskiy pr 40",
      "from": "Pr Vernadskogo 100"
    },
    {
      "type": "Bus",
      "id": 589555403,
      "name": "47k"
    },
    {
      "type": "Route",
      "id": 1823262951,
      "to": "Leningradskoye sh 100",
      "from": "Leningradskiy pr 100"
    },
    {
      "type": "Route",
      "id": 1258053733,
      "to": "Mokhovaya ul 20",
      "from": "Mezhdunarodnoye sh 100"
    },
    {
      "type": "Route",
      "id": 295657195,
      "to": "Leningradskiy pr 50",
      "from": "Leningradskoye sh 30"
    },
    {
      "type": "Stop",
      "id": 132064624,
      "name": "Komsomolskiy pr 50"
    },
    {
      "type": "Bus",
      "id": 1459482213,
      "name": "42"
    },
    {
      "type": "Route",
      "id": 855962237,
      "to": "Khimki",
      "from": "Mezhdunarodnoye sh 70"
    },
    {
      "type": "Route",
      "id": 1221384617,
      "to": "Tverskaya ul 50",
      "from": "Pr Vernadskogo 80"
    },
    {
      "type": "Route",
      "id": 1987626820,
      "to": "Belorusskiy vokzal",
      "from": "Leningradskoye sh 70"
    },
    {
      "type": "Bus",
      "id": 2104567867,
      "name": "28"
    },
    {
      "type": "Route",
      "id": 1174899328,
      "to": "Ostozhenka 20",
      "from": "Leningradskiy pr 20"
    },
    {
      "type": "Stop",
      "id": 1249144660,
      "name": "EzyGwmHDxKahUE9wlYkbpMO"
    },
    {
      "type": "Route",
      "id": 1231841078,
      "to": "Ostozhenka 90",
      "from": "Vnukovo"
    },
    {
      "type": "Route",
      "id": 1683922973,
      "to": "Leningradskoye sh 60",
      "from": "Komsomolskiy pr 100"
    },
    {
      "type": "Stop",
      "id": 1901744419,
      "name": "Leningradskoye sh 60"
    },
    {
      "type": "Route",
      "id": 627970370,
      "to": "Mezhdunarodnoye sh 90",
      "from": "Leningradskiy pr 60"
    },
    {
      "type": "Bus",
      "id": 1960392737,
      "name": "13"
    },
    {
      "type": "Bus",
      "id": 1472826063,
      "name": "98"
    },
    {
      "type": "Stop",
      "id": 1797256328,
      "name": "Pr Vernadskogo 100"
    },
    {
      "type": "Route",
      "id": 452517815,
      "to": "Leningradskiy pr 60",
      "from": "Tverskaya ul 60"
    },
    {
      "type": "Route",
      "id": 1504311232,
      "to": "Tverskaya ul 60",
      "from": "Mezhdunarodnoye sh 30"
    },
    {
      "type": "Route",
      "id": 464841003,
      "to": "Ostozhenka 70",
      "from": "Khimki"
    },
    {
      "type": "Route",
      "id": 1328563016,
      "to": "Leningradskiy pr 70",
      "from": "Kiyevskoye sh 70"
    },
    {
      "type": "Route",
      "id": 614862338,
      "to": "Kiyevskoye sh 30",
      "from": "Pr Vernadskogo 80"
    },
    {
      "type": "Stop",
      "id": 260277259,
      "name": "Tverskaya ul 90"
    },
    {
      "type": "Route",
      "id": 1859411760,
      "to": "Leningradskoye sh 70",
      "from": "Mokhovaya ul 100"
    },
    {
      "type": "Route",
      "id": 1974729716,
      "to": "Tverskaya ul 50",
      "from": "Leningradskiy pr 20"
    },
    {
      "type": "Route",
      "id": 1320138317,
      "to": "Kiyevskoye sh 50",
      "from": "Mezhdunarodnoye sh 20"
    },
    {
      "type": "Route",
      "id": 1607000115,
      "to": "Pr Vernadskogo 20",
      "from": "Kiyevskoye sh 60"
    },
    {
      "type": "Route",
      "id": 709688600,
      "to": "Kiyevskoye sh 60",
      "from": "Tverskaya ul 50"
    },
    {
      "type": "Bus",
      "id": 640386051,
      "name": "14"
    },
    {
      "type": "Route",
      "id": 1535409304,
      "to": "Pr Vernadskogo 70",
      "from": "Komsomolskiy pr 10"
    },
    {
      "type": "Stop",
      "id": 1495910205,
      "name": "Mezhdunarodnoye sh 70"
    },
    {
      "type": "Route",
      "id": 1438441150,
      "to": "Kiyevskoye sh 80",
      "from": "Tverskaya ul 100"
    },
    {
      "type": "Route",
      "id": 309662858,
      "to": "Sokol",
      "from": "Mezhdunarodnoye sh 70"
    },
    {
      "type": "Route",
      "id": 1089029688,
      "to": "Leningradskoye sh 30",
      "from": "Leningradskoye sh 20"
    },
    {
      "type": "Route",
      "id": 770141494,
      "to": "Tverskaya ul 80",
      "from": "Kiyevskoye sh 100"
    },
    {
      "type": "Route",
      "id": 1995045570,
      "to": "Leningradskiy pr 30",
      "from": "Kiyevskoye sh 20"
    },
    {
      "type": "Bus",
      "id": 1432394340,
      "name": "38k"
    },
    {
      "type": "Route",
      "id": 1569843456,
      "to": "Mezhdunarodnoye sh 70",
      "from": "Mokhovaya ul 50"
    },
    {
      "type": "Stop",
      "id": 1770268632,
      "name": "Ostozhenka 90"
    },
    {
      "type": "Route",
      "id": 240170207,
      "to": "Pr Vernadskogo 20",
      "from": "Ostozhenka 10"
    },
    {
      "type": "Route",
      "id": 856458027,
      "to": "Tverskaya ul 40",
      "from": "Komsomolskiy pr 50"
    },
    {
      "type": "Route",
      "id": 868322217,
      "to": "Ostozhenka 50",
      "from": "Leningradskoye sh 80"
    },
    {
      "type": "Route",
      "id": 1149949602,
      "to": "Ostozhenka 90",
      "from": "Mezhdunarodnoye sh 60"
    },
    {
      "type": "Route",
      "id": 1259470803,
      "to": "Mokhovaya ul 20",
      "from": "Mezhdunarodnoye sh 80"
    },
    {
      "type": "Stop",
      "id": 2077587727,
      "name": "Pr Vernadskogo 40"
    },
    {
      "type": "Route",
      "id": 1208305765,
      "to": "Mokhovaya ul 100",
      "from": "Kiyevskoye sh 90"
    },
    {
      "type": "Route",
      "id": 1931867777,
      "to": "Leningradskiy pr 100",
      "from": "Sheremetyevo"
    },
    {
      "type": "Route",
      "id": 1048821539,
      "to": "Mokhovaya ul 80",
      "from": "Pr Vernadskogo 40"
    },
    {
      "type": "Route",
      "id": 1034015685,
      "to": "Leningradskiy pr 40",
      "from": "Ostozhenka 80"
    },
    {
      "type": "Bus",
      "id": 54397640,
      "name": "78k"
    },
    {
      "type": "Route",
      "id": 96858075,
      "to": "Mezhdunarodnoye sh 20",
      "from": "Tverskaya ul 10"
    },
    {
      "type": "Route",
      "id": 1412095146,
      "to": "Kiyevskoye sh 80",
      "from": "Pr Vernadskogo 10"
    },
    {
      "type": "Route",
      "id": 542626469,
      "to": "Ostozhenka 80",
      "from": "Ostozhenka 60"
    },
    {
      "type": "Stop",
      "id": 1460982888,
      "name": "Pr Vernadskogo 80"
    },
    {
      "type": "Route",
      "id": 726965099,
      "to": "Komsomolskiy pr 30",
      "from": "Tverskaya ul 40"
    },
    {
      "type": "Stop",
      "id": 1396785474,
      "name": "Ostozhenka 40"
    },
    {
      "type": "Stop",
      "id": 1878414831,
      "name": "2lvg8YfgkaCJPHLVzWlv"
    },
    {
      "type": "Route",
      "id": 1715794281,
      "to": "Ostozhenka 70",
      "from": "Tverskaya ul 10"
    },
    {
      "type": "Route",
      "id": 1631148964,
      "to": "Leningradskiy pr 50",
      "from": "Yandex"
    },
    {
      "type": "Route",
      "id": 526522666,
      "to": "Mokhovaya ul 40",
      "from": "Mokhovaya ul 30"
    },
    {
      "type": "Route",
      "id": 1741512299,
      "to": "Manezh",
      "from": "Ostozhenka 20"
    },
    {
      "type": "Bus",
      "id": 1028047723,
      "name": "20"
    },
    {
      "type": "Stop",
      "id": 1603176856,
      "name": "Leningradskoye sh 80"
    },
    {
      "type": "Route",
      "id": 1630104965,
      "to": "Kiyevskoye sh 80",
      "from": "Leningradskiy pr 60"
    },
    {
      "type": "Route",
      "id": 349012442,
      "to": "Yandex",
      "from": "Komsomolskiy pr 40"
    },
    {
      "type": "Route",
      "id": 492804644,
      "to": "Tverskaya ul 80",
      "from": "Leningradskoye sh 100"
    },
    {
      "type": "Stop",
      "id": 1879866889,
      "name": "Pr Vernadskogo 10"
    },
    {
      "type": "Route",
      "id": 1964072666,
      "to": "Ostozhenka 20",
      "from": "Leningradskiy pr 30"
    },
    {
      "type": "Route",
      "id": 1323126858,
      "to": "Komsomolskiy pr 20",
      "from": "Leningradskoye sh 40"
    },
    {
      "type": "Stop",
      "id": 1603176856,
      "name": "Leningradskoye sh 80"
    },
    {
      "type": "Route",
      "id": 1610877290,
      "to": "Kiyevskoye sh 30",
      "from": "Mezhdunarodnoye sh 80"
    },
    {
      "type": "Route",
      "id": 596323985,
      "to": "Mokhovaya ul 30",
      "from": "Tverskaya ul 70"
    },
    {
      "type": "Bus",
      "id": 589555403,
      "name": "47k"
    },
    {
      "type": "Route",
      "id": 2070220577,
      "to": "Komsomolskiy pr 20",
      "from": "Leningradskoye sh 20"
    },
    {
      "type": "Stop",
      "id": 1494048957,
      "name": "cWhYNb7z"
    },
    {
      "type": "Route",
      "id": 492804644,
      "to": "Tverskaya ul 80",
      "from": "Leningradskoye sh 100"
    },
    {
      "type": "Route",
      "id": 1266357999,
      "to": "Mezhdunarodnoye sh 90",
      "from": "Komsomolskiy pr 90"
    },
    {
      "type": "Route",
      "id": 22590122,
      "to": "Leningradskoye sh 20",
      "from": "Tverskaya ul 60"
    },
    {
      "type": "Route",
      "id": 2085323124,
      "to": "Komsomolskiy pr 80",
      "from": "Mokhovaya ul 70"
    },
    {
      "type": "Bus",
      "id": 1260274581,
      "name": "40k"
    },
    {
      "type": "Route",
      "id": 522793698,
      "to": "Leningradskoye sh 70",
      "from": "Sheremetyevo"
    },
    {
      "type": "Route",
      "id": 361837950,
      "to": "Ostozhenka 80",
      "from": "Ostozhenka 40"
    },
    {
      "type": "Stop",
      "id": 1770268632,
      "name": "Ostozhenka 90"
    },
    {
      "type": "Bus",
      "id": 1074291017,
      "name": "EIY1mq6wy2Zu63I2eb"
    },
    {
      "type": "Stop",
      "id": 978815997,
      "name": "Mokhovaya ul 30"
    },
    {
      "type": "Route",
      "id": 1374183314,
      "to": "Ostozhenka 90",
      "from": "Mezhdunarodnoye sh 30"
    },
    {
      "type": "Route",
      "id": 868322217,
      "to": "Ostozhenka 50",
      "from": "Leningradskoye sh 80"
    },
    {
      "type": "Route",
      "id": 761004543,
      "to": "Pr Vernadskogo 100",
      "from": "Komsomolskiy pr 70"
    },
    {
      "type": "Route",
      "id": 321914567,
      "to": "Mezhdunarodnoye sh 10",
      "from": "Belorusskiy vokzal"
    },
    {
      "type": "Route",
      "id": 904484759,
      "to": "Pr Vernadskogo 100",
      "from": "Kiyevskoye sh 100"
    },
    {
      "type": "Bus",
      "id": 1500331975,
      "name": "100k"
    },
    {
      "type": "Stop",
      "id": 1460982888,
      "name": "Pr Vernadskogo 80"
    },
    {
      "type": "Stop",
      "id": 2059621403,
      "name": "Kiyevskoye sh 30"
    },
    {
      "type": "Bus",
      "id": 640386051,
      "name": "14"
    },
    {
      "type": "Bus",
      "id": 1952512570,
      "name": "62"
    },
    {
      "type": "Bus",
      "id": 652318140,
      "name": "92k"
    },
    {
      "type": "Stop",
      "id": 1321456988,
      "name": "Mezhdunarodnoye sh 100"
    },
    {
      "type": "Bus",
      "id": 2064455607,
      "name": "76k"
    },
    {
      "type": "Route",
      "id": 2113585676,
      "to": "Troparyovo",
      "from": "Sheremetyevo"
    },
    {
      "type": "Route",
      "id": 1778447876,
      "to": "Mokhovaya ul 40",
      "from": "Ostozhenka 20"
    },
    {
      "type": "Route",
      "id": 1134899832,
      "to": "Tverskaya ul 10",
      "from": "Leningradskoye sh 30"
    },
    {
      "type": "Route",
      "id": 607005622,
      "to": "Kiyevskoye sh 20",
      "from": "Mezhdunarodnoye sh 10"
    },
    {
      "type": "Stop",
      "id": 1491490499,
      "name": "Belorusskiy vokzal"
    },
    {
      "type": "Route",
      "id": 530501261,
      "to": "Mezhdunarodnoye sh 50",
      "from": "Komsomolskiy pr 40"
    },
    {
      "type": "Bus",
      "id": 1000797031,
      "name": "35"
    },
    {
      "type": "Stop",
      "id": 1242118002,
      "name": "Sokol"
    },
    {
      "type": "Stop",
      "id": 341956560,
      "name": "Leningradskoye sh 50"
    },
    {
      "type": "Bus",
      "id": 1459482213,
      "name": "42"
    },
    {
      "type": "Route",
      "id": 485100209,
      "to": "Pr Vernadskogo 30",
      "from": "Mokhovaya ul 40"
    },
    {
      "type": "Route",
      "id": 483498483,
      "to": "Tverskaya ul 100",
      "from": "Kiyevskoye sh 60"
    },
    {
      "type": "Stop",
      "id": 227053825,
      "name": "M0lNzrm6FJ"
    },
    {
      "type": "Route",
      "id": 57950281,
      "to": "Leningradskoye sh 60",
      "from": "Leningradskiy pr 80"
    },
    {
      "type": "Stop",
      "id": 1680438344,
      "name": "Leningradskiy pr 90"
    },
    {
      "type": "Stop",
      "id": 101285333,
      "name": "qGlRhSsaCUm"
    },
    {
      "type": "Route",
      "id": 365329655,
      "to": "Pr Vernadskogo 100",
      "from": "Tverskaya ul 10"
    },
    {
      "type": "Bus",
      "id": 2086679848,
      "name": "73k"
    },
    {
      "type": "Route",
      "id": 1936209977,
      "to": "Leningradskoye sh 10",
      "from": "Ostozhenka 70"
    },
    {
      "type": "Bus",
      "id": 2102433046,
      "name": "21k"
    },
    {
      "type": "Route",
      "id": 372905705,
      "to": "Mezhdunarodnoye sh 60",
      "from": "Leningradskiy pr 60"
    },
    {
      "type": "Route",
      "id": 1071230749,
      "to": "Pr Vernadskogo 70",
      "from": "Tverskaya ul 50"
    },
    {
      "type": "Route",
      "id": 272575707,
      "to": "Mokhovaya ul 70",
      "from": "Tverskaya ul 60"
    },
    {
      "type": "Stop",
      "id": 341956560,
      "name": "Leningradskoye sh 50"
    },
    {
      "type": "Stop",
      "id": 978815997,
      "name": "Mokhovaya ul 30"
    },
    {
      "type": "Stop",
      "id": 132416404,
      "name": "Mokhovaya ul 10"
    },
    {
      "type": "Stop",
      "id": 948168823,
      "name": "Mezhdunarodnoye sh 50"
    },
    {
      "type": "Route",
      "id": 1173984878,
      "to": "Komsomolskiy pr 90",
      "from": "Kiyevskoye sh 30"
    },
    {
      "type": "Stop",
      "id": 744679212,
      "name": "lWtHAhJkA"
    },
    {
      "type": "Route",
      "id": 1886511862,
      "to": "Leningradskoye sh 70",
      "from": "Kremlin"
    },
    {
      "type": "Bus",
      "id": 1197887645,
      "name": "18"
    },
    {
      "type": "Route",
      "id": 987287695,
      "to": "Mokhovaya ul 20",
      "from": "Ostozhenka 60"
    },
    {
      "type": "Route",
      "id": 584594163,
      "to": "Ostozhenka 60",
      "from": "Ostozhenka 70"
    },
    {
      "type": "Route",
      "id": 117769885,
      "to": "Pr Vernadskogo 60",
      "from": "Sheremetyevo"
    },
    {
      "type": "Route",
      "id": 1375125155,
      "to": "Komsomolskiy pr 60",
      "from": "Yandex"
    },
    {
      "type": "Route",
      "id": 212397783,
      "to": "Sokol",
      "from": "Leningradskiy pr 10"
    },
    {
      "type": "Stop",
      "id": 1242118002,
      "name": "Sokol"
    },
    {
      "type": "Route",
      "id": 2017091542,
      "to": "Kiyevskoye sh 10",
      "from": "Kiyevskoye sh 80"
    },
    {
      "type": "Route",
      "id": 1445323734,
      "to": "Kiyevskoye sh 90",
      "from": "Mokhovaya ul 100"
    },
    {
      "type": "Route",
      "id": 791026214,
      "to": "Ostozhenka 30",
      "from": "Leningradskoye sh 10"
    },
    {
      "type": "Route",
      "id": 411707626,
      "to": "Mezhdunarodnoye sh 20",
      "from": "Pr Vernadskogo 80"
    },
    {
      "type": "Route",
      "id": 1455360998,
      "to": "Tverskaya ul 10",
      "from": "Pr Vernadskogo 70"
    },
    {
      "type": "Route",
      "id": 1655356417,
      "to": "Pr Vernadskogo 40",
      "from": "Komsomolskiy pr 80"
    },
    {
      "type": "Route",
      "id": 102162803,
      "to": "Leningradskiy pr 20",
      "from": "Leningradskoye sh 20"
    },
    {
      "type": "Route",
      "id": 453786017,
      "to": "Komsomolskiy pr 80",
      "from": "Mokhovaya ul 50"
    },
    {
      "type": "Route",
      "id": 1092274349,
      "to": "Ostozhenka 90",
      "from": "Mokhovaya ul 60"
    },
    {
      "type": "Route",
      "id": 470859568,
      "to": "Pr Vernadskogo 90",
      "from": "Komsomolskiy pr 10"
    },
    {
      "type": "Bus",
      "id": 1574480338,
      "name": "70k"
    },
    {
      "type": "Route",
      "id": 997113424,
      "to": "Komsomolskiy pr 50",
      "from": "Tverskaya ul 30"
    },
    {
      "type": "Bus",
      "id": 243636714,
      "name": "39k"
    },
    {
      "type": "Route",
      "id": 1642801276,
      "to": "Pr Vernadskogo 30",
      "from": "Pr Vernadskogo 30"
    },
    {
      "type": "Route",
      "id": 1733666485,
      "to": "Leningradskoye sh 70",
      "from": "Ostozhenka 20"
    },
    {
      "type": "Stop",
      "id": 1466894379,
      "name": "CLLF34Du2aq8Hw8wwIi"
    },
    {
      "type": "Bus",
      "id": 1387259836,
      "name": "51k"
    },
    {
      "type": "Route",
      "id": 609573172,
      "to": "Leningradskoye sh 90",
      "from": "Leningradskiy pr 100"
    },
    {
      "type": "Bus",
      "id": 1960392737,
      "name": "13"
    },
    {
      "type": "Route",
      "id": 670161997,
      "to": "Leningradskiy pr 90",
      "from": "Mokhovaya ul 30"
    },
    {
      "type": "Route",
      "id": 1251342801,
      "to": "Ostozhenka 100",
      "from": "Mezhdunarodnoye sh 20"
    },
    {
      "type": "Route",
      "id": 431607968,
      "to": "Mezhdunarodnoye sh 10",
      "from": "Komsomolskiy pr 30"
    },
    {
      "type": "Route",
      "id": 1853259204,
      "to": "Komsomolskiy pr 10",
      "from": "Mokhovaya ul 20"
    },
    {
      "type": "Route",
      "id": 616615236,
      "to": "Leningradskiy pr 40",
      "from": "Komsomolskiy pr 90"
    },
    {
      "type": "Route",
      "id": 59735831,
      "to": "Leningradskiy pr 10",
      "from": "Manezh"
    },
    {
      "type": "Route",
      "id": 521795577,
      "to": "Mokhovaya ul 30",
      "from": "Kiyevskoye sh 100"
    },
    {
      "type": "Stop",
      "id": 1879866889,
      "name": "Pr Vernadskogo 10"
    },
    {
      "type": "Bus",
      "id": 2102433046,
      "name": "21k"
    },
    {
      "type": "Bus",
      "id": 243636714,
      "name": "39k"
    },
    {
      "type": "Route",
      "id": 1947387346,
      "to": "Mezhdunarodnoye sh 60",
      "from": "Kiyevskoye sh 40"
    },
    {
      "type": "Stop",
      "id": 1215181049,
      "name": "Tverskaya ul 40"
    },
    {
      "type": "Route",
      "id": 1972694566,
      "to": "Vnukovo",
      "from": "Pr Vernadskogo 50"
    },
    {
      "type": "Route",
      "id": 912228900,
      "to": "Komsomolskiy pr 90",
      "from": "Mokhovaya ul 30"
    },
    {
      "type": "Route",
      "id": 1853532026,
      "to": "Kiyevskoye sh 10",
      "from": "Leningradskoye sh 100"
    },
    {
      "type": "Route",
      "id": 860721547,
      "to": "Kiyevskoye sh 90",
      "from": "Tverskaya ul 60"
    },
    {
      "type": "Stop",
      "id": 1921087375,
      "name": "Kiyevskoye sh 40"
    },
    {
      "type": "Route",
      "id": 2078846923,
      "to": "Leningradskoye sh 70",
      "from": "Kiyevskoye sh 50"
    },
    {
      "type": "Bus",
      "id": 231005000,
      "name": "90k"
    },
    {
      "type": "Bus",
      "id": 625048715,
      "name": "85k"
    },
    {
      "type": "Route",
      "id": 2112887636,
      "to": "Mokhovaya ul 20",
      "from": "Komsomolskiy pr 10"
    },
    {
      "type": "Route",
      "id": 1794547247,
      "to": "Pr Vernadskogo 90",
      "from": "Tverskaya ul 60"
    },
    {
      "type": "Bus",
      "id": 1439084482,
      "name": "nJ"
    },
    {
      "type": "Bus",
      "id": 1197887645,
      "name": "18"
    },
    {
      "type": "Stop",
      "id": 833334897,
      "name": "Mezhdunarodnoye sh 90"
    },
    {
      "type": "Bus",
      "id": 214763116,
      "name": "27k"
    },
    {
      "type": "Route",
      "id": 1561080924,
      "to": "Tverskaya ul 100",
      "from": "Pr Vernadskogo 60"
    },
    {
      "type": "Bus",
      "id": 1432394340,
      "name": "38k"
    },
    {
      "type": "Route",
      "id": 187608615,
      "to": "Pr Vernadskogo 50",
      "from": "Leningradskiy pr 40"
    },
    {
      "type": "Route",
      "id": 2037413359,
      "to": "Leningradskoye sh 70",
      "from": "Tverskaya ul 90"
    },
    {
      "type": "Route",
      "id": 887995895,
      "to": "Sheremetyevo",
      "from": "Leningradskoye sh 80"
    },
    {
      "type": "Route",
      "id": 629080831,
      "to": "Leningradskoye sh 50",
      "from": "Pr Vernadskogo 20"
    },
    {
      "type": "Route",
      "id": 77913303,
      "to": "Mezhdunarodnoye sh 50",
      "from": "Leningradskiy pr 20"
    },
    {
      "type": "Route",
      "id": 815843215,
      "to": "Komsomolskiy pr 10",
      "from": "Komsomolskiy pr 50"
    },
    {
      "type": "Route",
      "id": 1748629121,
      "to": "Leningradskoye sh 70",
      "from": "Leningradskoye sh 30"
    },
    {
      "type": "Stop",
      "id": 132416404,
      "name": "Mokhovaya ul 10"
    },
    {
      "type": "Route",
      "id": 533335950,
      "to": "Leningradskiy pr 10",
      "from": "Khimki"
    },
    {
      "type": "Route",
      "id": 668615101,
      "to": "Ostozhenka 70",
      "from": "Leningradskiy pr 80"
    },
    {
      "type": "Bus",
      "id": 1398003484,
      "name": "11"
    },
    {
      "type": "Route",
      "id": 1091231636,
      "to": "Leningradskiy pr 20",
      "from": "Leningradskoye sh 50"
    },
    {
      "type": "Route",
      "id": 766390355,
      "to": "Mokhovaya ul 100",
      "from": "Pr Vernadskogo 50"
    },
    {
      "type": "Stop",
      "id": 2059621403,
      "name": "Kiyevskoye sh 30"
    },
    {
      "type": "Route",
      "id": 1455625023,
      "to": "Tverskaya ul 40",
      "from": "Tverskaya ul 100"
    },
    {
      "type": "Stop",
      "id": 331121298,
      "name": "Mokhovaya ul 20"
    },
    {
      "type": "Route",
      "id": 1823768249,
      "to": "Komsomolskiy pr 70",
      "from": "Kiyevskoye sh 80"
    },
    {
      "type": "Route",
      "id": 1068347204,
      "to": "Mokhovaya ul 60",
      "from": "Pr Vernadskogo 60"
    },
    {
      "type": "Route",
      "id": 2037185992,
      "to": "Komsomolskiy pr 60",
      "from": "Mokhovaya ul 90"
    },
    {
      "type": "Route",
      "id": 177591521,
      "to": "Pr Vernadskogo 30",
      "from": "Mezhdunarodnoye sh 60"
    },
    {
      "type": "Route",
      "id": 6840917,
      "to": "Ostozhenka 50",
      "from": "Komsomolskiy pr 20"
    },
    {
      "type": "Route",
      "id": 988433085,
      "to": "Sokol",
      "from": "Khimki"
    },
    {
      "type": "Route",
      "id": 1848388113,
      "to": "Leningradskiy pr 60",
      "from": "Kiyevskoye sh 20"
    },
    {
      "type": "Route",
      "id": 281639940,
      "to": "Tverskaya ul 60",
      "from": "Leningradskoye sh 90"
    },
    {
      "type": "Bus",
      "id": 1152814312,
      "name": "3k"
    },
    {
      "type": "Bus",
      "id": 1183335362,
      "name": "87k"
    },
    {
      "type": "Route",
      "id": 833195589,
      "to": "Mezhdunarodnoye sh 50",
      "from": "Mokhovaya ul 90"
    },
    {
      "type": "Bus",
      "id": 243636714,
      "name": "39k"
    },
    {
      "type": "Route",
      "id": 693205939,
      "to": "Ostozhenka 70",
      "from": "Mokhovaya ul 90"
    },
    {
      "type": "Bus",
      "id": 1671271267,
      "name": "59"
    },
    {
      "type": "Bus",
      "id": 548886,
      "name": "91"
    },
    {
      "type": "Route",
      "id": 1631449496,
      "to": "Mezhdunarodnoye sh 30",
      "from": "Mezhdunarodnoye sh 100"
    },
    {
      "type": "Stop",
      "id": 1034325494,
      "name": "Mezhdunarodnoye sh 10"
    },
    {
      "type": "Route",
      "id": 469414069,
      "to": "Ostozhenka 20",
      "from": "Leningradskiy pr 90"
    },
    {
      "type": "Bus",
      "id": 548886,
      "name": "91"
    },
    {
      "type": "Stop",
      "id": 1386408138,
      "name": "Leningradskiy pr 40"
    },
    {
      "type": "Route",
      "id": 2081465429,
      "to": "Mokhovaya ul 100",
      "from": "Leningradskoye sh 20"
    },
    {
      "type": "Stop",
      "id": 1374238609,
      "name": "Leningradskoye sh 40"
    },
    {
      "type": "Stop",
      "id": 802630252,
      "name": "NmBF4ZOAqEVk0UI J"
    },
    {
      "type": "Route",
      "id": 799933121,
      "to": "Sokol",
      "from": "Pr Vernadskogo 60"
    },
    {
      "type": "Route",
      "id": 239606326,
      "to": "Ostozhenka 70",
      "from": "Leningradskoye sh 20"
    },
    {
      "type": "Stop",
      "id": 2067887205,
      "name": "Mokhovaya ul 80"
    },
    {
      "type": "Stop",
      "id": 1766990604,
      "name": "Leningradskiy pr 10"
    },
    {
      "type": "Stop",
      "id": 564351016,
      "name": "Kiyevskoye sh 80"
    },
    {
      "type": "Route",
      "id": 188241234,
      "to": "Mezhdunarodnoye sh 80",
      "from": "Mokhovaya ul 80"
    },
    {
      "type": "Route",
      "id": 839026458,
      "to": "Pr Vernadskogo 10",
      "from": "Leningradskoye sh 30"
    },
    {
      "type": "Bus",
      "id": 353672929,
      "name": "16k"
    },
    {
      "type": "Bus",
      "id": 438693796,
      "name": "46k"
    },
    {
      "type": "Bus",
      "id": 231005000,
      "name": "90k"
    },
    {
      "type": "Route",
      "id": 1268978483,
      "to": "Tverskaya ul 40",
      "from": "Komsomolskiy pr 20"
    },
    {
      "type": "Bus",
      "id": 2102040328,
      "name": "67"
    },
    {
      "type": "Route",
      "id": 550535315,
      "to": "Kiyevskoye sh 50",
      "from": "Pr Vernadskogo 50"
    },
    {
      "type": "Route",
      "id": 1302314936,
      "to": "Pr Vernadskogo 30",
      "from": "Leningradskiy pr 100"
    },
    {
      "type": "Route",
      "id": 1574151042,
      "to": "Mokhovaya ul 20",
      "from": "Leningradskoye sh 70"
    },
    {
      "type": "Route",
      "id": 862413581,
      "to": "Tverskaya ul 30",
      "from": "Vnukovo"
    },
    {
      "type": "Route",
      "id": 1923207176,
      "to": "Leningradskiy pr 40",
      "from": "Tverskaya ul 70"
    },
    {
      "type": "Route",
      "id": 2075740716,
      "to": "Mezhdunarodnoye sh 40",
      "from": "Mezhdunarodnoye sh 100"
    },
    {
      "type": "Stop",
      "id": 1396785474,
      "name": "Ostozhenka 40"
    },
    {
      "type": "Bus",
      "id": 474099276,
      "name": "26"
    },
    {
      "type": "Route",
      "id": 1756780026,
      "to": "Tverskaya ul 20",
      "from": "Mokhovaya ul 60"
    },
    {
      "type": "Route",
      "id": 454217366,
      "to": "Ostozhenka 70",
      "from": "Ostozhenka 60"
    },
    {
      "type": "Stop",
      "id": 139773181,
      "name": "ur9xdmEH58LEnic6OWw"
    },
    {
      "type": "Route",
      "id": 1596665987,
      "to": "Mokhovaya ul 60",
      "from": "Troparyovo"
    },
    {
      "type": "Route",
      "id": 415171915,
      "to": "Leningradskiy pr 20",
      "from": "Mokhovaya ul 70"
    },
    {
      "type": "Route",
      "id": 1174335662,
      "to": "Komsomolskiy pr 20",
      "from": "Ostozhenka 80"
    },
    {
      "type": "Route",
      "id": 1081614565,
      "to": "Komsomolskiy pr 70",
      "from": "Pr Vernadskogo 10"
    },
    {
      "type": "Bus",
      "id": 1742458421,
      "name": "44"
    },
    {
      "type": "Route",
      "id": 1618409648,
      "to": "Leningradskiy pr 20",
      "from": "Kiyevskoye sh 90"
    },
    {
      "type": "Route",
      "id": 776544930,
      "to": "Ostozhenka 100",
      "from": "Komsomolskiy pr 80"
    },
    {
      "type": "Bus",
      "id": 819430422,
      "name": "19k"
    },
    {
      "type": "Stop",
      "id": 1660040291,
      "name": "Komsomolskiy pr 20"
    },
    {
      "type": "Route",
      "id": 1770636148,
      "to": "Kiyevskoye sh 70",
      "from": "Leningradskoye sh 20"
    },
    {
      "type": "Route",
      "id": 585637,
      "to": "Pr Vernadskogo 20",
      "from": "Leningradskoye sh 80"
    },
    {
      "type": "Bus",
      "id": 1221092998,
      "name": "99k"
    },
    {
      "type": "Route",
      "id": 927653089,
      "to": "Leningradskoye sh 50",
      "from": "Ostozhenka 50"
    },
    {
      "type": "Route",
      "id": 1553354257,
      "to": "Ostozhenka 70",
      "from": "Pr Vernadskogo 60"
    },
    {
      "type": "Bus",
      "id": 2102433046,
      "name": "21k"
    },
    {
      "type": "Bus",
      "id": 297028842,
      "name": "10k"
    },
    {
      "type": "Stop",
      "id": 825803312,
      "name": "Komsomolskiy pr 70"
    },
    {
      "type": "Bus",
      "id": 625048715,
      "name": "85k"
    },
    {
      "type": "Route",
      "id": 2087476316,
      "to": "Tverskaya ul 30",
      "from": "Pr Vernadskogo 90"
    },
    {
      "type": "Route",
      "id": 419237352,
      "to": "Mokhovaya ul 20",
      "from": "Pr Vernadskogo 60"
    },
    {
      "type": "Route",
      "id": 148100116,
      "to": "Leningradskoye sh 40",
      "from": "Pr Vernadskogo 30"
    },
    {
      "type": "Route",
      "id": 1143258575,
      "to": "Komsomolskiy pr 50",
      "from": "Mokhovaya ul 60"
    },
    {
      "type": "Route",
      "id": 66204384,
      "to": "Kiyevskoye sh 80",
      "from": "Komsomolskiy pr 50"
    },
    {
      "type": "Route",
      "id": 1725733499,
      "to": "Leningradskoye sh 90",
      "from": "Mokhovaya ul 80"
    },
    {
      "type": "Route",
      "id": 1232824734,
      "to": "Sheremetyevo",
      "from": "Tverskaya ul 90"
    },
    {
      "type": "Route",
      "id": 1597849111,
      "to": "Leningradskiy pr 90",
      "from": "Kiyevskoye sh 10"
    },
    {
      "type": "Route",
      "id": 40135990,
      "to": "Ostozhenka 90",
      "from": "Mezhdunarodnoye sh 40"
    },
    {
      "type": "Stop",
      "id": 254220526,
      "name": "Khimki"
    },
    {
      "type": "Bus",
      "id": 870750768,
      "name": "94k"
    },
    {
      "type": "Route",
      "id": 1732918920,
      "to": "Mokhovaya ul 20",
      "from": "Leningradskoye sh 50"
    },
    {
      "type": "Route",
      "id": 907478373,
      "to": "Mokhovaya ul 70",
      "from": "Mezhdunarodnoye sh 10"
    },
    {
      "type": "Route",
      "id": 94385792,
      "to": "Luzhniki",
      "from": "Leningradskiy pr 70"
    },
    {
      "type": "Route",
      "id": 1905682988,
      "to": "Kiyevskoye sh 50",
      "from": "Komsomolskiy pr 70"
    },
    {
      "type": "Bus",
      "id": 32878382,
      "name": "79"
    },
    {
      "type": "Stop",
      "id": 564351016,
      "name": "Kiyevskoye sh 80"
    },
    {
      "type": "Route",
      "id": 1259114120,
      "to": "Leningradskoye sh 60",
      "from": "Leningradskiy pr 90"
    },
    {
      "type": "Route",
      "id": 2002627138,
      "to": "Belorusskiy vokzal",
      "from": "Leningradskoye sh 10"
    },
    {
      "type": "Route",
      "id": 1748629121,
      "to": "Leningradskoye sh 70",
      "from": "Leningradskoye sh 30"
    },
    {
      "type": "Bus",
      "id": 297028842,
      "name": "10k"
    },
    {
      "type": "Route",
      "id": 40998212,
      "to": "Vnukovo",
      "from": "Mezhdunarodnoye sh 40"
    },
    {
      "type": "Stop",
      "id": 699793610,
      "name": "Sheremetyevo"
    },
    {
      "type": "Route",
      "id": 2026858072,
      "to": "Kiyevskoye sh 60",
      "from": "Mokhovaya ul 50"
    },
    {
      "type": "Route",
      "id": 486048101,
      "to": "Mokhovaya ul 70",
      "from": "Ostozhenka 50"
    },
    {
      "type": "Route",
      "id": 1943018109,
      "to": "Pr Vernadskogo 100",
      "from": "Leningradskiy pr 10"
    },
    {
      "type": "Route",
      "id": 1013494664,
      "to": "Ostozhenka 60",
      "from": "Mezhdunarodnoye sh 90"
    },
    {
      "type": "Stop",
      "id": 956946434,
      "name": "Av5K0aNr8ESHPeJb3zEKJFkuu"
    },
    {
      "type": "Route",
      "id": 1693361495,
      "to": "Pr Vernadskogo 30",
      "from": "Mezhdunarodnoye sh 10"
    },
    {
      "type": "Route",
      "id": 1031002244,
      "to": "Tverskaya ul 10",
      "from": "Sheremetyevo"
    },
    {
      "type": "Stop",
      "id": 1770268632,
      "name": "Ostozhenka 90"
    },
    {
      "type": "Stop",
      "id": 132416404,
      "name": "Mokhovaya ul 10"
    },
    {
      "type": "Route",
      "id": 262684513,
      "to": "Tverskaya ul 90",
      "from": "Ostozhenka 80"
    },
    {
      "type": "Route",
      "id": 345752564,
      "to": "Komsomolskiy pr 70",
      "from": "Tverskaya ul 70"
    },
    {
      "type": "Stop",
      "id": 619416549,
      "name": "Tverskaya ul 70"
    },
    {
      "type": "Route",
      "id": 1228127203,
      "to": "Mokhovaya ul 20",
      "from": "Ostozhenka 30"
    },
    {
      "type": "Route",
      "id": 742020210,
      "to": "Pr Vernadskogo 100",
      "from": "Pr Vernadskogo 30"
    },
    {
      "type": "Route",
      "id": 1688558302,
      "to": "Mezhdunarodnoye sh 40",
      "from": "Mokhovaya ul 30"
    },
    {
      "type": "Route",
      "id": 649272639,
      "to": "Pr Vernadskogo 10",
      "from": "Komsomolskiy pr 100"
    },
    {
      "type": "Route",
      "id": 1671078505,
      "to": "Leningradskoye sh 80",
      "from": "Tverskaya ul 40"
    },
    {
      "type": "Route",
      "id": 1274134863,
      "to": "Mezhdunarodnoye sh 90",
      "from": "Ostozhenka 50"
    },
    {
      "type": "Route",
      "id": 256686087,
      "to": "Leningradskoye sh 30",
      "from": "Leningradskiy pr 50"
    },
    {
      "type": "Route",
      "id": 690048497,
      "to": "Komsomolskiy pr 100",
      "from": "Leningradskoye sh 70"
    },
    {
      "type": "Route",
      "id": 1720312790,
      "to": "Kiyevskoye sh 100",
      "from": "Leningradskiy pr 30"
    },
    {
      "type": "Route",
      "id": 1891242171,
      "to": "Leningradskoye sh 100",
      "from": "Komsomolskiy pr 90"
    },
    {
      "type": "Bus",
      "id": 297028842,
      "name": "10k"
    },
    {
      "type": "Route",
      "id": 68093325,
      "to": "Troparyovo",
      "from": "Tverskaya ul 30"
    },
    {
      "type": "Route",
      "id": 637503487,
      "to": "Vnukovo",
      "from": "Mezhdunarodnoye sh 30"
    },
    {
      "type": "Route",
      "id": 682614469,
      "to": "Mokhovaya ul 50",
      "from": "Mokhovaya ul 80"
    },
    {
      "type": "Stop",
      "id": 1122794222,
      "name": "Leningradskiy pr 60"
    },
    {
      "type": "Bus",
      "id": 704148699,
      "name": "93k"
    },
    {
      "type": "Bus",
      "id": 877779756,
      "name": "54"
    },
    {
      "type": "Route",
      "id": 912228900,
      "to": "Komsomolskiy pr 90",
      "from": "Mokhovaya ul 30"
    },
    {
      "type": "Route",
      "id": 1468354835,
      "to": "Tverskaya ul 60",
      "from": "Ostozhenka 30"
    },
    {
      "type": "Bus",
      "id": 1862530785,
      "name": "12"
    },
    {
      "type": "Route",
      "id": 2037259477,
      "to": "Ostozhenka 100",
      "from": "Kiyevskoye sh 50"
    },
    {
      "type": "Stop",
      "id": 769356656,
      "name": "qIXjn7OISO7qJ"
    },
    {
      "type": "Route",
      "id": 370798408,
      "to": "Kiyevskoye sh 80",
      "from": "Tverskaya ul 30"
    },
    {
      "type": "Route",
      "id": 1146248476,
      "to": "Mezhdunarodnoye sh 70",
      "from": "Leningradskoye sh 30"
    },
    {
      "type": "Route",
      "id": 1841738554,
      "to": "Pr Vernadskogo 80",
      "from": "Kiyevskoye sh 80"
    },
    {
      "type": "Route",
      "id": 1467265172,
      "to": "Mezhdunarodnoye sh 70",
      "from": "Mezhdunarodnoye sh 10"
    },
    {
      "type": "Route",
      "id": 2065402462,
      "to": "Kiyevskoye sh 90",
      "from": "Mokhovaya ul 70"
    },
    {
      "type": "Route",
      "id": 577146560,
      "to": "Ostozhenka 10",
      "from": "Ostozhenka 60"
    },
    {
      "type": "Stop",
      "id": 1603176856,
      "name": "Leningradskoye sh 80"
    },
    {
      "type": "Route",
      "id": 2054858938,
      "to": "Komsomolskiy pr 10",
      "from": "Tverskaya ul 20"
    },
    {
      "type": "Route",
      "id": 1634124223,
      "to": "Tverskaya ul 90",
      "from": "Pr Vernadskogo 100"
    },
    {
      "type": "Route",
      "id": 1091921962,
      "to": "Tverskaya ul 10",
      "from": "Kiyevskoye sh 20"
    },
    {
      "type": "Route",
      "id": 1313051356,
      "to": "Leningradskiy pr 60",
      "from": "Ostozhenka 90"
    },
    {
      "type": "Route",
      "id": 1885318223,
      "to": "Ostozhenka 50",
      "from": "Komsomolskiy pr 70"
    },
    {
      "type": "Bus",
      "id": 941326940,
      "name": "8"
    },
    {
      "type": "Bus",
      "id": 63823119,
      "name": "A"
    },
    {
      "type": "Bus",
      "id": 1387259836,
      "name": "51k"
    },
    {
      "type": "Route",
      "id": 794535371,
      "to": "Leningradskiy pr 10",
      "from": "Mokhovaya ul 100"
    },
    {
      "type": "Route",
      "id": 1433436922,
      "to": "Sokol",
      "from": "Leningradskoye sh 40"
    },
    {
      "type": "Route",
      "id": 1106838333,
      "to": "Leningradskiy pr 80",
      "from": "Kiyevskoye sh 80"
    },
    {
      "type": "Stop",
      "id": 1660040291,
      "name": "Komsomolskiy pr 20"
    },
    {
      "type": "Stop",
      "id": 472257200,
      "name": "Ostozhenka 100"
    },
    {
      "type": "Route",
      "id": 1440595467,
      "to": "Mokhovaya ul 100",
      "from": "Khimki"
    },
    {
      "type": "Route",
      "id": 754695249,
      "to": "Tverskaya ul 90",
      "from": "Mezhdunarodnoye sh 50"
    },
    {
      "type": "Route",
      "id": 62892066,
      "to": "Ostozhenka 40",
      "from": "Ostozhenka 50"
    },
    {
      "type": "Route",
      "id": 594223772,
      "to": "Leningradskiy pr 80",
      "from": "Kiyevskoye sh 30"
    },
    {
      "type": "Stop",
      "id": 1603176856,
      "name": "Leningradskoye sh 80"
    },
    {
      "type": "Route",
      "id": 1564104053,
      "to": "Ostozhenka 60",
      "from": "Leningradskoye sh 40"
    },
    {
      "type": "Route",
      "id": 1735210429,
      "to": "Sokol",
      "from": "Ostozhenka 60"
    },
    {
      "type": "Route",
      "id": 1723425088,
      "to": "Komsomolskiy pr 50",
      "from": "Ostozhenka 10"
    },
    {
      "type": "Stop",
      "id": 1971821751,
      "name": "Komsomolskiy pr 30"
    },
    {
      "type": "Route",
      "id": 543951686,
      "to": "Mezhdunarodnoye sh 100",
      "from": "Mokhovaya ul 10"
    },
    {
      "type": "Bus",
      "id": 519279717,
      "name": "31k"
    },
    {
      "type": "Stop",
      "id": 1524276591,
      "name": "Kiyevskoye sh 70"
    },
    {
      "type": "Bus",
      "id": 2085998310,
      "name": "29k"
    },
    {
      "type": "Route",
      "id": 1538579290,
      "to": "Mokhovaya ul 60",
      "from": "Leningradskiy pr 10"
    },
    {
      "type": "Bus",
      "id": 1858267958,
      "name": "48k"
    },
    {
      "type": "Route",
      "id": 935445902,
      "to": "Mokhovaya ul 40",
      "from": "Khimki"
    },
    {
      "type": "Stop",
      "id": 694034036,
      "name": "Kiyevskoye sh 50"
    },
    {
      "type": "Stop",
      "id": 1748756235,
      "name": "Komsomolskiy pr 100"
    },
    {
      "type": "Route",
      "id": 1109523700,
      "to": "Luzhniki",
      "from": "Komsomolskiy pr 10"
    },
    {
      "type": "Route",
      "id": 496209186,
      "to": "Tverskaya ul 80",
      "from": "Komsomolskiy pr 80"
    },
    {
      "type": "Route",
      "id": 1387809291,
      "to": "Pr Vernadskogo 80",
      "from": "Pr Vernadskogo 90"
    },
    {
      "type": "Route",
      "id": 1734327526,
      "to": "Komsomolskiy pr 20",
      "from": "Tverskaya ul 10"
    },
    {
      "type": "Bus",
      "id": 1574480338,
      "name": "70k"
    },
    {
      "type": "Bus",
      "id": 231005000,
      "name": "90k"
    },
    {
      "type": "Stop",
      "id": 132416404,
      "name": "Mokhovaya ul 10"
    },
    {
      "type": "Route",
      "id": 558680858,
      "to": "Pr Vernadskogo 70",
      "from": "Mezhdunarodnoye sh 30"
    },
    {
      "type": "Stop",
      "id": 1770268632,
      "name": "Ostozhenka 90"
    },
    {
      "type": "Route",
      "id": 1448089333,
      "to": "Mezhdunarodnoye sh 40",
      "from": "Leningradskiy pr 40"
    },
    {
      "type": "Route",
      "id": 1416047162,
      "to": "Ostozhenka 10",
      "from": "Kiyevskoye sh 40"
    },
    {
      "type": "Route",
      "id": 1269258375,
      "to": "Mokhovaya ul 100",
      "from": "Ostozhenka 90"
    },
    {
      "type": "Route",
      "id": 1353487675,
      "to": "Tverskaya ul 90",
      "from": "Pr Vernadskogo 60"
    },
    {
      "type": "Route",
      "id": 1223296009,
      "to": "Mokhovaya ul 30",
      "from": "Ostozhenka 40"
    },
    {
      "type": "Route",
      "id": 262700798,
      "to": "Komsomolskiy pr 80",
      "from": "Komsomolskiy pr 70"
    },
    {
      "type": "Bus",
      "id": 1574480338,
      "name": "70k"
    },
    {
      "type": "Stop",
      "id": 331121298,
      "name": "Mokhovaya ul 20"
    },
    {
      "type": "Route",
      "id": 954401089,
      "to": "Belorusskiy vokzal",
      "from": "Mokhovaya ul 70"
    },
    {
      "type": "Route",
      "id": 1886177123,
      "to": "Belorusskiy vokzal",
      "from": "Mokhovaya ul 10"
    },
    {
      "type": "Route",
      "id": 103756314,
      "to": "Mokhovaya ul 10",
      "from": "Khimki"
    },
    {
      "type": "Route",
      "id": 1867418929,
      "to": "Ostozhenka 70",
      "from": "Leningradskiy pr 10"
    },
    {
      "type": "Stop",
      "id": 1971821751,
      "name": "Komsomolskiy pr 30"
    },
    {
      "type": "Bus",
      "id": 291021923,
      "name": "68k"
    },
    {
      "type": "Bus",
      "id": 95205841,
      "name": "58"
    },
    {
      "type": "Bus",
      "id": 1674389347,
      "name": "34"
    },
    {
      "type": "Bus",
      "id": 1862530785,
      "name": "12"
    },
    {
      "type": "Route",
      "id": 1849379497,
      "to": "Leningradskoye sh 40",
      "from": "Vnukovo"
    },
    {
      "type": "Stop",
      "id": 2095250019,
      "name": "Tverskaya ul 50"
    },
    {
      "type": "Route",
      "id": 470794004,
      "to": "Pr Vernadskogo 10",
      "from": "Leningradskoye sh 100"
    },
    {
      "type": "Route",
      "id": 1712712685,
      "to": "Tverskaya ul 90",
      "from": "Tverskaya ul 80"
    },
    {
      "type": "Route",
      "id": 775033967,
      "to": "Leningradskiy pr 80",
      "from": "Mezhdunarodnoye sh 100"
    },
    {
      "type": "Route",
      "id": 879884340,
      "to": "Tverskaya ul 60",
      "from": "Luzhniki"
    },
    {
      "type": "Route",
      "id": 1484223989,
      "to": "Ostozhenka 80",
      "from": "Komsomolskiy pr 10"
    },
    {
      "type": "Stop",
      "id": 721281380,
      "name": "Leningradskiy pr 50"
    },
    {
      "type": "Route",
      "id": 852341676,
      "to": "Leningradskoye sh 30",
      "from": "Mezhdunarodnoye sh 60"
    },
    {
      "type": "Bus",
      "id": 1000797031,
      "name": "35"
    },
    {
      "type": "Bus",
      "id": 1391108907,
      "name": "4"
    },
    {
      "type": "Bus",
      "id": 1148160947,
      "name": "57k"
    },
    {
      "type": "Bus",
      "id": 1551351181,
      "name": "2k"
    },
    {
      "type": "Route",
      "id": 699670172,
      "to": "Mezhdunarodnoye sh 50",
      "from": "Komsomolskiy pr 90"
    },
    {
      "type": "Route",
      "id": 1453296722,
      "to": "Leningradskoye sh 80",
      "from": "Kiyevskoye sh 40"
    },
    {
      "type": "Route",
      "id": 1761177084,
      "to": "Tverskaya ul 70",
      "from": "Mezhdunarodnoye sh 90"
    },
    {
      "type": "Bus",
      "id": 1952512570,
      "name": "62"
    },
    {
      "type": "Stop",
      "id": 132064624,
      "name": "Komsomolskiy pr 50"
    },
    {
      "type": "Stop",
      "id": 2077587727,
      "name": "Pr Vernadskogo 40"
    },
    {
      "type": "Stop",
      "id": 1390717557,
      "name": "Leningradskoye sh 30"
    },
    {
      "type": "Route",
      "id": 700876418,
      "to": "Sheremetyevo",
      "from": "Leningradskiy pr 20"
    },
    {
      "type": "Route",
      "id": 2069258633,
      "to": "Kiyevskoye sh 50",
      "from": "Kiyevskoye sh 20"
    },
    {
      "type": "Route",
      "id": 1586342743,
      "to": "Komsomolskiy pr 70",
      "from": "Tverskaya ul 50"
    },
    {
      "type": "Route",
      "id": 1726403946,
      "to": "Kiyevskoye sh 50",
      "from": "Kiyevskoye sh 10"
    },
    {
      "type": "Bus",
      "id": 1183335362,
      "name": "87k"
    },
    {
      "type": "Stop",
      "id": 69735714,
      "name": "umT9X6qmxWLabig"
    },
    {
      "type": "Stop",
      "id": 936608505,
      "name": "Z05E"
    },
    {
      "type": "Stop",
      "id": 1901744419,
      "name": "Leningradskoye sh 60"
    },
    {
      "type": "Stop",
      "id": 341956560,
      "name": "Leningradskoye sh 50"
    },
    {
      "type": "Route",
      "id": 2136596711,
      "to": "Kremlin",
      "from": "Manezh"
    },
    {
      "type": "Route",
      "id": 154455418,
      "to": "Leningradskoye sh 60",
      "from": "Ostozhenka 40"
    },
    {
      "type": "Bus",
      "id": 271035707,
      "name": "69k"
    },
    {
      "type": "Stop",
      "id": 2059621403,
      "name": "Kiyevskoye sh 30"
    },
    {
      "type": "Bus",
      "id": 1398003484,
      "name": "11"
    },
    {
      "type": "Route",
      "id": 598145070,
      "to": "Pr Vernadskogo 90",
      "from": "Mokhovaya ul 60"
    },
    {
      "type": "Route",
      "id": 1028603880,
      "to": "Mokhovaya ul 90",
      "from": "Ostozhenka 80"
    },
    {
      "type": "Route",
      "id": 609464518,
      "to": "Leningradskoye sh 90",
      "from": "Leningradskiy pr 50"
    },
    {
      "type": "Route",
      "id": 81942293,
      "to": "Mokhovaya ul 30",
      "from": "Mokhovaya ul 40"
    },
    {
      "type": "Route",
      "id": 596514140,
      "to": "Leningradskiy pr 30",
      "from": "Leningradskiy pr 30"
    },
    {
      "type": "Bus",
      "id": 1997217022,
      "name": "83k"
    },
    {
      "type": "Route",
      "id": 1565845310,
      "to": "Leningradskiy pr 70",
      "from": "Kiyevskoye sh 60"
    },
    {
      "type": "Route",
      "id": 293125111,
      "to": "Tverskaya ul 20",
      "from": "Mokhovaya ul 80"
    },
    {
      "type": "Route",
      "id": 2085826742,
      "to": "Ostozhenka 70",
      "from": "Komsomolskiy pr 60"
    },
    {
      "type": "Stop",
      "id": 751222496,
      "name": "Mokhovaya ul 100"
    },
    {
      "type": "Route",
      "id": 560402950,
      "to": "Ostozhenka 40",
      "from": "Ostozhenka 30"
    },
    {
      "type": "Stop",
      "id": 1321456988,
      "name": "Mezhdunarodnoye sh 100"
    },
    {
      "type": "Route",
      "id": 291764761,
      "to": "Komsomolskiy pr 30",
      "from": "Ostozhenka 30"
    },
    {
      "type": "Route",
      "id": 635601839,
      "to": "Leningradskiy pr 30",
      "from": "Leningradskoye sh 70"
    },
    {
      "type": "Stop",
      "id": 316546093,
      "name": "Mokhovaya ul 60"
    },
    {
      "type": "Route",
      "id": 1707470319,
      "to": "Leningradskiy pr 40",
      "from": "Pr Vernadskogo 70"
    },
    {
      "type": "Route",
      "id": 147180656,
      "to": "Kiyevskoye sh 70",
      "from": "Pr Vernadskogo 90"
    },
    {
      "type": "Bus",
      "id": 1184095094,
      "name": "64k"
    },
    {
      "type": "Route",
      "id": 1955581502,
      "to": "Pr Vernadskogo 100",
      "from": "Tverskaya ul 30"
    },
    {
      "type": "Bus",
      "id": 1024709661,
      "name": "53k"
    },
    {
      "type": "Route",
      "id": 1850296263,
      "to": "Pr Vernadskogo 90",
      "from": "Mezhdunarodnoye sh 10"
    },
    {
      "type": "Bus",
      "id": 1081123661,
      "name": "65k"
    },
    {
      "type": "Bus",
      "id": 1500331975,
      "name": "100k"
    },
    {
      "type": "Route",
      "id": 788334162,
      "to": "Ostozhenka 30",
      "from": "Leningradskoye sh 90"
    },
    {
      "type": "Stop",
      "id": 1034325494,
      "name": "Mezhdunarodnoye sh 10"
    },
    {
      "type": "Route",
      "id": 431332602,
      "to": "Mezhdunarodnoye sh 100",
      "from": "Leningradskiy pr 30"
    },
    {
      "type": "Route",
      "id": 565053448,
      "to": "Belorusskiy vokzal",
      "from": "Leningradskoye sh 60"
    },
    {
      "type": "Route",
      "id": 1748712160,
      "to": "Mokhovaya ul 50",
      "from": "Kiyevskoye sh 60"
    },
    {
      "type": "Route",
      "id": 346031210,
      "to": "Mezhdunarodnoye sh 40",
      "from": "Leningradskiy pr 90"
    },
    {
      "type": "Route",
      "id": 104132674,
      "to": "Kiyevskoye sh 60",
      "from": "Pr Vernadskogo 100"
    },
    {
      "type": "Route",
      "id": 255464848,
      "to": "Ostozhenka 100",
      "from": "Komsomolskiy pr 50"
    },
    {
      "type": "Route",
      "id": 1427942358,
      "to": "Kremlin",
      "from": "Leningradskiy pr 50"
    },
    {
      "type": "Bus",
      "id": 474099276,
      "name": "26"
    },
    {
      "type": "Route",
      "id": 1537939540,
      "to": "Tverskaya ul 10",
      "from": "Ostozhenka 90"
    },
    {
      "type": "Route",
      "id": 1959473558,
      "to": "Ostozhenka 40",
      "from": "Leningradskoye sh 70"
    },
    {
      "type": "Route",
      "id": 1011581861,
      "to": "Leningradskoye sh 20",
      "from": "Leningradskiy pr 50"
    },
    {
      "type": "Route",
      "id": 1451396765,
      "to": "Komsomolskiy pr 30",
      "from": "Leningradskoye sh 90"
    },
    {
      "type": "Bus",
      "id": 1320378198,
      "name": "32"
    },
    {
      "type": "Route",
      "id": 1004562835,
      "to": "Sokol",
      "from": "Mokhovaya ul 100"
    },
    {
      "type": "Route",
      "id": 1762754270,
      "to": "Leningradskiy pr 30",
      "from": "Mezhdunarodnoye sh 40"
    },
    {
      "type": "Stop",
      "id": 564351016,
      "name": "Kiyevskoye sh 80"
    },
    {
      "type": "Stop",
      "id": 699793610,
      "name": "Sheremetyevo"
    },
    {
      "type": "Route",
      "id": 1965701123,
      "to": "Ostozhenka 70",
      "from": "Pr Vernadskogo 80"
    },
    {
      "type": "Route",
      "id": 1481209273,
      "to": "Mokhovaya ul 20",
      "from": "Mokhovaya ul 40"
    },
    {
      "type": "Route",
      "id": 1176654741,
      "to": "Komsomolskiy pr 10",
      "from": "Mokhovaya ul 10"
    },
    {
      "type": "Bus",
      "id": 438693796,
      "name": "46k"
    },
    {
      "type": "Stop",
      "id": 948168823,
      "name": "Mezhdunarodnoye sh 50"
    },
    {
      "type": "Bus",
      "id": 801615852,
      "name": "89"
    },
    {
      "type": "Route",
      "id": 526876836,
      "to": "Kiyevskoye sh 10",
      "from": "Komsomolskiy pr 60"
    },
    {
      "type": "Stop",
      "id": 1122794222,
      "name": "Leningradskiy pr 60"
    },
    {
      "type": "Stop",
      "id": 1879866889,
      "name": "Pr Vernadskogo 10"
    },
    {
      "type": "Stop",
      "id": 694649906,
      "name": "Manezh"
    },
    {
      "type": "Route",
      "id": 2130321954,
      "to": "Kiyevskoye sh 100",
      "from": "Pr Vernadskogo 60"
    },
    {
      "type": "Bus",
      "id": 1197887645,
      "name": "18"
    },
    {
      "type": "Route",
      "id": 1660929748,
      "to": "Kiyevskoye sh 70",
      "from": "Sheremetyevo"
    },
    {
      "type": "Route",
      "id": 1757588615,
      "to": "Tverskaya ul 80",
      "from": "Kiyevskoye sh 90"
    },
    {
      "type": "Route",
      "id": 1882398687,
      "to": "Manezh",
      "from": "Pr Vernadskogo 70"
    },
    {
      "type": "Bus",
      "id": 2102040328,
      "name": "67"
    },
    {
      "type": "Stop",
      "id": 359984358,
      "name": "Ostozhenka 80"
    },
    {
      "type": "Stop",
      "id": 1839841385,
      "name": "Tverskaya ul 20"
    },
    {
      "type": "Route",
      "id": 610287404,
      "to": "Tverskaya ul 100",
      "from": "Troparyovo"
    },
    {
      "type": "Route",
      "id": 571348360,
      "to": "Tverskaya ul 40",
      "from": "Ostozhenka 20"
    },
    {
      "type": "Stop",
      "id": 2017557436,
      "name": "Komsomolskiy pr 90"
    },
    {
      "type": "Bus",
      "id": 1184095094,
      "name": "64k"
    },
    {
      "type": "Route",
      "id": 837904778,
      "to": "Ostozhenka 10",
      "from": "Leningradskoye sh 60"
    },
    {
      "type": "Route",
      "id": 909857630,
      "to": "Leningradskoye sh 40",
      "from": "Kiyevskoye sh 100"
    },
    {
      "type": "Route",
      "id": 1289124913,
      "to": "Mokhovaya ul 20",
      "from": "Mokhovaya ul 50"
    },
    {
      "type": "Bus",
      "id": 1574480338,
      "name": "70k"
    },
    {
      "type": "Stop",
      "id": 1640544668,
      "name": "Leningradskoye sh 70"
    },
    {
      "type": "Stop",
      "id": 699793610,
      "name": "Sheremetyevo"
    },
    {
      "type": "Route",
      "id": 227689018,
      "to": "Mezhdunarodnoye sh 20",
      "from": "Sokol"
    },
    {
      "type": "Stop",
      "id": 2059621403,
      "name": "Kiyevskoye sh 30"
    },
    {
      "type": "Route",
      "id": 142274688,
      "to": "Pr Vernadskogo 40",
      "from": "Kiyevskoye sh 20"
    },
    {
      "type": "Route",
      "id": 1995029730,
      "to": "Pr Vernadskogo 80",
      "from": "Leningradskiy pr 60"
    },
    {
      "type": "Route",
      "id": 2100719136,
      "to": "Mezhdunarodnoye sh 100",
      "from": "Ostozhenka 60"
    },
    {
      "type": "Route",
      "id": 1489552622,
      "to": "Mokhovaya ul 80",
      "from": "Mezhdunarodnoye sh 50"
    },
    {
      "type": "Route",
      "id": 1708928416,
      "to": "Luzhniki",
      "from": "Leningradskoye sh 30"
    },
    {
      "type": "Route",
      "id": 564005384,
      "to": "Leningradskoye sh 20",
      "from": "Manezh"
    },
    {
      "type": "Route",
      "id": 2139585889,
      "to": "Kremlin",
      "from": "Komsomolskiy pr 80"
    },
    {
      "type": "Route",
      "id": 1938981673,
      "to": "Ostozhenka 30",
      "from": "Komsomolskiy pr 100"
    },
    {
      "type": "Route",
      "id": 1301564813,
      "to": "Kiyevskoye sh 80",
      "from": "Troparyovo"
    },
    {
      "type": "Route",
      "id": 759502559,
      "to": "Kiyevskoye sh 90",
      "from": "Kremlin"
    },
    {
      "type": "Stop",
      "id": 474720704,
      "name": "Pr Vernadskogo 50"
    },
    {
      "type": "Route",
      "id": 1310023469,
      "to": "Kiyevskoye sh 20",
      "from": "Yandex"
    },
    {
      "type": "Route",
      "id": 1115060164,
      "to": "Mokhovaya ul 100",
      "from": "Mezhdunarodnoye sh 40"
    },
    {
      "type": "Route",
      "id": 179118343,
      "to": "Mokhovaya ul 80",
      "from": "Leningradskoye sh 70"
    },
    {
      "type": "Route",
      "id": 651169569,
      "to": "Komsomolskiy pr 60",
      "from": "Pr Vernadskogo 60"
    },
    {
      "type": "Bus",
      "id": 63424298,
      "name": "80k"
    },
    {
      "type": "Bus",
      "id": 1674389347,
      "name": "34"
    },
    {
      "type": "Bus",
      "id": 1695308373,
      "name": "15"
    },
    {
      "type": "Route",
      "id": 1846266422,
      "to": "Leningradskiy pr 80",
      "from": "Mezhdunarodnoye sh 10"
    },
    {
      "type": "Route",
      "id": 1896183796,
      "to": "Leningradskiy pr 90",
      "from": "Ostozhenka 90"
    },
    {
      "type": "Bus",
      "id": 1391108907,
      "name": "4"
    },
    {
      "type": "Route",
      "id": 1992845801,
      "to": "Komsomolskiy pr 60",
      "from": "Mokhovaya ul 60"
    },
    {
      "type": "Bus",
      "id": 1888137148,
      "name": "84k"
    },
    {
      "type": "Stop",
      "id": 2059621403,
      "name": "Kiyevskoye sh 30"
    },
    {
      "type": "Bus",
      "id": 519279717,
      "name": "31k"
    },
    {
      "type": "Route",
      "id": 858021245,
      "to": "Komsomolskiy pr 60",
      "from": "Leningradskiy pr 10"
    },
    {
      "type": "Bus",
      "id": 1172557281,
      "name": "97"
    },
    {
      "type": "Stop",
      "id": 1524276591,
      "name": "Kiyevskoye sh 70"
    },
    {
      "type": "Route",
      "id": 390689813,
      "to": "Mezhdunarodnoye sh 100",
      "from": "Tverskaya ul 50"
    },
    {
      "type": "Bus",
      "id": 870750768,
      "name": "94k"
    },
    {
      "type": "Bus",
      "id": 877779756,
      "name": "54"
    },
    {
      "type": "Route",
      "id": 1934142683,
      "to": "Leningradskiy pr 90",
      "from": "Tverskaya ul 40"
    },
    {
      "type": "Route",
      "id": 82224349,
      "to": "Kremlin",
      "from": "Mokhovaya ul 30"
    },
    {
      "type": "Route",
      "id": 1105880470,
      "to": "Komsomolskiy pr 80",
      "from": "Tverskaya ul 30"
    },
    {
      "type": "Route",
      "id": 1268325587,
      "to": "Mokhovaya ul 30",
      "from": "Leningradskiy pr 80"
    },
    {
      "type": "Route",
      "id": 923683691,
      "to": "Ostozhenka 20",
      "from": "Mokhovaya ul 50"
    },
    {
      "type": "Bus",
      "id": 1679261573,
      "name": "74k"
    },
    {
      "type": "Route",
      "id": 879885454,
      "to": "Ostozhenka 20",
      "from": "Leningradskoye sh 70"
    },
    {
      "type": "Route",
      "id": 1772295565,
      "to": "Komsomolskiy pr 30",
      "from": "Kiyevskoye sh 60"
    },
    {
      "type": "Stop",
      "id": 1791067641,
      "name": "Mokhovaya ul 40"
    },
    {
      "type": "Route",
      "id": 1375819386,
      "to": "Mokhovaya ul 10",
      "from": "Ostozhenka 70"
    },
    {
      "type": "Stop",
      "id": 132064624,
      "name": "Komsomolskiy pr 50"
    },
    {
      "type": "Route",
      "id": 854382561,
      "to": "Leningradskoye sh 60",
      "from": "Kiyevskoye sh 40"
    },
    {
      "type": "Route",
      "id": 720218277,
      "to": "Leningradskiy pr 10",
      "from": "Leningradskoye sh 40"
    },
    {
      "type": "Route",
      "id": 203237445,
      "to": "Tverskaya ul 70",
      "from": "Komsomolskiy pr 50"
    },
    {
      "type": "Route",
      "id": 863387643,
      "to": "Mezhdunarodnoye sh 30",
      "from": "Pr Vernadskogo 60"
    },
    {
      "type": "Stop",
      "id": 694649906,
      "name": "Manezh"
    },
    {
      "type": "Stop",
      "id": 592711837,
      "name": "5Mzyosg2Iv9qNW"
    },
    {
      "type": "Stop",
      "id": 1071355180,
      "name": "Pr Vernadskogo 20"
    },
    {
      "type": "Route",
      "id": 663792372,
      "to": "Leningradskiy pr 50",
      "from": "Mezhdunarodnoye sh 100"
    },
    {
      "type": "Route",
      "id": 227676706,
      "to": "Troparyovo",
      "from": "Komsomolskiy pr 20"
    },
    {
      "type": "Route",
      "id": 548434071,
      "to": "Komsomolskiy pr 50",
      "from": "Leningradskoye sh 10"
    },
    {
      "type": "Stop",
      "id": 672630347,
      "name": "g8eMQ0YkXGekIKaK0cm7sZ3b8"
    },
    {
      "type": "Route",
      "id": 331070549,
      "to": "Mokhovaya ul 70",
      "from": "Ostozhenka 30"
    },
    {
      "type": "Route",
      "id": 1733807058,
      "to": "Belorusskiy vokzal",
      "from": "Mezhdunarodnoye sh 100"
    },
    {
      "type": "Route",
      "id": 298131953,
      "to": "Komsomolskiy pr 10",
      "from": "Pr Vernadskogo 70"
    },
    {
      "type": "Stop",
      "id": 1603176856,
      "name": "Leningradskoye sh 80"
    },
    {
      "type": "Route",
      "id": 1166569563,
      "to": "Tverskaya ul 20",
      "from": "Ostozhenka 100"
    },
    {
      "type": "Route",
      "id": 1544102293,
      "to": "Kiyevskoye sh 80",
      "from": "Mokhovaya ul 40"
    },
    {
      "type": "Stop",
      "id": 802437680,
      "name": "Kiyevskoye sh 20"
    },
    {
      "type": "Bus",
      "id": 799165931,
      "name": "24k"
    },
    {
      "type": "Bus",
      "id": 291021923,
      "name": "68k"
    },
    {
      "type": "Route",
      "id": 232841227,
      "to": "Leningradskoye sh 60",
      "from": "Tverskaya ul 70"
    },
    {
      "type": "Stop",
      "id": 1971821751,
      "name": "Komsomolskiy pr 30"
    },
    {
      "type": "Route",
      "id": 1936688346,
      "to": "Leningradskoye sh 70",
      "from": "Pr Vernadskogo 10"
    },
    {
      "type": "Stop",
      "id": 1034325494,
      "name": "Mezhdunarodnoye sh 10"
    },
    {
      "type": "Route",
      "id": 1911228449,
      "to": "Leningradskoye sh 80",
      "from": "Luzhniki"
    },
    {
      "type": "Bus",
      "id": 76662407,
      "name": "72k"
    },
    {
      "type": "Route",
      "id": 1320562907,
      "to": "Khimki",
      "from": "Leningradskiy pr 90"
    },
    {
      "type": "Route",
      "id": 1478259370,
      "to": "Mokhovaya ul 70",
      "from": "Tverskaya ul 20"
    },
    {
      "type": "Stop",
      "id": 1071355180,
      "name": "Pr Vernadskogo 20"
    },
    {
      "type": "Stop",
      "id": 1524276591,
      "name": "Kiyevskoye sh 70"
    },
    {
      "type": "Route",
      "id": 1576156015,
      "to": "Pr Vernadskogo 30",
      "from": "Komsomolskiy pr 100"
    },
    {
      "type": "Route",
      "id": 1031385399,
      "to": "Leningradskoye sh 10",
      "from": "Mokhovaya ul 20"
    },
    {
      "type": "Stop",
      "id": 522478233,
      "name": "Mokhovaya ul 70"
    },
    {
      "type": "Bus",
      "id": 474099276,
      "name": "26"
    },
    {
      "type": "Route",
      "id": 2005372518,
      "to": "Komsomolskiy pr 30",
      "from": "Belorusskiy vokzal"
    },
    {
      "type": "Route",
      "id": 73525764,
      "to": "Mezhdunarodnoye sh 80",
      "from": "Komsomolskiy pr 50"
    },
    {
      "type": "Bus",
      "id": 1742458421,
      "name": "44"
    },
    {
      "type": "Route",
      "id": 2128614664,
      "to": "Ostozhenka 50",
      "from": "Tverskaya ul 90"
    },
    {
      "type": "Stop",
      "id": 132064624,
      "name": "Komsomolskiy pr 50"
    },
    {
      "type": "Route",
      "id": 459967247,
      "to": "Leningradskiy pr 50",
      "from": "Mezhdunarodnoye sh 40"
    },
    {
      "type": "Route",
      "id": 1802212764,
      "to": "Mezhdunarodnoye sh 50",
      "from": "Mezhdunarodnoye sh 80"
    },
    {
      "type": "Bus",
      "id": 862596254,
      "name": "63"
    },
    {
      "type": "Route",
      "id": 181224577,
      "to": "Kiyevskoye sh 50",
      "from": "Leningradskiy pr 90"
    },
    {
      "type": "Route",
      "id": 596323985,
      "to": "Mokhovaya ul 30",
      "from": "Tverskaya ul 70"
    },
    {
      "type": "Route",
      "id": 827012710,
      "to": "Kiyevskoye sh 90",
      "from": "Ostozhenka 10"
    },
    {
      "type": "Route",
      "id": 655541433,
      "to": "Ostozhenka 20",
      "from": "Pr Vernadskogo 70"
    },
    {
      "type": "Bus",
      "id": 589555403,
      "name": "47k"
    },
    {
      "type": "Route",
      "id": 627970370,
      "to": "Mezhdunarodnoye sh 90",
      "from": "Leningradskiy pr 60"
    },
    {
      "type": "Route",
      "id": 470803132,
      "to": "Leningradskoye sh 10",
      "from": "Ostozhenka 30"
    },
    {
      "type": "Route",
      "id": 1173734170,
      "to": "Mokhovaya ul 60",
      "from": "Kiyevskoye sh 100"
    },
    {
      "type": "Stop",
      "id": 2077587727,
      "name": "Pr Vernadskogo 40"
    },
    {
      "type": "Stop",
      "id": 844924949,
      "name": "Kiyevskoye sh 60"
    },
    {
      "type": "Stop",
      "id": 359984358,
      "name": "Ostozhenka 80"
    },
    {
      "type": "Route",
      "id": 336578226,
      "to": "Leningradskiy pr 90",
      "from": "Leningradskoye sh 100"
    },
    {
      "type": "Route",
      "id": 1495576277,
      "to": "Mezhdunarodnoye sh 100",
      "from": "Leningradskoye sh 80"
    },
    {
      "type": "Stop",
      "id": 1438491283,
      "name": "Yandex"
    },
    {
      "type": "Route",
      "id": 346187454,
      "to": "Kiyevskoye sh 40",
      "from": "Mokhovaya ul 60"
    },
    {
      "type": "Bus",
      "id": 2086679848,
      "name": "73k"
    },
    {
      "type": "Stop",
      "id": 1847557780,
      "name": "Mezhdunarodnoye sh 30"
    },
    {
      "type": "Route",
      "id": 1850952842,
      "to": "Mokhovaya ul 80",
      "from": "Leningradskoye sh 40"
    },
    {
      "type": "Route",
      "id": 687789409,
      "to": "Mokhovaya ul 30",
      "from": "Mezhdunarodnoye sh 80"
    },
    {
      "type": "Bus",
      "id": 214763116,
      "name": "27k"
    },
    {
      "type": "Stop",
      "id": 1770268632,
      "name": "Ostozhenka 90"
    },
    {
      "type": "Route",
      "id": 1253401049,
      "to": "Kiyevskoye sh 80",
      "from": "Ostozhenka 90"
    },
    {
      "type": "Stop",
      "id": 1438491283,
      "name": "Yandex"
    },
    {
      "type": "Route",
      "id": 1222810770,
      "to": "Pr Vernadskogo 30",
      "from": "Leningradskoye sh 20"
    },
    {
      "type": "Stop",
      "id": 1692384196,
      "name": "Mezhdunarodnoye sh 80"
    },
    {
      "type": "Stop",
      "id": 564351016,
      "name": "Kiyevskoye sh 80"
    },
    {
      "type": "Route",
      "id": 254155866,
      "to": "Ostozhenka 90",
      "from": "Leningradskoye sh 30"
    },
    {
      "type": "Bus",
      "id": 49095282,
      "name": "82k"
    },
    {
      "type": "Route",
      "id": 1836589183,
      "to": "Tverskaya ul 30",
      "from": "Kiyevskoye sh 50"
    },
    {
      "type": "Stop",
      "id": 1488211348,
      "name": "Leningradskiy pr 100"
    },
    {
      "type": "Route",
      "id": 330341228,
      "to": "Ostozhenka 30",
      "from": "Leningradskoye sh 30"
    },
    {
      "type": "Bus",
      "id": 1204847478,
      "name": "tVY"
    },
    {
      "type": "Route",
      "id": 877704082,
      "to": "Pr Vernadskogo 30",
      "from": "Pr Vernadskogo 70"
    },
    {
      "type": "Route",
      "id": 489966296,
      "to": "Leningradskiy pr 20",
      "from": "Sheremetyevo"
    },
    {
      "type": "Bus",
      "id": 519279717,
      "name": "31k"
    },
    {
      "type": "Route",
      "id": 358755586,
      "to": "Leningradskoye sh 50",
      "from": "Leningradskoye sh 10"
    },
    {
      "type": "Bus",
      "id": 941326940,
      "name": "8"
    },
    {
      "type": "Route",
      "id": 1649260515,
      "to": "Vnukovo",
      "from": "Tverskaya ul 60"
    },
    {
      "type": "Route",
      "id": 2111143080,
      "to": "Tverskaya ul 70",
      "from": "Ostozhenka 90"
    },
    {
      "type": "Bus",
      "id": 1532241955,
      "name": "7k"
    },
    {
      "type": "Bus",
      "id": 1865859901,
      "name": "96"
    },
    {
      "type": "Bus",
      "id": 1674389347,
      "name": "34"
    },
    {
      "type": "Bus",
      "id": 1197887645,
      "name": "18"
    },
    {
      "type": "Route",
      "id": 1167559402,
      "to": "Tverskaya ul 60",
      "from": "Mezhdunarodnoye sh 10"
    },
    {
      "type": "Route",
      "id": 1821559365,
      "to": "Mezhdunarodnoye sh 70",
      "from": "Komsomolskiy pr 40"
    },
    {
      "type": "Route",
      "id": 378335081,
      "to": "Ostozhenka 60",
      "from": "Luzhniki"
    },
    {
      "type": "Route",
      "id": 720715757,
      "to": "Leningradskoye sh 100",
      "from": "Khimki"
    },
    {
      "type": "Stop",
      "id": 1735256877,
      "name": "Komsomolskiy pr 40"
    },
    {
      "type": "Route",
      "id": 482931789,
      "to": "Leningradskoye sh 10",
      "from": "Pr Vernadskogo 10"
    },
    {
      "type": "Route",
      "id": 1331110731,
      "to": "Tverskaya ul 70",
      "from": "Kiyevskoye sh 100"
    },
    {
      "type": "Bus",
      "id": 1863191122,
      "name": "JIuzJYUk928u0jnQB"
    },
    {
      "type": "Stop",
      "id": 1901744419,
      "name": "Leningradskoye sh 60"
    },
    {
      "type": "Stop",
      "id": 341956560,
      "name": "Leningradskoye sh 50"
    },
    {
      "type": "Route",
      "id": 1335332897,
      "to": "Ostozhenka 100",
      "from": "Mezhdunarodnoye sh 60"
    },
    {
      "type": "Route",
      "id": 1465576236,
      "to": "Leningradskiy pr 60",
      "from": "Sokol"
    },
    {
      "type": "Bus",
      "id": 1081123661,
      "name": "65k"
    },
    {
      "type": "Stop",
      "id": 1971821751,
      "name": "Komsomolskiy pr 30"
    },
    {
      "type": "Bus",
      "id": 98583880,
      "name": "88k"
    },
    {
      "type": "Route",
      "id": 1958531615,
      "to": "Vnukovo",
      "from": "Komsomolskiy pr 20"
    },
    {
      "type": "Bus",
      "id": 54397640,
      "name": "78k"
    },
    {
      "type": "Route",
      "id": 2114083546,
      "to": "Pr Vernadskogo 90",
      "from": "Pr Vernadskogo 90"
    },
    {
      "type": "Route",
      "id": 2105118649,
      "to": "Tverskaya ul 20",
      "from": "Mokhovaya ul 100"
    },
    {
      "type": "Stop",
      "id": 164954391,
      "name": "Tverskaya ul 60"
    },
    {
      "type": "Bus",
      "id": 1033258466,
      "name": "Aorttv"
    },
    {
      "type": "Route",
      "id": 1788426184,
      "to": "Mokhovaya ul 10",
      "from": "Kiyevskoye sh 60"
    },
    {
      "type": "Route",
      "id": 474197957,
      "to": "Mezhdunarodnoye sh 20",
      "from": "Leningradskiy pr 30"
    },
    {
      "type": "Bus",
      "id": 1183335362,
      "name": "87k"
    },
    {
      "type": "Route",
      "id": 1775157758,
      "to": "Komsomolskiy pr 100",
      "from": "Leningradskoye sh 50"
    },
    {
      "type": "Bus",
      "id": 1517766680,
      "name": "52"
    },
    {
      "type": "Route",
      "id": 2078948068,
      "to": "Leningradskiy pr 70",
      "from": "Mezhdunarodnoye sh 40"
    },
    {
      "type": "Route",
      "id": 281712943,
      "to": "Komsomolskiy pr 60",
      "from": "Ostozhenka 50"
    },
    {
      "type": "Route",
      "id": 1494321937,
      "to": "Kiyevskoye sh 70",
      "from": "Mokhovaya ul 40"
    },
    {
      "type": "Route",
      "id": 118794132,
      "to": "Mezhdunarodnoye sh 60",
      "from": "Pr Vernadskogo 20"
    },
    {
      "type": "Bus",
      "id": 2064886899,
      "name": "95"
    },
    {
      "type": "Bus",
      "id": 421217921,
      "name": "5k"
    },
    {
      "type": "Route",
      "id": 1329068344,
      "to": "Kiyevskoye sh 50",
      "from": "Komsomolskiy pr 20"
    },
    {
      "type": "Route",
      "id": 2016964039,
      "to": "Ostozhenka 80",
      "from": "Mezhdunarodnoye sh 50"
    },
    {
      "type": "Stop",
      "id": 1770268632,
      "name": "Ostozhenka 90"
    },
    {
      "type": "Stop",
      "id": 1766990604,
      "name": "Leningradskiy pr 10"
    },
    {
      "type": "Route",
      "id": 1622645814,
      "to": "Vnukovo",
      "from": "Leningradskiy pr 100"
    },
    {
      "type": "Bus",
      "id": 1148160947,
      "name": "57k"
    },
    {
      "type": "Route",
      "id": 1017687170,
      "to": "Mokhovaya ul 80",
      "from": "Tverskaya ul 30"
    },
    {
      "type": "Bus",
      "id": 1024709661,
      "name": "53k"
    },
    {
      "type": "Bus",
      "id": 1081123661,
      "name": "65k"
    },
    {
      "type": "Route",
      "id": 1247292144,
      "to": "Ostozhenka 70",
      "from": "Belorusskiy vokzal"
    },
    {
      "type": "Route",
      "id": 838626365,
      "to": "Leningradskiy pr 90",
      "from": "Komsomolskiy pr 70"
    },
    {
      "type": "Stop",
      "id": 312162747,
      "name": "Mezhdunarodnoye sh 40"
    },
    {
      "type": "Route",
      "id": 168296313,
      "to": "Mokhovaya ul 50",
      "from": "Pr Vernadskogo 100"
    },
    {
      "type": "Route",
      "id": 1827741,
      "to": "Ostozhenka 50",
      "from": "Leningradskiy pr 100"
    },
    {
      "type": "Route",
      "id": 962331879,
      "to": "Ostozhenka 60",
      "from": "Kiyevskoye sh 40"
    },
    {
      "type": "Bus",
      "id": 819430422,
      "name": "19k"
    },
    {
      "type": "Route",
      "id": 2002109521,
      "to": "Pr Vernadskogo 70",
      "from": "Tverskaya ul 70"
    },
    {
      "type": "Route",
      "id": 489730113,
      "to": "Tverskaya ul 40",
      "from": "Ostozhenka 80"
    },
    {
      "type": "Route",
      "id": 2088121145,
      "to": "Tverskaya ul 100",
      "from": "Komsomolskiy pr 30"
    },
    {
      "type": "Route",
      "id": 1734410950,
      "to": "Kiyevskoye sh 30",
      "from": "Kiyevskoye sh 100"
    },
    {
      "type": "Route",
      "id": 1071230749,
      "to": "Pr Vernadskogo 70",
      "from": "Tverskaya ul 50"
    },
    {
      "type": "Route",
      "id": 1694924153,
      "to": "Sokol",
      "from": "Kiyevskoye sh 50"
    },
    {
      "type": "Route",
      "id": 1305240132,
      "to": "Mokhovaya ul 30",
      "from": "Mezhdunarodnoye sh 70"
    },
    {
      "type": "Route",
      "id": 30431305,
      "to": "Leningradskiy pr 10",
      "from": "Leningradskiy pr 30"
    },
    {
      "type": "Bus",
      "id": 144145560,
      "name": "ZP"
    },
    {
      "type": "Route",
      "id": 820514671,
      "to": "Leningradskoye sh 70",
      "from": "Komsomolskiy pr 50"
    },
    {
      "type": "Bus",
      "id": 1183335362,
      "name": "87k"
    },
    {
      "type": "Route",
      "id": 1333273840,
      "to": "Vnukovo",
      "from": "Vnukovo"
    },
    {
      "type": "Route",
      "id": 148305985,
      "to": "Komsomolskiy pr 50",
      "from": "Tverskaya ul 80"
    },
    {
      "type": "Stop",
      "id": 1374238609,
      "name": "Leningradskoye sh 40"
    },
    {
      "type": "Route",
      "id": 2143157425,
      "to": "Ostozhenka 100",
      "from": "Mezhdunarodnoye sh 10"
    },
    {
      "type": "Route",
      "id": 471718026,
      "to": "Pr Vernadskogo 10",
      "from": "Ostozhenka 70"
    },
    {
      "type": "Stop",
      "id": 1791067641,
      "name": "Mokhovaya ul 40"
    },
    {
      "type": "Route",
      "id": 560638864,
      "to": "Mezhdunarodnoye sh 50",
      "from": "Ostozhenka 70"
    },
    {
      "type": "Route",
      "id": 635669285,
      "to": "Mezhdunarodnoye sh 60",
      "from": "Tverskaya ul 60"
    },
    {
      "type": "Stop",
      "id": 2099103764,
      "name": "Pr Vernadskogo 60"
    },
    {
      "type": "Route",
      "id": 947748325,
      "to": "Mezhdunarodnoye sh 70",
      "from": "Leningradskiy pr 80"
    },
    {
      "type": "Bus",
      "id": 1148160947,
      "name": "57k"
    },
    {
      "type": "Stop",
      "id": 694034036,
      "name": "Kiyevskoye sh 50"
    },
    {
      "type": "Route",
      "id": 1085997167,
      "to": "Komsomolskiy pr 30",
      "from": "Khimki"
    },
    {
      "type": "Route",
      "id": 1920216273,
      "to": "Mezhdunarodnoye sh 10",
      "from": "Kiyevskoye sh 40"
    },
    {
      "type": "Route",
      "id": 1036261599,
      "to": "Kiyevskoye sh 50",
      "from": "Leningradskoye sh 50"
    },
    {
      "type": "Route",
      "id": 968534267,
      "to": "Leningradskiy pr 70",
      "from": "Kiyevskoye sh 50"
    },
    {
      "type": "Stop",
      "id": 2099103764,
      "name": "Pr Vernadskogo 60"
    },
    {
      "type": "Route",
      "id": 1049677286,
      "to": "Komsomolskiy pr 30",
      "from": "Mokhovaya ul 60"
    },
    {
      "type": "Stop",
      "id": 948168823,
      "name": "Mezhdunarodnoye sh 50"
    },
    {
      "type": "Bus",
      "id": 291021923,
      "name": "68k"
    },
    {
      "type": "Route",
      "id": 136547384,
      "to": "Pr Vernadskogo 100",
      "from": "Tverskaya ul 90"
    },
    {
      "type": "Stop",
      "id": 1640544668,
      "name": "Leningradskoye sh 70"
    },
    {
      "type": "Route",
      "id": 1872771806,
      "to": "Ostozhenka 80",
      "from": "Sokol"
    },
    {
      "type": "Stop",
      "id": 1794202133,
      "name": "Kiyevskoye sh 90"
    },
    {
      "type": "Bus",
      "id": 1459482213,
      "name": "42"
    },
    {
      "type": "Route",
      "id": 1369120560,
      "to": "Belorusskiy vokzal",
      "from": "Leningradskiy pr 80"
    },
    {
      "type": "Bus",
      "id": 1004571739,
      "name": "q8VBLTqPm2V5MFx"
    },
    {
      "type": "Route",
      "id": 594417709,
      "to": "Kiyevskoye sh 50",
      "from": "Tverskaya ul 40"
    },
    {
      "type": "Route",
      "id": 1354036958,
      "to": "Mokhovaya ul 100",
      "from": "Komsomolskiy pr 60"
    },
    {
      "type": "Route",
      "id": 1464116960,
      "to": "Leningradskiy pr 80",
      "from": "Tverskaya ul 90"
    },
    {
      "type": "Route",
      "id": 527093609,
      "to": "Tverskaya ul 10",
      "from": "Belorusskiy vokzal"
    },
    {
      "type": "Route",
      "id": 771309260,
      "to": "Tverskaya ul 60",
      "from": "Leningradskoye sh 70"
    },
    {
      "type": "Stop",
      "id": 2050326951,
      "name": "Ostozhenka 10"
    },
    {
      "type": "Bus",
      "id": 2102433046,
      "name": "21k"
    },
    {
      "type": "Bus",
      "id": 1183335362,
      "name": "87k"
    },
    {
      "type": "Route",
      "id": 1602166119,
      "to": "Leningradskiy pr 50",
      "from": "Kiyevskoye sh 80"
    },
    {
      "type": "Bus",
      "id": 1221092998,
      "name": "99k"
    },
    {
      "type": "Bus",
      "id": 1865859901,
      "name": "96"
    },
    {
      "type": "Route",
      "id": 1194015604,
      "to": "Komsomolskiy pr 40",
      "from": "Tverskaya ul 100"
    },
    {
      "type": "Route",
      "id": 441041,
      "to": "Ostozhenka 20",
      "from": "Ostozhenka 100"
    },
    {
      "type": "Route",
      "id": 1097647253,
      "to": "Komsomolskiy pr 30",
      "from": "Komsomolskiy pr 10"
    },
    {
      "type": "Route",
      "id": 1326923331,
      "to": "Pr Vernadskogo 10",
      "from": "Tverskaya ul 70"
    },
    {
      "type": "Bus",
      "id": 1742458421,
      "name": "44"
    },
    {
      "type": "Route",
      "id": 906540655,
      "to": "Pr Vernadskogo 90",
      "from": "Leningradskoye sh 20"
    },
    {
      "type": "Stop",
      "id": 2067887205,
      "name": "Mokhovaya ul 80"
    },
    {
      "type": "Route",
      "id": 930335539,
      "to": "Mezhdunarodnoye sh 30",
      "from": "Manezh"
    },
    {
      "type": "Stop",
      "id": 1464743061,
      "name": "Mokhovaya ul 90"
    },
    {
      "type": "Route",
      "id": 1552434902,
      "to": "Ostozhenka 20",
      "from": "Mezhdunarodnoye sh 70"
    },
    {
      "type": "Stop",
      "id": 661678309,
      "name": "Leningradskoye sh 10"
    },
    {
      "type": "Route",
      "id": 423679892,
      "to": "Mokhovaya ul 10",
      "from": "Leningradskoye sh 90"
    },
    {
      "type": "Route",
      "id": 1600920145,
      "to": "Vnukovo",
      "from": "Leningradskoye sh 10"
    },
    {
      "type": "Route",
      "id": 908946621,
      "to": "Leningradskoye sh 70",
      "from": "Pr Vernadskogo 90"
    },
    {
      "type": "Bus",
      "id": 941326940,
      "name": "8"
    },
    {
      "type": "Stop",
      "id": 825803312,
      "name": "Komsomolskiy pr 70"
    },
    {
      "type": "Stop",
      "id": 2003012636,
      "name": "PA4k"
    },
    {
      "type": "Bus",
      "id": 944928246,
      "name": "Fn1Q32bMRJUOk2edd"
    },
    {
      "type": "Route",
      "id": 795817439,
      "to": "Komsomolskiy pr 90",
      "from": "Mezhdunarodnoye sh 40"
    },
    {
      "type": "Bus",
      "id": 1972799637,
      "name": "77k"
    },
    {
      "type": "Route",
      "id": 441055139,
      "to": "Leningradskiy pr 70",
      "from": "Pr Vernadskogo 100"
    },
    {
      "type": "Route",
      "id": 767274838,
      "to": "Tverskaya ul 30",
      "from": "Mokhovaya ul 10"
    },
    {
      "type": "Route",
      "id": 360026903,
      "to": "Leningradskiy pr 20",
      "from": "Leningradskiy pr 100"
    },
    {
      "type": "Route",
      "id": 39945514,
      "to": "Mezhdunarodnoye sh 50",
      "from": "Mokhovaya ul 100"
    },
    {
      "type": "Bus",
      "id": 1006746075,
      "name": "hh"
    },
    {
      "type": "Bus",
      "id": 59207506,
      "name": "86k"
    },
    {
      "type": "Bus",
      "id": 1184095094,
      "name": "64k"
    },
    {
      "type": "Route",
      "id": 956404942,
      "to": "Leningradskiy pr 70",
      "from": "Mezhdunarodnoye sh 80"
    },
    {
      "type": "Route",
      "id": 549036999,
      "to": "Komsomolskiy pr 20",
      "from": "Mezhdunarodnoye sh 60"
    },
    {
      "type": "Route",
      "id": 1308286286,
      "to": "Troparyovo",
      "from": "Mezhdunarodnoye sh 30"
    },
    {
      "type": "Bus",
      "id": 1148160947,
      "name": "57k"
    },
    {
      "type": "Route",
      "id": 1819621461,
      "to": "Sheremetyevo",
      "from": "Mezhdunarodnoye sh 50"
    },
    {
      "type": "Route",
      "id": 1876961406,
      "to": "Sokol",
      "from": "Ostozhenka 50"
    },
    {
      "type": "Stop",
      "id": 1524276591,
      "name": "Kiyevskoye sh 70"
    },
    {
      "type": "Route",
      "id": 1967252288,
      "to": "Yandex",
      "from": "Komsomolskiy pr 100"
    },
    {
      "type": "Route",
      "id": 269921747,
      "to": "Leningradskiy pr 40",
      "from": "Pr Vernadskogo 10"
    },
    {
      "type": "Route",
      "id": 1450595876,
      "to": "Ostozhenka 20",
      "from": "Kiyevskoye sh 30"
    },
    {
      "type": "Route",
      "id": 733400803,
      "to": "Kiyevskoye sh 70",
      "from": "Mezhdunarodnoye sh 100"
    },
    {
      "type": "Route",
      "id": 1217157226,
      "to": "Leningradskoye sh 40",
      "from": "Tverskaya ul 30"
    },
    {
      "type": "Stop",
      "id": 672220528,
      "name": "Yga"
    },
    {
      "type": "Stop",
      "id": 164954391,
      "name": "Tverskaya ul 60"
    },
    {
      "type": "Route",
      "id": 893405403,
      "to": "Ostozhenka 60",
      "from": "Mokhovaya ul 30"
    },
    {
      "type": "Bus",
      "id": 1047704483,
      "name": "75k"
    },
    {
      "type": "Route",
      "id": 996927384,
      "to": "Kiyevskoye sh 50",
      "from": "Leningradskoye sh 60"
    },
    {
      "type": "Route",
      "id": 973813708,
      "to": "Kiyevskoye sh 10",
      "from": "Mokhovaya ul 10"
    },
    {
      "type": "Stop",
      "id": 1273991077,
      "name": "Kiyevskoye sh 10"
    },
    {
      "type": "Route",
      "id": 506233223,
      "to": "Pr Vernadskogo 10",
      "from": "Mokhovaya ul 70"
    },
    {
      "type": "Route",
      "id": 1669107757,
      "to": "Ostozhenka 10",
      "from": "Komsomolskiy pr 60"
    },
    {
      "type": "Stop",
      "id": 1640544668,
      "name": "Leningradskoye sh 70"
    },
    {
      "type": "Route",
      "id": 1683319824,
      "to": "Komsomolskiy pr 90",
      "from": "Mezhdunarodnoye sh 30"
    },
    {
      "type": "Stop",
      "id": 1787498175,
      "name": "Leningradskiy pr 80"
    },
    {
      "type": "Route",
      "id": 641658641,
      "to": "Mezhdunarodnoye sh 10",
      "from": "Ostozhenka 30"
    },
    {
      "type": "Bus",
      "id": 557108693,
      "name": "61k"
    },
    {
      "type": "Stop",
      "id": 699793610,
      "name": "Sheremetyevo"
    },
    {
      "type": "Bus",
      "id": 1517766680,
      "name": "52"
    },
    {
      "type": "Stop",
      "id": 1660040291,
      "name": "Komsomolskiy pr 20"
    },
    {
      "type": "Route",
      "id": 1245199734,
      "to": "Luzhniki",
      "from": "Tverskaya ul 80"
    },
    {
      "type": "Bus",
      "id": 801615852,
      "name": "89"
    },
    {
      "type": "Route",
      "id": 923428396,
      "to": "Mokhovaya ul 40",
      "from": "Tverskaya ul 40"
    },
    {
      "type": "Route",
      "id": 1625154687,
      "to": "Mokhovaya ul 80",
      "from": "Komsomolskiy pr 10"
    },
    {
      "type": "Route",
      "id": 111677296,
      "to": "Kiyevskoye sh 10",
      "from": "Mezhdunarodnoye sh 20"
    },
    {
      "type": "Route",
      "id": 1343347480,
      "to": "Leningradskiy pr 100",
      "from": "Tverskaya ul 60"
    },
    {
      "type": "Route",
      "id": 1080536571,
      "to": "Tverskaya ul 20",
      "from": "Komsomolskiy pr 20"
    },
    {
      "type": "Route",
      "id": 473310089,
      "to": "Kiyevskoye sh 20",
      "from": "Tverskaya ul 80"
    },
    {
      "type": "Bus",
      "id": 54397640,
      "name": "78k"
    },
    {
      "type": "Route",
      "id": 974756166,
      "to": "Komsomolskiy pr 40",
      "from": "Komsomolskiy pr 50"
    },
    {
      "type": "Bus",
      "id": 1221092998,
      "name": "99k"
    },
    {
      "type": "Route",
      "id": 807094840,
      "to": "Manezh",
      "from": "Leningradskoye sh 90"
    },
    {
      "type": "Route",
      "id": 813678091,
      "to": "Mezhdunarodnoye sh 50",
      "from": "Komsomolskiy pr 10"
    },
    {
      "type": "Route",
      "id": 427535589,
      "to": "Mezhdunarodnoye sh 60",
      "from": "Mokhovaya ul 80"
    },
    {
      "type": "Route",
      "id": 1889458715,
      "to": "Ostozhenka 50",
      "from": "Leningradskiy pr 20"
    },
    {
      "type": "Bus",
      "id": 1500331975,
      "name": "100k"
    },
    {
      "type": "Route",
      "id": 38284167,
      "to": "Ostozhenka 40",
      "from": "Pr Vernadskogo 20"
    },
    {
      "type": "Route",
      "id": 1649532140,
      "to": "Ostozhenka 30",
      "from": "Komsomolskiy pr 20"
    },
    {
      "type": "Route",
      "id": 851244683,
      "to": "Kiyevskoye sh 60",
      "from": "Komsomolskiy pr 10"
    },
    {
      "type": "Route",
      "id": 762201451,
      "to": "Ostozhenka 40",
      "from": "Mezhdunarodnoye sh 80"
    },
    {
      "type": "Route",
      "id": 752745326,
      "to": "Kiyevskoye sh 20",
      "from": "Komsomolskiy pr 70"
    },
    {
      "type": "Route",
      "id": 514823813,
      "to": "Mezhdunarodnoye sh 80",
      "from": "Kiyevskoye sh 100"
    },
    {
      "type": "Route",
      "id": 650702828,
      "to": "Mezhdunarodnoye sh 80",
      "from": "Tverskaya ul 70"
    },
    {
      "type": "Route",
      "id": 1591691769,
      "to": "Leningradskoye sh 50",
      "from": "Pr Vernadskogo 50"
    },
    {
      "type": "Route",
      "id": 1692461128,
      "to": "Kiyevskoye sh 40",
      "from": "Ostozhenka 90"
    },
    {
      "type": "Route",
      "id": 420577552,
      "to": "Mezhdunarodnoye sh 50",
      "from": "Leningradskoye sh 100"
    },
    {
      "type": "Route",
      "id": 1834608240,
      "to": "Leningradskoye sh 90",
      "from": "Sheremetyevo"
    },
    {
      "type": "Stop",
      "id": 2017557436,
      "name": "Komsomolskiy pr 90"
    },
    {
      "type": "Route",
      "id": 66204384,
      "to": "Kiyevskoye sh 80",
      "from": "Komsomolskiy pr 50"
    },
    {
      "type": "Bus",
      "id": 548886,
      "name": "91"
    },
    {
      "type": "Stop",
      "id": 694034036,
      "name": "Kiyevskoye sh 50"
    },
    {
      "type": "Stop",
      "id": 306795514,
      "name": "Leningradskiy pr 70"
    },
    {
      "type": "Bus",
      "id": 95205841,
      "name": "58"
    },
    {
      "type": "Stop",
      "id": 2099103764,
      "name": "Pr Vernadskogo 60"
    },
    {
      "type": "Route",
      "id": 515510018,
      "to": "Komsomolskiy pr 90",
      "from": "Ostozhenka 40"
    },
    {
      "type": "Route",
      "id": 346534677,
      "to": "Mokhovaya ul 50",
      "from": "Ostozhenka 10"
    },
    {
      "type": "Stop",
      "id": 1695724973,
      "name": "Pr Vernadskogo 90"
    },
    {
      "type": "Bus",
      "id": 1819075940,
      "name": "owc7fUXpg8Pbuu H4Ln2I"
    },
    {
      "type": "Route",
      "id": 1133941968,
      "to": "Pr Vernadskogo 70",
      "from": "Leningradskiy pr 60"
    },
    {
      "type": "Stop",
      "id": 1660040291,
      "name": "Komsomolskiy pr 20"
    },
    {
      "type": "Bus",
      "id": 167784857,
      "name": "Z5JNWfHGeZE"
    },
    {
      "type": "Route",
      "id": 892100349,
      "to": "Kiyevskoye sh 50",
      "from": "Ostozhenka 40"
    },
    {
      "type": "Route",
      "id": 1164649299,
      "to": "Komsomolskiy pr 20",
      "from": "Mezhdunarodnoye sh 20"
    },
    {
      "type": "Route",
      "id": 617759131,
      "to": "Pr Vernadskogo 10",
      "from": "Kiyevskoye sh 70"
    },
    {
      "type": "Bus",
      "id": 581508689,
      "name": "71k"
    },
    {
      "type": "Route",
      "id": 1621400756,
      "to": "Kiyevskoye sh 30",
      "from": "Tverskaya ul 30"
    },
    {
      "type": "Route",
      "id": 662433531,
      "to": "Pr Vernadskogo 80",
      "from": "Mokhovaya ul 20"
    },
    {
      "type": "Stop",
      "id": 1034325494,
      "name": "Mezhdunarodnoye sh 10"
    },
    {
      "type": "Bus",
      "id": 590459346,
      "name": "43k"
    },
    {
      "type": "Route",
      "id": 1142096703,
      "to": "Ostozhenka 50",
      "from": "Mokhovaya ul 20"
    },
    {
      "type": "Route",
      "id": 1229786866,
      "to": "Yandex",
      "from": "Tverskaya ul 50"
    },
    {
      "type": "Route",
      "id": 534678973,
      "to": "Mokhovaya ul 60",
      "from": "Mokhovaya ul 30"
    },
    {
      "type": "Bus",
      "id": 291021923,
      "name": "68k"
    },
    {
      "type": "Route",
      "id": 781739855,
      "to": "Komsomolskiy pr 40",
      "from": "Pr Vernadskogo 90"
    },
    {
      "type": "Bus",
      "id": 1926401502,
      "name": "33k"
    },
    {
      "type": "Bus",
      "id": 1926401502,
      "name": "33k"
    },
    {
      "type": "Bus",
      "id": 884992097,
      "name": "81"
    },
    {
      "type": "Route",
      "id": 568911420,
      "to": "Leningradskoye sh 20",
      "from": "Mezhdunarodnoye sh 70"
    },
    {
      "type": "Route",
      "id": 709688600,
      "to": "Kiyevskoye sh 60",
      "from": "Tverskaya ul 50"
    },
    {
      "type": "Route",
      "id": 244545246,
      "to": "Tverskaya ul 40",
      "from": "Komsomolskiy pr 100"
    },
    {
      "type": "Bus",
      "id": 557108693,
      "name": "61k"
    },
    {
      "type": "Route",
      "id": 1830931733,
      "to": "Troparyovo",
      "from": "Kiyevskoye sh 80"
    },
    {
      "type": "Bus",
      "id": 353672929,
      "name": "16k"
    },
    {
      "type": "Route",
      "id": 1193957677,
      "to": "Tverskaya ul 10",
      "from": "Tverskaya ul 50"
    },
    {
      "type": "Stop",
      "id": 1882953163,
      "name": "emcrXEB4fl21 MzWV"
    },
    {
      "type": "Stop",
      "id": 721281380,
      "name": "Leningradskiy pr 50"
    },
    {
      "type": "Route",
      "id": 1522980817,
      "to": "Komsomolskiy pr 40",
      "from": "Mokhovaya ul 60"
    },
    {
      "type": "Route",
      "id": 556512429,
      "to": "Mezhdunarodnoye sh 80",
      "from": "Leningradskiy pr 10"
    },
    {
      "type": "Route",
      "id": 1644219889,
      "to": "Khimki",
      "from": "Mezhdunarodnoye sh 90"
    },
    {
      "type": "Route",
      "id": 827657182,
      "to": "Leningradskoye sh 20",
      "from": "Leningradskoye sh 20"
    },
    {
      "type": "Route",
      "id": 305796688,
      "to": "Komsomolskiy pr 90",
      "from": "Luzhniki"
    },
    {
      "type": "Route",
      "id": 1871451991,
      "to": "Leningradskoye sh 20",
      "from": "Leningradskoye sh 10"
    },
    {
      "type": "Route",
      "id": 439683426,
      "to": "Mokhovaya ul 60",
      "from": "Sheremetyevo"
    },
    {
      "type": "Bus",
      "id": 34679142,
      "name": "45k"
    },
    {
      "type": "Route",
      "id": 1699161658,
      "to": "Pr Vernadskogo 100",
      "from": "Mokhovaya ul 90"
    },
    {
      "type": "Route",
      "id": 1192875134,
      "to": "Mezhdunarodnoye sh 70",
      "from": "Pr Vernadskogo 70"
    },
    {
      "type": "Route",
      "id": 356623278,
      "to": "Ostozhenka 100",
      "from": "Leningradskiy pr 80"
    },
    {
      "type": "Route",
      "id": 31590049,
      "to": "Leningradskiy pr 30",
      "from": "Mokhovaya ul 100"
    },
    {
      "type": "Stop",
      "id": 1770268632,
      "name": "Ostozhenka 90"
    },
    {
      "type": "Route",
      "id": 1605473120,
      "to": "Mokhovaya ul 10",
      "from": "Manezh"
    },
    {
      "type": "Route",
      "id": 836527406,
      "to": "Ostozhenka 100",
      "from": "Mokhovaya ul 40"
    },
    {
      "type": "Stop",
      "id": 844924949,
      "name": "Kiyevskoye sh 60"
    },
    {
      "type": "Route",
      "id": 743595768,
      "to": "Kiyevskoye sh 20",
      "from": "Tverskaya ul 60"
    },
    {
      "type": "Stop",
      "id": 1967639712,
      "name": "Vnukovo"
    },
    {
      "type": "Bus",
      "id": 59207506,
      "name": "86k"
    },
    {
      "type": "Route",
      "id": 1773432939,
      "to": "Kiyevskoye sh 100",
      "from": "Mokhovaya ul 50"
    },
    {
      "type": "Stop",
      "id": 1770268632,
      "name": "Ostozhenka 90"
    },
    {
      "type": "Route",
      "id": 1131740254,
      "to": "Tverskaya ul 40",
      "from": "Ostozhenka 70"
    },
    {
      "type": "Bus",
      "id": 1862530785,
      "name": "12"
    },
    {
      "type": "Route",
      "id": 1563301592,
      "to": "Mezhdunarodnoye sh 20",
      "from": "Leningradskoye sh 70"
    },
    {
      "type": "Route",
      "id": 1583266593,
      "to": "Ostozhenka 90",
      "from": "Kiyevskoye sh 20"
    },
    {
      "type": "Route",
      "id": 1499638163,
      "to": "Ostozhenka 90",
      "from": "Leningradskiy pr 70"
    },
    {
      "type": "Bus",
      "id": 1320378198,
      "name": "32"
    },
    {
      "type": "Route",
      "id": 1169135858,
      "to": "Komsomolskiy pr 100",
      "from": "Leningradskoye sh 10"
    },
    {
      "type": "Stop",
      "id": 17618307,
      "name": "T99wH owb6tzDZf"
    },
    {
      "type": "Route",
      "id": 61048544,
      "to": "Ostozhenka 80",
      "from": "Tverskaya ul 80"
    },
    {
      "type": "Stop",
      "id": 1396785474,
      "name": "Ostozhenka 40"
    },
    {
      "type": "Route",
      "id": 843990588,
      "to": "Ostozhenka 60",
      "from": "Mezhdunarodnoye sh 20"
    },
    {
      "type": "Route",
      "id": 355007968,
      "to": "Ostozhenka 60",
      "from": "Leningradskoye sh 80"
    },
    {
      "type": "Route",
      "id": 268132164,
      "to": "Kiyevskoye sh 70",
      "from": "Tverskaya ul 50"
    },
    {
      "type": "Route",
      "id": 157906026,
      "to": "Troparyovo",
      "from": "Komsomolskiy pr 10"
    },
    {
      "type": "Route",
      "id": 1420305798,
      "to": "Ostozhenka 60",
      "from": "Ostozhenka 60"
    },
    {
      "type": "Stop",
      "id": 1071355180,
      "name": "Pr Vernadskogo 20"
    },
    {
      "type": "Route",
      "id": 1014006556,
      "to": "Komsomolskiy pr 10",
      "from": "Kiyevskoye sh 40"
    },
    {
      "type": "Route",
      "id": 1636504837,
      "to": "Pr Vernadskogo 80",
      "from": "Komsomolskiy pr 70"
    },
    {
      "type": "Route",
      "id": 559814597,
      "to": "Pr Vernadskogo 50",
      "from": "Leningradskoye sh 40"
    },
    {
      "type": "Bus",
      "id": 106969856,
      "name": "41"
    },
    {
      "type": "Route",
      "id": 1649532140,
      "to": "Ostozhenka 30",
      "from": "Komsomolskiy pr 20"
    },
    {
      "type": "Route",
      "id": 920122796,
      "to": "Pr Vernadskogo 40",
      "from": "Pr Vernadskogo 100"
    },
    {
      "type": "Route",
      "id": 1468676724,
      "to": "Mezhdunarodnoye sh 30",
      "from": "Vnukovo"
    },
    {
      "type": "Bus",
      "id": 717590302,
      "name": "66"
    },
    {
      "type": "Bus",
      "id": 519279717,
      "name": "31k"
    },
    {
      "type": "Route",
      "id": 1299767270,
      "to": "Leningradskoye sh 60",
      "from": "Leningradskoye sh 40"
    },
    {
      "type": "Stop",
      "id": 1603176856,
      "name": "Leningradskoye sh 80"
    },
    {
      "type": "Route",
      "id": 974319668,
      "to": "Tverskaya ul 40",
      "from": "Komsomolskiy pr 60"
    },
    {
      "type": "Route",
      "id": 1492120225,
      "to": "Tverskaya ul 10",
      "from": "Leningradskiy pr 50"
    },
    {
      "type": "Route",
      "id": 660751696,
      "to": "Yandex",
      "from": "Leningradskoye sh 100"
    },
    {
      "type": "Route",
      "id": 1010179324,
      "to": "Tverskaya ul 60",
      "from": "Khimki"
    },
    {
      "type": "Route",
      "id": 2044724893,
      "to": "Yandex",
      "from": "Mezhdunarodnoye sh 90"
    },
    {
      "type": "Stop",
      "id": 1605701212,
      "name": "Komsomolskiy pr 10"
    },
    {
      "type": "Route",
      "id": 1334313765,
      "to": "Mezhdunarodnoye sh 80",
      "from": "Leningradskoye sh 100"
    },
    {
      "type": "Route",
      "id": 2121420556,
      "to": "Mokhovaya ul 80",
      "from": "Khimki"
    },
    {
      "type": "Stop",
      "id": 1834107789,
      "name": "Tverskaya ul 100"
    },
    {
      "type": "Bus",
      "id": 1920253003,
      "name": "30"
    },
    {
      "type": "Route",
      "id": 188795841,
      "to": "Pr Vernadskogo 100",
      "from": "Tverskaya ul 70"
    },
    {
      "type": "Route",
      "id": 1504781670,
      "to": "Komsomolskiy pr 20",
      "from": "Leningradskiy pr 60"
    },
    {
      "type": "Bus",
      "id": 353672929,
      "name": "16k"
    },
    {
      "type": "Route",
      "id": 579351549,
      "to": "Mezhdunarodnoye sh 70",
      "from": "Leningradskoye sh 70"
    },
    {
      "type": "Route",
      "id": 1764697862,
      "to": "Ostozhenka 70",
      "from": "Manezh"
    },
    {
      "type": "Route",
      "id": 1363942562,
      "to": "Komsomolskiy pr 40",
      "from": "Leningradskiy pr 40"
    },
    {
      "type": "Stop",
      "id": 1071355180,
      "name": "Pr Vernadskogo 20"
    },
    {
      "type": "Bus",
      "id": 514611074,
      "name": "36"
    },
    {
      "type": "Bus",
      "id": 1221092998,
      "name": "99k"
    },
    {
      "type": "Route",
      "id": 975900828,
      "to": "Komsomolskiy pr 100",
      "from": "Leningradskiy pr 30"
    },
    {
      "type": "Bus",
      "id": 130648184,
      "name": "17"
    },
    {
      "type": "Route",
      "id": 1022357730,
      "to": "Ostozhenka 60",
      "from": "Mokhovaya ul 90"
    },
    {
      "type": "Route",
      "id": 74334581,
      "to": "Mokhovaya ul 10",
      "from": "Mezhdunarodnoye sh 70"
    },
    {
      "type": "Bus",
      "id": 1472826063,
      "name": "98"
    },
    {
      "type": "Route",
      "id": 672169317,
      "to": "Leningradskoye sh 20",
      "from": "Leningradskiy pr 30"
    },
    {
      "type": "Route",
      "id": 1522006079,
      "to": "Mokhovaya ul 40",
      "from": "Komsomolskiy pr 10"
    },
    {
      "type": "Route",
      "id": 1548490274,
      "to": "Mezhdunarodnoye sh 100",
      "from": "Leningradskiy pr 70"
    },
    {
      "type": "Bus",
      "id": 98583880,
      "name": "88k"
    },
    {
      "type": "Route",
      "id": 1226432355,
      "to": "Leningradskiy pr 50",
      "from": "Mezhdunarodnoye sh 30"
    },
    {
      "type": "Route",
      "id": 118216444,
      "to": "Tverskaya ul 70",
      "from": "Kiyevskoye sh 60"
    },
    {
      "type": "Stop",
      "id": 304652610,
      "name": "CuMFliWA gObyoesaQ"
    },
    {
      "type": "Route",
      "id": 476411221,
      "to": "Pr Vernadskogo 10",
      "from": "Kiyevskoye sh 80"
    },
    {
      "type": "Bus",
      "id": 1459482213,
      "name": "42"
    },
    {
      "type": "Route",
      "id": 1826776579,
      "to": "Pr Vernadskogo 100",
      "from": "Tverskaya ul 60"
    },
    {
      "type": "Stop",
      "id": 1071630589,
      "name": "Ostozhenka 50"
    },
    {
      "type": "Bus",
      "id": 1387259836,
      "name": "51k"
    },
    {
      "type": "Stop",
      "id": 1464743061,
      "name": "Mokhovaya ul 90"
    },
    {
      "type": "Stop",
      "id": 1809770657,
      "name": "3jPGjoA2DeT8weKgkZ"
    },
    {
      "type": "Bus",
      "id": 1551351181,
      "name": "2k"
    },
    {
      "type": "Stop",
      "id": 1897104722,
      "name": "msiZ"
    },
    {
      "type": "Route",
      "id": 581107867,
      "to": "Kiyevskoye sh 70",
      "from": "Komsomolskiy pr 100"
    },
    {
      "type": "Bus",
      "id": 1184095094,
      "name": "64k"
    },
    {
      "type": "Route",
      "id": 825571668,
      "to": "Mokhovaya ul 40",
      "from": "Vnukovo"
    },
    {
      "type": "Bus",
      "id": 514611074,
      "name": "36"
    },
    {
      "type": "Route",
      "id": 2002257482,
      "to": "Leningradskoye sh 40",
      "from": "Leningradskoye sh 90"
    },
    {
      "type": "Route",
      "id": 1038124188,
      "to": "Leningradskiy pr 90",
      "from": "Mezhdunarodnoye sh 80"
    },
    {
      "type": "Stop",
      "id": 254220526,
      "name": "Khimki"
    },
    {
      "type": "Stop",
      "id": 1748756235,
      "name": "Komsomolskiy pr 100"
    },
    {
      "type": "Route",
      "id": 1733666485,
      "to": "Leningradskoye sh 70",
      "from": "Ostozhenka 20"
    },
    {
      "type": "Route",
      "id": 975900828,
      "to": "Komsomolskiy pr 100",
      "from": "Leningradskiy pr 30"
    },
    {
      "type": "Route",
      "id": 1614643306,
      "to": "Tverskaya ul 80",
      "from": "Mezhdunarodnoye sh 80"
    },
    {
      "type": "Stop",
      "id": 617979428,
      "name": "YB6BjGjV0G4mxOxIvShzGG"
    },
    {
      "type": "Stop",
      "id": 1791067641,
      "name": "Mokhovaya ul 40"
    },
    {
      "type": "Route",
      "id": 131271133,
      "to": "Leningradskiy pr 70",
      "from": "Ostozhenka 90"
    },
    {
      "type": "Route",
      "id": 1776392848,
      "to": "Leningradskoye sh 60",
      "from": "Pr Vernadskogo 70"
    },
    {
      "type": "Route",
      "id": 661692776,
      "to": "Leningradskoye sh 40",
      "from": "Leningradskoye sh 50"
    },
    {
      "type": "Route",
      "id": 1872771806,
      "to": "Ostozhenka 80",
      "from": "Sokol"
    },
    {
      "type": "Stop",
      "id": 1985217913,
      "name": "Pr Vernadskogo 70"
    },
    {
      "type": "Stop",
      "id": 440933152,
      "name": "Leningradskoye sh 20"
    },
    {
      "type": "Route",
      "id": 1933446366,
      "to": "Tverskaya ul 40",
      "from": "Leningradskoye sh 50"
    },
    {
      "type": "Route",
      "id": 1362635760,
      "to": "Sokol",
      "from": "Yandex"
    },
    {
      "type": "Route",
      "id": 1568457411,
      "to": "Manezh",
      "from": "Mokhovaya ul 40"
    },
    {
      "type": "Bus",
      "id": 1865859901,
      "name": "96"
    },
    {
      "type": "Stop",
      "id": 440933152,
      "name": "Leningradskoye sh 20"
    },
    {
      "type": "Stop",
      "id": 196230605,
      "name": "Leningradskiy pr 20"
    },
    {
      "type": "Stop",
      "id": 1787498175,
      "name": "Leningradskiy pr 80"
    },
    {
      "type": "Route",
      "id": 353992395,
      "to": "Leningradskoye sh 90",
      "from": "Leningradskiy pr 70"
    },
    {
      "type": "Route",
      "id": 1292728112,
      "to": "Leningradskoye sh 80",
      "from": "Kiyevskoye sh 50"
    },
    {
      "type": "Bus",
      "id": 1387259836,
      "name": "51k"
    },
    {
      "type": "Bus",
      "id": 590459346,
      "name": "43k"
    },
    {
      "type": "Route",
      "id": 1987458291,
      "to": "Leningradskoye sh 50",
      "from": "Leningradskiy pr 100"
    },
    {
      "type": "Stop",
      "id": 341956560,
      "name": "Leningradskoye sh 50"
    },
    {
      "type": "Route",
      "id": 1101144706,
      "to": "Kiyevskoye sh 10",
      "from": "Pr Vernadskogo 70"
    },
    {
      "type": "Route",
      "id": 594417709,
      "to": "Kiyevskoye sh 50",
      "from": "Tverskaya ul 40"
    },
    {
      "type": "Bus",
      "id": 49095282,
      "name": "82k"
    },
    {
      "type": "Stop",
      "id": 759524304,
      "name": "Mezhdunarodnoye sh 60"
    },
    {
      "type": "Route",
      "id": 1744633479,
      "to": "Tverskaya ul 50",
      "from": "Mokhovaya ul 30"
    },
    {
      "type": "Route",
      "id": 1284311982,
      "to": "Komsomolskiy pr 100",
      "from": "Mezhdunarodnoye sh 90"
    },
    {
      "type": "Route",
      "id": 842499802,
      "to": "Leningradskoye sh 80",
      "from": "Ostozhenka 60"
    },
    {
      "type": "Route",
      "id": 505914992,
      "to": "Leningradskiy pr 40",
      "from": "Kremlin"
    },
    {
      "type": "Stop",
      "id": 1390717557,
      "name": "Leningradskoye sh 30"
    },
    {
      "type": "Stop",
      "id": 1695724973,
      "name": "Pr Vernadskogo 90"
    },
    {
      "type": "Route",
      "id": 1700532403,
      "to": "Vnukovo",
      "from": "Leningradskoye sh 80"
    },
    {
      "type": "Route",
      "id": 1570266657,
      "to": "Komsomolskiy pr 20",
      "from": "Sokol"
    },
    {
      "type": "Route",
      "id": 1593512972,
      "to": "Ostozhenka 10",
      "from": "Tverskaya ul 60"
    },
    {
      "type": "Route",
      "id": 1473418498,
      "to": "Mezhdunarodnoye sh 40",
      "from": "Tverskaya ul 50"
    },
    {
      "type": "Route",
      "id": 1025757968,
      "to": "Komsomolskiy pr 20",
      "from": "Leningradskiy pr 90"
    },
    {
      "type": "Stop",
      "id": 825803312,
      "name": "Komsomolskiy pr 70"
    },
    {
      "type": "Route",
      "id": 598145070,
      "to": "Pr Vernadskogo 90",
      "from": "Mokhovaya ul 60"
    },
    {
      "type": "Bus",
      "id": 652318140,
      "name": "92k"
    },
    {
      "type": "Route",
      "id": 1747791179,
      "to": "Leningradskiy pr 20",
      "from": "Pr Vernadskogo 70"
    },
    {
      "type": "Route",
      "id": 2106617962,
      "to": "Leningradskiy pr 100",
      "from": "Leningradskoye sh 60"
    },
    {
      "type": "Stop",
      "id": 132064624,
      "name": "Komsomolskiy pr 50"
    },
    {
      "type": "Bus",
      "id": 49095282,
      "name": "82k"
    },
    {
      "type": "Route",
      "id": 17679429,
      "to": "Pr Vernadskogo 60",
      "from": "Mokhovaya ul 70"
    },
    {
      "type": "Route",
      "id": 894662056,
      "to": "Ostozhenka 60",
      "from": "Leningradskoye sh 10"
    },
    {
      "type": "Stop",
      "id": 825803312,
      "name": "Komsomolskiy pr 70"
    },
    {
      "type": "Route",
      "id": 1972079751,
      "to": "Mokhovaya ul 10",
      "from": "Ostozhenka 10"
    },
    {
      "type": "Route",
      "id": 1289241247,
      "to": "Belorusskiy vokzal",
      "from": "Manezh"
    },
    {
      "type": "Route",
      "id": 1323809994,
      "to": "Pr Vernadskogo 100",
      "from": "Mezhdunarodnoye sh 20"
    },
    {
      "type": "Stop",
      "id": 524832460,
      "name": "Leningradskiy pr 30"
    },
    {
      "type": "Stop",
      "id": 1242118002,
      "name": "Sokol"
    },
    {
      "type": "Stop",
      "id": 196230605,
      "name": "Leningradskiy pr 20"
    },
    {
      "type": "Bus",
      "id": 1387259836,
      "name": "51k"
    },
    {
      "type": "Route",
      "id": 846303195,
      "to": "Komsomolskiy pr 90",
      "from": "Troparyovo"
    },
    {
      "type": "Bus",
      "id": 1222823833,
      "name": "55k"
    },
    {
      "type": "Route",
      "id": 775123424,
      "to": "Komsomolskiy pr 30",
      "from": "Pr Vernadskogo 50"
    },
    {
      "type": "Bus",
      "id": 63424298,
      "name": "80k"
    },
    {
      "type": "Route",
      "id": 1269484092,
      "to": "Ostozhenka 70",
      "from": "Mezhdunarodnoye sh 70"
    },
    {
      "type": "Stop",
      "id": 331121298,
      "name": "Mokhovaya ul 20"
    },
    {
      "type": "Bus",
      "id": 76662407,
      "name": "72k"
    },
    {
      "type": "Route",
      "id": 1872771806,
      "to": "Ostozhenka 80",
      "from": "Sokol"
    },
    {
      "type": "Route",
      "id": 640705634,
      "to": "Leningradskoye sh 100",
      "from": "Leningradskiy pr 50"
    },
    {
      "type": "Route",
      "id": 1252380522,
      "to": "Mokhovaya ul 50",
      "from": "Leningradskoye sh 10"
    },
    {
      "type": "Stop",
      "id": 331121298,
      "name": "Mokhovaya ul 20"
    },
    {
      "type": "Route",
      "id": 591098729,
      "to": "Mezhdunarodnoye sh 10",
      "from": "Mokhovaya ul 70"
    },
    {
      "type": "Route",
      "id": 1174335662,
      "to": "Komsomolskiy pr 20",
      "from": "Ostozhenka 80"
    },
    {
      "type": "Route",
      "id": 272809646,
      "to": "Tverskaya ul 100",
      "from": "Leningradskiy pr 70"
    },
    {
      "type": "Route",
      "id": 131271133,
      "to": "Leningradskiy pr 70",
      "from": "Ostozhenka 90"
    },
    {
      "type": "Stop",
      "id": 1215181049,
      "name": "Tverskaya ul 40"
    },
    {
      "type": "Stop",
      "id": 1797256328,
      "name": "Pr Vernadskogo 100"
    },
    {
      "type": "Route",
      "id": 2037054186,
      "to": "Ostozhenka 80",
      "from": "Mezhdunarodnoye sh 20"
    },
    {
      "type": "Stop",
      "id": 578048491,
      "name": "Ostozhenka 30"
    },
    {
      "type": "Bus",
      "id": 1387259836,
      "name": "51k"
    },
    {
      "type": "Route",
      "id": 801996456,
      "to": "Mezhdunarodnoye sh 30",
      "from": "Mezhdunarodnoye sh 90"
    },
    {
      "type": "Route",
      "id": 672909960,
      "to": "Leningradskiy pr 90",
      "from": "Kiyevskoye sh 70"
    },
    {
      "type": "Route",
      "id": 525318572,
      "to": "Mokhovaya ul 20",
      "from": "Pr Vernadskogo 80"
    },
    {
      "type": "Route",
      "id": 1514651722,
      "to": "Kiyevskoye sh 90",
      "from": "Kiyevskoye sh 50"
    },
    {
      "type": "Route",
      "id": 222139304,
      "to": "Luzhniki",
      "from": "Mezhdunarodnoye sh 80"
    },
    {
      "type": "Route",
      "id": 440345996,
      "to": "Mezhdunarodnoye sh 100",
      "from": "Tverskaya ul 40"
    },
    {
      "type": "Route",
      "id": 902176040,
      "to": "Pr Vernadskogo 10",
      "from": "Mezhdunarodnoye sh 80"
    },
    {
      "type": "Route",
      "id": 1597314451,
      "to": "Mezhdunarodnoye sh 10",
      "from": "Mokhovaya ul 40"
    },
    {
      "type": "Bus",
      "id": 589555403,
      "name": "47k"
    },
    {
      "type": "Route",
      "id": 1212814019,
      "to": "Mokhovaya ul 10",
      "from": "Leningradskiy pr 10"
    },
    {
      "type": "Stop",
      "id": 2134777890,
      "name": "Luzhniki"
    },
    {
      "type": "Stop",
      "id": 1864267513,
      "name": "8ckOuTwrBVuJ"
    },
    {
      "type": "Route",
      "id": 553016268,
      "to": "Pr Vernadskogo 90",
      "from": "Mokhovaya ul 30"
    },
    {
      "type": "Route",
      "id": 1599851633,
      "to": "Mezhdunarodnoye sh 30",
      "from": "Tverskaya ul 70"
    },
    {
      "type": "Route",
      "id": 359322543,
      "to": "Kiyevskoye sh 40",
      "from": "Kiyevskoye sh 60"
    },
    {
      "type": "Route",
      "id": 1282280363,
      "to": "Leningradskiy pr 10",
      "from": "Leningradskoye sh 100"
    },
    {
      "type": "Stop",
      "id": 1921087375,
      "name": "Kiyevskoye sh 40"
    },
    {
      "type": "Route",
      "id": 1894399125,
      "to": "Komsomolskiy pr 30",
      "from": "Mokhovaya ul 50"
    },
    {
      "type": "Route",
      "id": 294163844,
      "to": "Pr Vernadskogo 50",
      "from": "Ostozhenka 20"
    },
    {
      "type": "Stop",
      "id": 1591051116,
      "name": "40UBmtadk"
    },
    {
      "type": "Route",
      "id": 489152038,
      "to": "Leningradskiy pr 70",
      "from": "Mezhdunarodnoye sh 30"
    },
    {
      "type": "Route",
      "id": 665370203,
      "to": "Leningradskiy pr 80",
      "from": "Mokhovaya ul 40"
    },
    {
      "type": "Route",
      "id": 1551733926,
      "to": "Mokhovaya ul 40",
      "from": "Ostozhenka 70"
    },
    {
      "type": "Route",
      "id": 1305921562,
      "to": "Ostozhenka 30",
      "from": "Ostozhenka 90"
    },
    {
      "type": "Route",
      "id": 785616617,
      "to": "Tverskaya ul 10",
      "from": "Leningradskoye sh 90"
    },
    {
      "type": "Route",
      "id": 1619501397,
      "to": "Komsomolskiy pr 10",
      "from": "Leningradskoye sh 10"
    },
    {
      "type": "Route",
      "id": 1678700050,
      "to": "Pr Vernadskogo 90",
      "from": "Sheremetyevo"
    },
    {
      "type": "Route",
      "id": 427324429,
      "to": "Ostozhenka 30",
      "from": "Leningradskoye sh 60"
    },
    {
      "type": "Stop",
      "id": 1797256328,
      "name": "Pr Vernadskogo 100"
    },
    {
      "type": "Route",
      "id": 986808931,
      "to": "Mezhdunarodnoye sh 70",
      "from": "Leningradskiy pr 20"
    },
    {
      "type": "Stop",
      "id": 1374238609,
      "name": "Leningradskoye sh 40"
    },
    {
      "type": "Route",
      "id": 1689877799,
      "to": "Ostozhenka 30",
      "from": "Sheremetyevo"
    },
    {
      "type": "Bus",
      "id": 63424298,
      "name": "80k"
    },
    {
      "type": "Route",
      "id": 1145158312,
      "to": "Leningradskiy pr 50",
      "from": "Leningradskiy pr 90"
    },
    {
      "type": "Stop",
      "id": 1901744419,
      "name": "Leningradskoye sh 60"
    },
    {
      "type": "Route",
      "id": 1182613399,
      "to": "Tverskaya ul 100",
      "from": "Mezhdunarodnoye sh 100"
    },
    {
      "type": "Route",
      "id": 969515705,
      "to": "Luzhniki",
      "from": "Pr Vernadskogo 40"
    },
    {
      "type": "Route",
      "id": 1287326210,
      "to": "Mezhdunarodnoye sh 30",
      "from": "Ostozhenka 10"
    },
    {
      "type": "Stop",
      "id": 285559190,
      "name": "Kiyevskoye sh 100"
    },
    {
      "type": "Bus",
      "id": 1551351181,
      "name": "2k"
    },
    {
      "type": "Stop",
      "id": 524832460,
      "name": "Leningradskiy pr 30"
    },
    {
      "type": "Stop",
      "id": 312162747,
      "name": "Mezhdunarodnoye sh 40"
    },
    {
      "type": "Route",
      "id": 771083358,
      "to": "Mezhdunarodnoye sh 100",
      "from": "Mokhovaya ul 50"
    },
    {
      "type": "Route",
      "id": 131611764,
      "to": "Pr Vernadskogo 40",
      "from": "Komsomolskiy pr 30"
    },
    {
      "type": "Route",
      "id": 1522006079,
      "to": "Mokhovaya ul 40",
      "from": "Komsomolskiy pr 10"
    },
    {
      "type": "Route",
      "id": 657255086,
      "to": "Tverskaya ul 100",
      "from": "Tverskaya ul 50"
    },
    {
      "type": "Stop",
      "id": 316546093,
      "name": "Mokhovaya ul 60"
    },
    {
      "type": "Route",
      "id": 1292728112,
      "to": "Leningradskoye sh 80",
      "from": "Kiyevskoye sh 50"
    },
    {
      "type": "Route",
      "id": 755494550,
      "to": "Pr Vernadskogo 30",
      "from": "Leningradskoye sh 80"
    },
    {
      "type": "Route",
      "id": 767745109,
      "to": "Pr Vernadskogo 70",
      "from": "Manezh"
    },
    {
      "type": "Route",
      "id": 1288555168,
      "to": "Mokhovaya ul 90",
      "from": "Ostozhenka 70"
    },
    {
      "type": "Route",
      "id": 938871172,
      "to": "Leningradskoye sh 80",
      "from": "Tverskaya ul 20"
    },
    {
      "type": "Bus",
      "id": 49095282,
      "name": "82k"
    },
    {
      "type": "Stop",
      "id": 694649906,
      "name": "Manezh"
    },
    {
      "type": "Route",
      "id": 2080481823,
      "to": "Tverskaya ul 40",
      "from": "Mokhovaya ul 100"
    },
    {
      "type": "Bus",
      "id": 941326940,
      "name": "8"
    },
    {
      "type": "Route",
      "id": 172931527,
      "to": "Leningradskoye sh 50",
      "from": "Kiyevskoye sh 80"
    },
    {
      "type": "Bus",
      "id": 76662407,
      "name": "72k"
    },
    {
      "type": "Stop",
      "id": 833334897,
      "name": "Mezhdunarodnoye sh 90"
    },
    {
      "type": "Bus",
      "id": 130648184,
      "name": "17"
    },
    {
      "type": "Route",
      "id": 1019402552,
      "to": "Tverskaya ul 20",
      "from": "Leningradskiy pr 90"
    },
    {
      "type": "Bus",
      "id": 214763116,
      "name": "27k"
    },
    {
      "type": "Route",
      "id": 1994073664,
      "to": "Tverskaya ul 90",
      "from": "Ostozhenka 70"
    },
    {
      "type": "Route",
      "id": 1799318259,
      "to": "Mokhovaya ul 30",
      "from": "Leningradskiy pr 70"
    },
    {
      "type": "Bus",
      "id": 725590944,
      "name": "1"
    },
    {
      "type": "Route",
      "id": 90297532,
      "to": "Leningradskiy pr 30",
      "from": "Vnukovo"
    },
    {
      "type": "Stop",
      "id": 1964458092,
      "name": "Troparyovo"
    },
    {
      "type": "Route",
      "id": 1735210429,
      "to": "Sokol",
      "from": "Ostozhenka 60"
    },
    {
      "type": "Route",
      "id": 81890313,
      "to": "Mezhdunarodnoye sh 90",
      "from": "Mokhovaya ul 40"
    },
    {
      "type": "Stop",
      "id": 774773839,
      "name": "Ostozhenka 60"
    },
    {
      "type": "Route",
      "id": 1607000115,
      "to": "Pr Vernadskogo 20",
      "from": "Kiyevskoye sh 60"
    },
    {
      "type": "Route",
      "id": 2107811644,
      "to": "Mezhdunarodnoye sh 50",
      "from": "Tverskaya ul 90"
    },
    {
      "type": "Stop",
      "id": 2077587727,
      "name": "Pr Vernadskogo 40"
    },
    {
      "type": "Bus",
      "id": 801615852,
      "name": "89"
    },
    {
      "type": "Bus",
      "id": 557108693,
      "name": "61k"
    },
    {
      "type": "Bus",
      "id": 903337307,
      "name": "56k"
    },
    {
      "type": "Stop",
      "id": 578048491,
      "name": "Ostozhenka 30"
    },
    {
      "type": "Route",
      "id": 1894399125,
      "to": "Komsomolskiy pr 30",
      "from": "Mokhovaya ul 50"
    },
    {
      "type": "Route",
      "id": 1577262218,
      "to": "Ostozhenka 30",
      "from": "Kiyevskoye sh 70"
    },
    {
      "type": "Route",
      "id": 2023511071,
      "to": "Ostozhenka 80",
      "from": "Troparyovo"
    },
    {
      "type": "Route",
      "id": 1886753533,
      "to": "Leningradskiy pr 80",
      "from": "Vnukovo"
    },
    {
      "type": "Route",
      "id": 1514105945,
      "to": "Leningradskiy pr 40",
      "from": "Leningradskiy pr 70"
    },
    {
      "type": "Bus",
      "id": 625048715,
      "name": "85k"
    },
    {
      "type": "Stop",
      "id": 1396785474,
      "name": "Ostozhenka 40"
    },
    {
      "type": "Route",
      "id": 1669804471,
      "to": "Komsomolskiy pr 50",
      "from": "Komsomolskiy pr 70"
    },
    {
      "type": "Route",
      "id": 1541079646,
      "to": "Pr Vernadskogo 80",
      "from": "Mokhovaya ul 10"
    },
    {
      "type": "Stop",
      "id": 1367453951,
      "name": "Ostozhenka 70"
    },
    {
      "type": "Route",
      "id": 1780189383,
      "to": "Pr Vernadskogo 50",
      "from": "Tverskaya ul 40"
    },
    {
      "type": "Route",
      "id": 1214786662,
      "to": "Leningradskoye sh 100",
      "from": "Kiyevskoye sh 60"
    },
    {
      "type": "Bus",
      "id": 1472826063,
      "name": "98"
    },
    {
      "type": "Route",
      "id": 445097612,
      "to": "Leningradskiy pr 20",
      "from": "Leningradskoye sh 30"
    },
    {
      "type": "Route",
      "id": 1422641891,
      "to": "Mokhovaya ul 30",
      "from": "Tverskaya ul 80"
    },
    {
      "type": "Bus",
      "id": 396086334,
      "name": "BwhPf"
    },
    {
      "type": "Route",
      "id": 894285338,
      "to": "Mezhdunarodnoye sh 70",
      "from": "Kiyevskoye sh 60"
    },
    {
      "type": "Stop",
      "id": 2053097921,
      "name": "Pu8lU"
    },
    {
      "type": "Route",
      "id": 1157298383,
      "to": "Vnukovo",
      "from": "Tverskaya ul 10"
    },
    {
      "type": "Route",
      "id": 1025711819,
      "to": "Komsomolskiy pr 40",
      "from": "Komsomolskiy pr 80"
    },
    {
      "type": "Route",
      "id": 951422942,
      "to": "Leningradskoye sh 20",
      "from": "Tverskaya ul 70"
    },
    {
      "type": "Route",
      "id": 1047585148,
      "to": "Mezhdunarodnoye sh 80",
      "from": "Kiyevskoye sh 60"
    },
    {
      "type": "Bus",
      "id": 1997217022,
      "name": "83k"
    },
    {
      "type": "Bus",
      "id": 98583880,
      "name": "88k"
    },
    {
      "type": "Route",
      "id": 1194889440,
      "to": "Ostozhenka 80",
      "from": "Mokhovaya ul 60"
    },
    {
      "type": "Stop",
      "id": 1787498175,
      "name": "Leningradskiy pr 80"
    },
    {
      "type": "Bus",
      "id": 519279717,
      "name": "31k"
    },
    {
      "type": "Stop",
      "id": 1034325494,
      "name": "Mezhdunarodnoye sh 10"
    },
    {
      "type": "Route",
      "id": 441155840,
      "to": "Komsomolskiy pr 20",
      "from": "Ostozhenka 70"
    },
    {
      "type": "Route",
      "id": 1228472651,
      "to": "Kiyevskoye sh 30",
      "from": "Mokhovaya ul 70"
    },
    {
      "type": "Bus",
      "id": 1391108907,
      "name": "4"
    },
    {
      "type": "Route",
      "id": 1228472651,
      "to": "Kiyevskoye sh 30",
      "from": "Mokhovaya ul 70"
    },
    {
      "type": "Route",
      "id": 2136600268,
      "to": "Kiyevskoye sh 100",
      "from": "Leningradskoye sh 80"
    },
    {
      "type": "Route",
      "id": 552367260,
      "to": "Leningradskiy pr 80",
      "from": "Komsomolskiy pr 10"
    },
    {
      "type": "Stop",
      "id": 660769553,
      "name": "RtX1gviE59udvC1IaYFZY"
    },
    {
      "type": "Route",
      "id": 686135172,
      "to": "Kiyevskoye sh 70",
      "from": "Komsomolskiy pr 50"
    },
    {
      "type": "Stop",
      "id": 1794202133,
      "name": "Kiyevskoye sh 90"
    },
    {
      "type": "Route",
      "id": 1886514344,
      "to": "Pr Vernadskogo 30",
      "from": "Tverskaya ul 30"
    },
    {
      "type": "Route",
      "id": 829199655,
      "to": "Mokhovaya ul 90",
      "from": "Luzhniki"
    },
    {
      "type": "Bus",
      "id": 2086679848,
      "name": "73k"
    },
    {
      "type": "Route",
      "id": 1965363194,
      "to": "Belorusskiy vokzal",
      "from": "Pr Vernadskogo 20"
    },
    {
      "type": "Route",
      "id": 1149053367,
      "to": "Komsomolskiy pr 10",
      "from": "Troparyovo"
    },
    {
      "type": "Stop",
      "id": 312162747,
      "name": "Mezhdunarodnoye sh 40"
    },
    {
      "type": "Stop",
      "id": 1913619708,
      "name": "Komsomolskiy pr 60"
    },
    {
      "type": "Route",
      "id": 681170061,
      "to": "Leningradskoye sh 50",
      "from": "Leningradskoye sh 100"
    },
    {
      "type": "Stop",
      "id": 196230605,
      "name": "Leningradskiy pr 20"
    },
    {
      "type": "Stop",
      "id": 978815997,
      "name": "Mokhovaya ul 30"
    },
    {
      "type": "Stop",
      "id": 859244554,
      "name": "dNuST"
    },
    {
      "type": "Route",
      "id": 1299594861,
      "to": "Pr Vernadskogo 70",
      "from": "Leningradskiy pr 50"
    },
    {
      "type": "Route",
      "id": 922764073,
      "to": "Komsomolskiy pr 80",
      "from": "Kiyevskoye sh 50"
    },
    {
      "type": "Bus",
      "id": 1532241955,
      "name": "7k"
    },
    {
      "type": "Stop",
      "id": 694034036,
      "name": "Kiyevskoye sh 50"
    },
    {
      "type": "Route",
      "id": 1976474729,
      "to": "Leningradskoye sh 50",
      "from": "Tverskaya ul 50"
    },
    {
      "type": "Bus",
      "id": 63424298,
      "name": "80k"
    },
    {
      "type": "Bus",
      "id": 548886,
      "name": "91"
    },
    {
      "type": "Route",
      "id": 1907125718,
      "to": "Leningradskoye sh 60",
      "from": "Mokhovaya ul 20"
    },
    {
      "type": "Route",
      "id": 634588812,
      "to": "Mokhovaya ul 100",
      "from": "Mokhovaya ul 70"
    },
    {
      "type": "Route",
      "id": 662984248,
      "to": "Mokhovaya ul 30",
      "from": "Komsomolskiy pr 30"
    },
    {
      "type": "Route",
      "id": 909337693,
      "to": "Ostozhenka 40",
      "from": "Kiyevskoye sh 100"
    },
    {
      "type": "Bus",
      "id": 1152814312,
      "name": "3k"
    },
    {
      "type": "Route",
      "id": 2006498166,
      "to": "Tverskaya ul 60",
      "from": "Ostozhenka 60"
    },
    {
      "type": "Stop",
      "id": 254220526,
      "name": "Khimki"
    },
    {
      "type": "Route",
      "id": 2052578202,
      "to": "Tverskaya ul 80",
      "from": "Ostozhenka 100"
    },
    {
      "type": "Route",
      "id": 203834589,
      "to": "Ostozhenka 40",
      "from": "Kremlin"
    },
    {
      "type": "Bus",
      "id": 799165931,
      "name": "24k"
    },
    {
      "type": "Route",
      "id": 1317812821,
      "to": "Mezhdunarodnoye sh 90",
      "from": "Tverskaya ul 90"
    },
    {
      "type": "Route",
      "id": 229541379,
      "to": "Mokhovaya ul 30",
      "from": "Ostozhenka 20"
    },
    {
      "type": "Stop",
      "id": 474720704,
      "name": "Pr Vernadskogo 50"
    },
    {
      "type": "Route",
      "id": 1200223479,
      "to": "Leningradskiy pr 90",
      "from": "Mokhovaya ul 50"
    },
    {
      "type": "Route",
      "id": 1931867777,
      "to": "Leningradskiy pr 100",
      "from": "Sheremetyevo"
    },
    {
      "type": "Route",
      "id": 344119623,
      "to": "Pr Vernadskogo 60",
      "from": "Tverskaya ul 60"
    },
    {
      "type": "Route",
      "id": 1576614856,
      "to": "Pr Vernadskogo 30",
      "from": "Ostozhenka 70"
    },
    {
      "type": "Stop",
      "id": 1273991077,
      "name": "Kiyevskoye sh 10"
    },
    {
      "type": "Route",
      "id": 1762626314,
      "to": "Sheremetyevo",
      "from": "Pr Vernadskogo 80"
    },
    {
      "type": "Route",
      "id": 92520865,
      "to": "Tverskaya ul 40",
      "from": "Tverskaya ul 70"
    },
    {
      "type": "Route",
      "id": 1929560046,
      "to": "Ostozhenka 90",
      "from": "Pr Vernadskogo 30"
    },
    {
      "type": "Stop",
      "id": 1524276591,
      "name": "Kiyevskoye sh 70"
    },
    {
      "type": "Route",
      "id": 200271764,
      "to": "Kiyevskoye sh 60",
      "from": "Ostozhenka 90"
    },
    {
      "type": "Route",
      "id": 1028603880,
      "to": "Mokhovaya ul 90",
      "from": "Ostozhenka 80"
    },
    {
      "type": "Route",
      "id": 2079320993,
      "to": "Kremlin",
      "from": "Mezhdunarodnoye sh 100"
    },
    {
      "type": "Stop",
      "id": 759524304,
      "name": "Mezhdunarodnoye sh 60"
    },
    {
      "type": "Stop",
      "id": 1660040291,
      "name": "Komsomolskiy pr 20"
    },
    {
      "type": "Route",
      "id": 547207033,
      "to": "Leningradskoye sh 80",
      "from": "Komsomolskiy pr 80"
    },
    {
      "type": "Route",
      "id": 1253490809,
      "to": "Komsomolskiy pr 30",
      "from": "Komsomolskiy pr 80"
    },
    {
      "type": "Bus",
      "id": 421217921,
      "name": "5k"
    },
    {
      "type": "Route",
      "id": 785938231,
      "to": "Tverskaya ul 100",
      "from": "Komsomolskiy pr 60"
    },
    {
      "type": "Route",
      "id": 1492321719,
      "to": "Kiyevskoye sh 70",
      "from": "Ostozhenka 30"
    },
    {
      "type": "Bus",
      "id": 1926401502,
      "name": "33k"
    },
    {
      "type": "Route",
      "id": 1031663553,
      "to": "Kiyevskoye sh 100",
      "from": "Leningradskiy pr 90"
    },
    {
      "type": "Stop",
      "id": 1971821751,
      "name": "Komsomolskiy pr 30"
    },
    {
      "type": "Bus",
      "id": 1081123661,
      "name": "65k"
    },
    {
      "type": "Stop",
      "id": 1794202133,
      "name": "Kiyevskoye sh 90"
    },
    {
      "type": "Route",
      "id": 446703164,
      "to": "Ostozhenka 90",
      "from": "Kremlin"
    },
    {
      "type": "Bus",
      "id": 1960392737,
      "name": "13"
    },
    {
      "type": "Bus",
      "id": 297028842,
      "name": "10k"
    },
    {
      "type": "Stop",
      "id": 1034325494,
      "name": "Mezhdunarodnoye sh 10"
    },
    {
      "type": "Route",
      "id": 1720848083,
      "to": "Tverskaya ul 70",
      "from": "Luzhniki"
    },
    {
      "type": "Route",
      "id": 60528849,
      "to": "Mokhovaya ul 50",
      "from": "Pr Vernadskogo 40"
    },
    {
      "type": "Stop",
      "id": 774773839,
      "name": "Ostozhenka 60"
    },
    {
      "type": "Route",
      "id": 1380811995,
      "to": "Kremlin",
      "from": "Leningradskoye sh 10"
    },
    {
      "type": "Bus",
      "id": 39536936,
      "name": "60"
    },
    {
      "type": "Stop",
      "id": 1430505565,
      "name": "yRvQv"
    },
    {
      "type": "Route",
      "id": 1979666903,
      "to": "Tverskaya ul 30",
      "from": "Mezhdunarodnoye sh 100"
    },
    {
      "type": "Bus",
      "id": 1972799637,
      "name": "77k"
    },
    {
      "type": "Stop",
      "id": 1922284939,
      "name": "Wys1tcXeaBRSVkBz"
    },
    {
      "type": "Route",
      "id": 302603381,
      "to": "Komsomolskiy pr 60",
      "from": "Mokhovaya ul 70"
    },
    {
      "type": "Bus",
      "id": 353672929,
      "name": "16k"
    },
    {
      "type": "Bus",
      "id": 725590944,
      "name": "1"
    },
    {
      "type": "Route",
      "id": 2007704220,
      "to": "Troparyovo",
      "from": "Mezhdunarodnoye sh 80"
    },
    {
      "type": "Route",
      "id": 1685873292,
      "to": "Tverskaya ul 70",
      "from": "Ostozhenka 100"
    },
    {
      "type": "Route",
      "id": 1018242148,
      "to": "Yandex",
      "from": "Pr Vernadskogo 60"
    },
    {
      "type": "Route",
      "id": 1693395457,
      "to": "Leningradskiy pr 100",
      "from": "Ostozhenka 80"
    },
    {
      "type": "Stop",
      "id": 1054827034,
      "name": "Tverskaya ul 10"
    },
    {
      "type": "Bus",
      "id": 39536936,
      "name": "60"
    },
    {
      "type": "Bus",
      "id": 618630191,
      "name": "NMX"
    },
    {
      "type": "Bus",
      "id": 1671271267,
      "name": "59"
    },
    {
      "type": "Bus",
      "id": 557108693,
      "name": "61k"
    },
    {
      "type": "Stop",
      "id": 694034036,
      "name": "Kiyevskoye sh 50"
    },
    {
      "type": "Route",
      "id": 979189607,
      "to": "Tverskaya ul 90",
      "from": "Sokol"
    },
    {
      "type": "Stop",
      "id": 1321456988,
      "name": "Mezhdunarodnoye sh 100"
    },
    {
      "type": "Route",
      "id": 324117984,
      "to": "Leningradskoye sh 100",
      "from": "Komsomolskiy pr 30"
    },
    {
      "type": "Bus",
      "id": 799165931,
      "name": "24k"
    },
    {
      "type": "Route",
      "id": 1296889851,
      "to": "Manezh",
      "from": "Pr Vernadskogo 100"
    },
    {
      "type": "Route",
      "id": 1466315783,
      "to": "Tverskaya ul 60",
      "from": "Pr Vernadskogo 30"
    },
    {
      "type": "Route",
      "id": 454217366,
      "to": "Ostozhenka 70",
      "from": "Ostozhenka 60"
    },
    {
      "type": "Stop",
      "id": 132064624,
      "name": "Komsomolskiy pr 50"
    },
    {
      "type": "Route",
      "id": 2022631968,
      "to": "Leningradskoye sh 30",
      "from": "Mezhdunarodnoye sh 100"
    },
    {
      "type": "Route",
      "id": 1149594362,
      "to": "Kiyevskoye sh 100",
      "from": "Kiyevskoye sh 20"
    },
    {
      "type": "Route",
      "id": 819050735,
      "to": "Komsomolskiy pr 90",
      "from": "Leningradskoye sh 30"
    },
    {
      "type": "Route",
      "id": 1138644023,
      "to": "Kiyevskoye sh 100",
      "from": "Leningradskoye sh 20"
    },
    {
      "type": "Route",
      "id": 82568547,
      "to": "Luzhniki",
      "from": "Belorusskiy vokzal"
    },
    {
      "type": "Route",
      "id": 1266412415,
      "to": "Komsomolskiy pr 80",
      "from": "Mokhovaya ul 20"
    },
    {
      "type": "Route",
      "id": 856844879,
      "to": "Ostozhenka 30",
      "from": "Ostozhenka 60"
    },
    {
      "type": "Bus",
      "id": 589555403,
      "name": "47k"
    },
    {
      "type": "Route",
      "id": 550745790,
      "to": "Pr Vernadskogo 80",
      "from": "Leningradskiy pr 80"
    },
    {
      "type": "Route",
      "id": 618573386,
      "to": "Pr Vernadskogo 50",
      "from": "Leningradskoye sh 50"
    },
    {
      "type": "Route",
      "id": 1388261527,
      "to": "Mezhdunarodnoye sh 50",
      "from": "Tverskaya ul 10"
    },
    {
      "type": "Route",
      "id": 1079547543,
      "to": "Pr Vernadskogo 60",
      "from": "Tverskaya ul 30"
    },
    {
      "type": "Route",
      "id": 1205248473,
      "to": "Tverskaya ul 50",
      "from": "Leningradskiy pr 100"
    },
    {
      "type": "Route",
      "id": 1595696270,
      "to": "Mokhovaya ul 40",
      "from": "Leningradskiy pr 60"
    },
    {
      "type": "Route",
      "id": 1902364582,
      "to": "Leningradskoye sh 90",
      "from": "Leningradskoye sh 20"
    },
    {
      "type": "Route",
      "id": 463423951,
      "to": "Kiyevskoye sh 20",
      "from": "Luzhniki"
    },
    {
      "type": "Stop",
      "id": 833334897,
      "name": "Mezhdunarodnoye sh 90"
    },
    {
      "type": "Stop",
      "id": 1034325494,
      "name": "Mezhdunarodnoye sh 10"
    },
    {
      "type": "Route",
      "id": 847179380,
      "to": "Pr Vernadskogo 60",
      "from": "Pr Vernadskogo 60"
    },
    {
      "type": "Route",
      "id": 1538865978,
      "to": "Mokhovaya ul 60",
      "from": "Mokhovaya ul 100"
    },
    {
      "type": "Stop",
      "id": 1680438344,
      "name": "Leningradskiy pr 90"
    },
    {
      "type": "Stop",
      "id": 2077587727,
      "name": "Pr Vernadskogo 40"
    },
    {
      "type": "Bus",
      "id": 32878382,
      "name": "79"
    },
    {
      "type": "Route",
      "id": 1840201070,
      "to": "Kiyevskoye sh 60",
      "from": "Komsomolskiy pr 100"
    },
    {
      "type": "Bus",
      "id": 819430422,
      "name": "19k"
    },
    {
      "type": "Route",
      "id": 1005036060,
      "to": "Mokhovaya ul 50",
      "from": "Komsomolskiy pr 90"
    },
    {
      "type": "Route",
      "id": 2011929347,
      "to": "Luzhniki",
      "from": "Pr Vernadskogo 30"
    },
    {
      "type": "Stop",
      "id": 1321456988,
      "name": "Mezhdunarodnoye sh 100"
    },
    {
      "type": "Route",
      "id": 1302063782,
      "to": "Komsomolskiy pr 80",
      "from": "Mezhdunarodnoye sh 100"
    },
    {
      "type": "Bus",
      "id": 788681752,
      "name": "fvMbmRckhHDi4eg BKm9HW"
    },
    {
      "type": "Bus",
      "id": 625048715,
      "name": "85k"
    },
    {
      "type": "Route",
      "id": 931067281,
      "to": "Mezhdunarodnoye sh 50",
      "from": "Tverskaya ul 50"
    },
    {
      "type": "Stop",
      "id": 196230605,
      "name": "Leningradskiy pr 20"
    },
    {
      "type": "Bus",
      "id": 1124213689,
      "name": "25k"
    },
    {
      "type": "Route",
      "id": 1019704413,
      "to": "Komsomolskiy pr 50",
      "from": "Leningradskiy pr 70"
    },
    {
      "type": "Route",
      "id": 802549437,
      "to": "Ostozhenka 10",
      "from": "Khimki"
    },
    {
      "type": "Route",
      "id": 532796977,
      "to": "Leningradskiy pr 20",
      "from": "Mokhovaya ul 60"
    },
    {
      "type": "Bus",
      "id": 267670207,
      "name": "hXbXMJDrGBeEK5MqtM"
    },
    {
      "type": "Stop",
      "id": 1847557780,
      "name": "Mezhdunarodnoye sh 30"
    },
    {
      "type": "Stop",
      "id": 1605701212,
      "name": "Komsomolskiy pr 10"
    },
    {
      "type": "Route",
      "id": 313776932,
      "to": "Tverskaya ul 60",
      "from": "Mokhovaya ul 100"
    },
    {
      "type": "Route",
      "id": 1333734037,
      "to": "Mokhovaya ul 60",
      "from": "Tverskaya ul 60"
    },
    {
      "type": "Route",
      "id": 2145909384,
      "to": "Yandex",
      "from": "Sheremetyevo"
    },
    {
      "type": "Route",
      "id": 706531486,
      "to": "Leningradskoye sh 60",
      "from": "Tverskaya ul 50"
    },
    {
      "type": "Route",
      "id": 922764073,
      "to": "Komsomolskiy pr 80",
      "from": "Kiyevskoye sh 50"
    },
    {
      "type": "Route",
      "id": 1004052378,
      "to": "Leningradskiy pr 70",
      "from": "Pr Vernadskogo 60"
    },
    {
      "type": "Route",
      "id": 746357193,
      "to": "Komsomolskiy pr 30",
      "from": "Kiyevskoye sh 20"
    },
    {
      "type": "Route",
      "id": 2040155342,
      "to": "Mokhovaya ul 50",
      "from": "Leningradskoye sh 70"
    },
    {
      "type": "Route",
      "id": 1358420226,
      "to": "Troparyovo",
      "from": "Mokhovaya ul 20"
    },
    {
      "type": "Route",
      "id": 480962684,
      "to": "Ostozhenka 100",
      "from": "Kiyevskoye sh 60"
    },
    {
      "type": "Route",
      "id": 989156908,
      "to": "Leningradskoye sh 100",
      "from": "Leningradskiy pr 40"
    },
    {
      "type": "Stop",
      "id": 306795514,
      "name": "Leningradskiy pr 70"
    },
    {
      "type": "Route",
      "id": 1124241248,
      "to": "Kiyevskoye sh 100",
      "from": "Leningradskoye sh 90"
    },
    {
      "type": "Route",
      "id": 1442399506,
      "to": "Kremlin",
      "from": "Leningradskoye sh 40"
    },
    {
      "type": "Route",
      "id": 648188180,
      "to": "Ostozhenka 90",
      "from": "Pr Vernadskogo 10"
    },
    {
      "type": "Stop",
      "id": 2059621403,
      "name": "Kiyevskoye sh 30"
    },
    {
      "type": "Bus",
      "id": 1926401502,
      "name": "33k"
    },
    {
      "type": "Stop",
      "id": 694034036,
      "name": "Kiyevskoye sh 50"
    },
    {
      "type": "Route",
      "id": 1869482967,
      "to": "Komsomolskiy pr 30",
      "from": "Mezhdunarodnoye sh 50"
    },
    {
      "type": "Bus",
      "id": 1551351181,
      "name": "2k"
    },
    {
      "type": "Route",
      "id": 329809143,
      "to": "Mezhdunarodnoye sh 40",
      "from": "Ostozhenka 40"
    },
    {
      "type": "Route",
      "id": 990066492,
      "to": "Ostozhenka 80",
      "from": "Komsomolskiy pr 20"
    },
    {
      "type": "Stop",
      "id": 570491200,
      "name": "Leningradskoye sh 90"
    },
    {
      "type": "Stop",
      "id": 1071355180,
      "name": "Pr Vernadskogo 20"
    },
    {
      "type": "Stop",
      "id": 699793610,
      "name": "Sheremetyevo"
    },
    {
      "type": "Bus",
      "id": 59207506,
      "name": "86k"
    },
    {
      "type": "Route",
      "id": 1288747694,
      "to": "Komsomolskiy pr 100",
      "from": "Kiyevskoye sh 50"
    },
    {
      "type": "Route",
      "id": 1838791537,
      "to": "Mokhovaya ul 20",
      "from": "Komsomolskiy pr 40"
    },
    {
      "type": "Route",
      "id": 167273124,
      "to": "Leningradskoye sh 10",
      "from": "Tverskaya ul 90"
    },
    {
      "type": "Bus",
      "id": 1574480338,
      "name": "70k"
    },
    {
      "type": "Route",
      "id": 390840847,
      "to": "Mokhovaya ul 50",
      "from": "Leningradskoye sh 30"
    },
    {
      "type": "Route",
      "id": 1041325794,
      "to": "Tverskaya ul 20",
      "from": "Leningradskoye sh 10"
    },
    {
      "type": "Stop",
      "id": 1901744419,
      "name": "Leningradskoye sh 60"
    },
    {
      "type": "Route",
      "id": 1033770026,
      "to": "Tverskaya ul 100",
      "from": "Komsomolskiy pr 100"
    },
    {
      "type": "Route",
      "id": 931316490,
      "to": "Leningradskoye sh 20",
      "from": "Leningradskoye sh 30"
    },
    {
      "type": "Bus",
      "id": 1926401502,
      "name": "33k"
    },
    {
      "type": "Route",
      "id": 2142754102,
      "to": "Tverskaya ul 10",
      "from": "Mokhovaya ul 90"
    },
    {
      "type": "Bus",
      "id": 2064455607,
      "name": "76k"
    },
    {
      "type": "Route",
      "id": 177809578,
      "to": "Komsomolskiy pr 20",
      "from": "Mokhovaya ul 70"
    },
    {
      "type": "Route",
      "id": 1346423766,
      "to": "Pr Vernadskogo 80",
      "from": "Kiyevskoye sh 50"
    },
    {
      "type": "Stop",
      "id": 374490040,
      "name": "BlXFzFNoulbBCmldUAIgNMsAg"
    },
    {
      "type": "Route",
      "id": 1505688358,
      "to": "Kiyevskoye sh 70",
      "from": "Leningradskiy pr 40"
    },
    {
      "type": "Stop",
      "id": 1913619708,
      "name": "Komsomolskiy pr 60"
    },
    {
      "type": "Route",
      "id": 1596197531,
      "to": "Komsomolskiy pr 60",
      "from": "Tverskaya ul 90"
    },
    {
      "type": "Stop",
      "id": 978815997,
      "name": "Mokhovaya ul 30"
    },
    {
      "type": "Route",
      "id": 536464925,
      "to": "Tverskaya ul 40",
      "from": "Komsomolskiy pr 80"
    },
    {
      "type": "Route",
      "id": 1615990436,
      "to": "Komsomolskiy pr 80",
      "from": "Kiyevskoye sh 80"
    },
    {
      "type": "Route",
      "id": 1036264216,
      "to": "Vnukovo",
      "from": "Mezhdunarodnoye sh 20"
    },
    {
      "type": "Stop",
      "id": 1603176856,
      "name": "Leningradskoye sh 80"
    },
    {
      "type": "Bus",
      "id": 2102433046,
      "name": "21k"
    },
    {
      "type": "Bus",
      "id": 1024709661,
      "name": "53k"
    },
    {
      "type": "Stop",
      "id": 1985217913,
      "name": "Pr Vernadskogo 70"
    },
    {
      "type": "Bus",
      "id": 1858267958,
      "name": "48k"
    },
    {
      "type": "Route",
      "id": 1068630960,
      "to": "Vnukovo",
      "from": "Mezhdunarodnoye sh 10"
    },
    {
      "type": "Route",
      "id": 765985802,
      "to": "Mezhdunarodnoye sh 40",
      "from": "Mezhdunarodnoye sh 70"
    },
    {
      "type": "Bus",
      "id": 1952512570,
      "name": "62"
    },
    {
      "type": "Stop",
      "id": 1071630589,
      "name": "Ostozhenka 50"
    },
    {
      "type": "Bus",
      "id": 1695308373,
      "name": "15"
    },
    {
      "type": "Stop",
      "id": 1640544668,
      "name": "Leningradskoye sh 70"
    },
    {
      "type": "Route",
      "id": 677718707,
      "to": "Troparyovo",
      "from": "Tverskaya ul 40"
    },
    {
      "type": "Route",
      "id": 1549930220,
      "to": "Kiyevskoye sh 90",
      "from": "Ostozhenka 20"
    },
    {
      "type": "Stop",
      "id": 1460982888,
      "name": "Pr Vernadskogo 80"
    },
    {
      "type": "Route",
      "id": 401532389,
      "to": "Pr Vernadskogo 40",
      "from": "Ostozhenka 80"
    },
    {
      "type": "Route",
      "id": 959523353,
      "to": "Mezhdunarodnoye sh 60",
      "from": "Mokhovaya ul 70"
    },
    {
      "type": "Route",
      "id": 185906739,
      "to": "Leningradskoye sh 90",
      "from": "Leningradskoye sh 80"
    },
    {
      "type": "Route",
      "id": 19575418,
      "to": "Kiyevskoye sh 100",
      "from": "Ostozhenka 30"
    },
    {
      "type": "Stop",
      "id": 721281380,
      "name": "Leningradskiy pr 50"
    },
    {
      "type": "Stop",
      "id": 1386408138,
      "name": "Leningradskiy pr 40"
    },
    {
      "type": "Route",
      "id": 1546989781,
      "to": "Tverskaya ul 90",
      "from": "Kiyevskoye sh 30"
    },
    {
      "type": "Bus",
      "id": 2056811794,
      "name": "Nj1QcnFfLl"
    },
    {
      "type": "Bus",
      "id": 1920432944,
      "name": "MgVROod"
    },
    {
      "type": "Route",
      "id": 1154053837,
      "to": "Mezhdunarodnoye sh 30",
      "from": "Leningradskoye sh 20"
    },
    {
      "type": "Bus",
      "id": 2085998310,
      "name": "29k"
    },
    {
      "type": "Route",
      "id": 1891242171,
      "to": "Leningradskoye sh 100",
      "from": "Komsomolskiy pr 90"
    },
    {
      "type": "Stop",
      "id": 1091618186,
      "name": "Ostozhenka 20"
    },
    {
      "type": "Route",
      "id": 214670950,
      "to": "Leningradskoye sh 80",
      "from": "Tverskaya ul 70"
    },
    {
      "type": "Route",
      "id": 2006157435,
      "to": "Leningradskoye sh 50",
      "from": "Ostozhenka 70"
    },
    {
      "type": "Route",
      "id": 329144463,
      "to": "Pr Vernadskogo 70",
      "from": "Pr Vernadskogo 50"
    },
    {
      "type": "Route",
      "id": 697570158,
      "to": "Belorusskiy vokzal",
      "from": "Leningradskiy pr 30"
    },
    {
      "type": "Route",
      "id": 1275957776,
      "to": "Leningradskiy pr 30",
      "from": "Tverskaya ul 60"
    },
    {
      "type": "Route",
      "id": 1903588840,
      "to": "Mokhovaya ul 100",
      "from": "Pr Vernadskogo 20"
    },
    {
      "type": "Route",
      "id": 1609631442,
      "to": "Komsomolskiy pr 40",
      "from": "Leningradskoye sh 80"
    },
    {
      "type": "Route",
      "id": 2028534256,
      "to": "Pr Vernadskogo 70",
      "from": "Kiyevskoye sh 100"
    },
    {
      "type": "Stop",
      "id": 1524276591,
      "name": "Kiyevskoye sh 70"
    },
    {
      "type": "Route",
      "id": 2136596711,
      "to": "Kremlin",
      "from": "Manezh"
    },
    {
      "type": "Bus",
      "id": 2112186050,
      "name": "AVwF"
    },
    {
      "type": "Bus",
      "id": 95205841,
      "name": "58"
    },
    {
      "type": "Stop",
      "id": 833334897,
      "name": "Mezhdunarodnoye sh 90"
    },
    {
      "type": "Route",
      "id": 2110380750,
      "to": "Pr Vernadskogo 70",
      "from": "Pr Vernadskogo 70"
    },
    {
      "type": "Route",
      "id": 198052749,
      "to": "Belorusskiy vokzal",
      "from": "Belorusskiy vokzal"
    },
    {
      "type": "Route",
      "id": 159900013,
      "to": "Leningradskoye sh 100",
      "from": "Leningradskoye sh 80"
    },
    {
      "type": "Stop",
      "id": 1640544668,
      "name": "Leningradskoye sh 70"
    },
    {
      "type": "Route",
      "id": 1464748113,
      "to": "Kiyevskoye sh 10",
      "from": "Tverskaya ul 50"
    },
    {
      "type": "Route",
      "id": 448077581,
      "to": "Ostozhenka 50",
      "from": "Komsomolskiy pr 60"
    },
    {
      "type": "Bus",
      "id": 1671271267,
      "name": "59"
    },
    {
      "type": "Stop",
      "id": 524832460,
      "name": "Leningradskiy pr 30"
    },
    {
      "type": "Route",
      "id": 1683319824,
      "to": "Komsomolskiy pr 90",
      "from": "Mezhdunarodnoye sh 30"
    },
    {
      "type": "Route",
      "id": 1609133114,
      "to": "Mezhdunarodnoye sh 60",
      "from": "Komsomolskiy pr 70"
    },
    {
      "type": "Route",
      "id": 1206160194,
      "to": "Kiyevskoye sh 90",
      "from": "Troparyovo"
    },
    {
      "type": "Route",
      "id": 1836589183,
      "to": "Tverskaya ul 30",
      "from": "Kiyevskoye sh 50"
    },
    {
      "type": "Route",
      "id": 734228946,
      "to": "Manezh",
      "from": "Leningradskiy pr 40"
    },
    {
      "type": "Bus",
      "id": 98583880,
      "name": "88k"
    },
    {
      "type": "Route",
      "id": 304754112,
      "to": "Tverskaya ul 20",
      "from": "Mezhdunarodnoye sh 40"
    },
    {
      "type": "Stop",
      "id": 132416404,
      "name": "Mokhovaya ul 10"
    },
    {
      "type": "Route",
      "id": 8353812,
      "to": "Manezh",
      "from": "Leningradskoye sh 10"
    },
    {
      "type": "Stop",
      "id": 759524304,
      "name": "Mezhdunarodnoye sh 60"
    },
    {
      "type": "Route",
      "id": 685761175,
      "to": "Mokhovaya ul 90",
      "from": "Sheremetyevo"
    },
    {
      "type": "Stop",
      "id": 1603176856,
      "name": "Leningradskoye sh 80"
    },
    {
      "type": "Bus",
      "id": 625048715,
      "name": "85k"
    },
    {
      "type": "Route",
      "id": 1725664938,
      "to": "Pr Vernadskogo 70",
      "from": "Belorusskiy vokzal"
    },
    {
      "type": "Route",
      "id": 1246435950,
      "to": "Mokhovaya ul 100",
      "from": "Leningradskoye sh 70"
    },
    {
      "type": "Route",
      "id": 1843188048,
      "to": "Komsomolskiy pr 70",
      "from": "Komsomolskiy pr 90"
    },
    {
      "type": "Route",
      "id": 1209128541,
      "to": "Pr Vernadskogo 70",
      "from": "Mezhdunarodnoye sh 10"
    },
    {
      "type": "Route",
      "id": 1561663282,
      "to": "Leningradskoye sh 100",
      "from": "Mezhdunarodnoye sh 20"
    },
    {
      "type": "Route",
      "id": 331024831,
      "to": "Tverskaya ul 30",
      "from": "Ostozhenka 60"
    },
    {
      "type": "Route",
      "id": 101445924,
      "to": "Komsomolskiy pr 20",
      "from": "Pr Vernadskogo 10"
    },
    {
      "type": "Route",
      "id": 1271623983,
      "to": "Mokhovaya ul 40",
      "from": "Ostozhenka 10"
    },
    {
      "type": "Route",
      "id": 638110323,
      "to": "Pr Vernadskogo 60",
      "from": "Kiyevskoye sh 50"
    },
    {
      "type": "Stop",
      "id": 1791067641,
      "name": "Mokhovaya ul 40"
    },
    {
      "type": "Route",
      "id": 572257336,
      "to": "Mezhdunarodnoye sh 20",
      "from": "Tverskaya ul 80"
    },
    {
      "type": "Stop",
      "id": 1290648739,
      "name": "gYsHTtiCF7rFjB"
    },
    {
      "type": "Route",
      "id": 1759125977,
      "to": "Leningradskiy pr 40",
      "from": "Komsomolskiy pr 40"
    },
    {
      "type": "Route",
      "id": 1538865978,
      "to": "Mokhovaya ul 60",
      "from": "Mokhovaya ul 100"
    },
    {
      "type": "Route",
      "id": 1779134062,
      "to": "Komsomolskiy pr 10",
      "from": "Mezhdunarodnoye sh 60"
    },
    {
      "type": "Bus",
      "id": 1159872807,
      "name": "f2gHvBfZmbnjpk4sXx"
    },
    {
      "type": "Route",
      "id": 649982669,
      "to": "Mokhovaya ul 30",
      "from": "Mezhdunarodnoye sh 100"
    },
    {
      "type": "Route",
      "id": 653436307,
      "to": "Komsomolskiy pr 40",
      "from": "Kiyevskoye sh 30"
    },
    {
      "type": "Route",
      "id": 1646628562,
      "to": "Sokol",
      "from": "Pr Vernadskogo 80"
    },
    {
      "type": "Route",
      "id": 1952432204,
      "to": "Leningradskiy pr 30",
      "from": "Komsomolskiy pr 90"
    },
    {
      "type": "Route",
      "id": 621590429,
      "to": "Mokhovaya ul 100",
      "from": "Tverskaya ul 10"
    },
    {
      "type": "Stop",
      "id": 1358692640,
      "name": "O9g2i 1XZINQy7DbaFh"
    },
    {
      "type": "Stop",
      "id": 1787498175,
      "name": "Leningradskiy pr 80"
    },
    {
      "type": "Route",
      "id": 1632456021,
      "to": "Leningradskiy pr 20",
      "from": "Leningradskoye sh 60"
    },
    {
      "type": "Route",
      "id": 1016006806,
      "to": "Komsomolskiy pr 30",
      "from": "Pr Vernadskogo 40"
    },
    {
      "type": "Route",
      "id": 2018507081,
      "to": "Komsomolskiy pr 10",
      "from": "Mezhdunarodnoye sh 30"
    },
    {
      "type": "Route",
      "id": 543114077,
      "to": "Pr Vernadskogo 50",
      "from": "Troparyovo"
    },
    {
      "type": "Bus",
      "id": 1260274581,
      "name": "40k"
    },
    {
      "type": "Route",
      "id": 1792696637,
      "to": "Ostozhenka 80",
      "from": "Leningradskiy pr 70"
    },
    {
      "type": "Route",
      "id": 1450046751,
      "to": "Tverskaya ul 60",
      "from": "Mezhdunarodnoye sh 60"
    },
    {
      "type": "Route",
      "id": 1129541697,
      "to": "Ostozhenka 10",
      "from": "Pr Vernadskogo 90"
    },
    {
      "type": "Route",
      "id": 1189604761,
      "to": "Leningradskiy pr 90",
      "from": "Kiyevskoye sh 80"
    },
    {
      "type": "Bus",
      "id": 725590944,
      "name": "1"
    },
    {
      "type": "Stop",
      "id": 324331084,
      "name": "CO"
    },
    {
      "type": "Route",
      "id": 2072857578,
      "to": "Pr Vernadskogo 60",
      "from": "Mezhdunarodnoye sh 60"
    },
    {
      "type": "Route",
      "id": 1674297862,
      "to": "Ostozhenka 10",
      "from": "Komsomolskiy pr 20"
    },
    {
      "type": "Route",
      "id": 1356257988,
      "to": "Mezhdunarodnoye sh 60",
      "from": "Mokhovaya ul 60"
    },
    {
      "type": "Route",
      "id": 910011172,
      "to": "Luzhniki",
      "from": "Komsomolskiy pr 20"
    },
    {
      "type": "Bus",
      "id": 1865859901,
      "name": "96"
    },
    {
      "type": "Route",
      "id": 623052192,
      "to": "Leningradskoye sh 60",
      "from": "Komsomolskiy pr 90"
    },
    {
      "type": "Route",
      "id": 2143157425,
      "to": "Ostozhenka 100",
      "from": "Mezhdunarodnoye sh 10"
    },
    {
      "type": "Stop",
      "id": 1735256877,
      "name": "Komsomolskiy pr 40"
    },
    {
      "type": "Stop",
      "id": 1321456988,
      "name": "Mezhdunarodnoye sh 100"
    },
    {
      "type": "Bus",
      "id": 1197887645,
      "name": "18"
    },
    {
      "type": "Route",
      "id": 1081664315,
      "to": "Pr Vernadskogo 40",
      "from": "Tverskaya ul 20"
    },
    {
      "type": "Route",
      "id": 1830226301,
      "to": "Kiyevskoye sh 70",
      "from": "Ostozhenka 20"
    },
    {
      "type": "Route",
      "id": 1871404677,
      "to": "Mezhdunarodnoye sh 70",
      "from": "Ostozhenka 30"
    },
    {
      "type": "Route",
      "id": 1409555494,
      "to": "Kiyevskoye sh 70",
      "from": "Troparyovo"
    },
    {
      "type": "Route",
      "id": 1329068344,
      "to": "Kiyevskoye sh 50",
      "from": "Komsomolskiy pr 20"
    },
    {
      "type": "Stop",
      "id": 2059621403,
      "name": "Kiyevskoye sh 30"
    },
    {
      "type": "Route",
      "id": 1497256683,
      "to": "Leningradskiy pr 70",
      "from": "Vnukovo"
    },
    {
      "type": "Route",
      "id": 1088056980,
      "to": "Pr Vernadskogo 100",
      "from": "Leningradskoye sh 100"
    },
    {
      "type": "Route",
      "id": 1350134801,
      "to": "Tverskaya ul 20",
      "from": "Mokhovaya ul 40"
    },
    {
      "type": "Route",
      "id": 1336817485,
      "to": "Kiyevskoye sh 50",
      "from": "Mokhovaya ul 50"
    },
    {
      "type": "Stop",
      "id": 2077587727,
      "name": "Pr Vernadskogo 40"
    },
    {
      "type": "Route",
      "id": 266447777,
      "to": "Leningradskiy pr 60",
      "from": "Leningradskoye sh 20"
    },
    {
      "type": "Route",
      "id": 452517815,
      "to": "Leningradskiy pr 60",
      "from": "Tverskaya ul 60"
    },
    {
      "type": "Stop",
      "id": 359984358,
      "name": "Ostozhenka 80"
    },
    {
      "type": "Stop",
      "id": 331121298,
      "name": "Mokhovaya ul 20"
    },
    {
      "type": "Stop",
      "id": 920147004,
      "name": "bNKvXSTjt"
    },
    {
      "type": "Route",
      "id": 895034397,
      "to": "Leningradskiy pr 10",
      "from": "Kiyevskoye sh 80"
    },
    {
      "type": "Bus",
      "id": 98583880,
      "name": "88k"
    },
    {
      "type": "Stop",
      "id": 1091618186,
      "name": "Ostozhenka 20"
    },
    {
      "type": "Route",
      "id": 1881574219,
      "to": "Troparyovo",
      "from": "Leningradskoye sh 10"
    },
    {
      "type": "Route",
      "id": 890475223,
      "to": "Ostozhenka 50",
      "from": "Kiyevskoye sh 80"
    },
    {
      "type": "Route",
      "id": 781903582,
      "to": "Leningradskoye sh 100",
      "from": "Ostozhenka 60"
    },
    {
      "type": "Stop",
      "id": 1942017243,
      "name": "Tverskaya ul 80"
    },
    {
      "type": "Route",
      "id": 1219682547,
      "to": "Mokhovaya ul 90",
      "from": "Kiyevskoye sh 90"
    },
    {
      "type": "Stop",
      "id": 1524276591,
      "name": "Kiyevskoye sh 70"
    },
    {
      "type": "Route",
      "id": 1635496951,
      "to": "Kiyevskoye sh 50",
      "from": "Leningradskiy pr 100"
    },
    {
      "type": "Route",
      "id": 1594144781,
      "to": "Leningradskiy pr 30",
      "from": "Luzhniki"
    },
    {
      "type": "Route",
      "id": 907595959,
      "to": "Mokhovaya ul 50",
      "from": "Kremlin"
    },
    {
      "type": "Route",
      "id": 43597450,
      "to": "Ostozhenka 10",
      "from": "Leningradskoye sh 100"
    },
    {
      "type": "Route",
      "id": 1971304120,
      "to": "Kiyevskoye sh 10",
      "from": "Pr Vernadskogo 60"
    },
    {
      "type": "Stop",
      "id": 341956560,
      "name": "Leningradskoye sh 50"
    },
    {
      "type": "Bus",
      "id": 1865859901,
      "name": "96"
    },
    {
      "type": "Bus",
      "id": 1674389347,
      "name": "34"
    },
    {
      "type": "Route",
      "id": 542511541,
      "to": "Leningradskoye sh 30",
      "from": "Leningradskoye sh 40"
    },
    {
      "type": "Bus",
      "id": 1459482213,
      "name": "42"
    },
    {
      "type": "Route",
      "id": 1017964375,
      "to": "Mezhdunarodnoye sh 70",
      "from": "Belorusskiy vokzal"
    },
    {
      "type": "Stop",
      "id": 1122794222,
      "name": "Leningradskiy pr 60"
    },
    {
      "type": "Route",
      "id": 889305679,
      "to": "Leningradskoye sh 90",
      "from": "Mezhdunarodnoye sh 90"
    },
    {
      "type": "Route",
      "id": 107991584,
      "to": "Tverskaya ul 10",
      "from": "Kiyevskoye sh 70"
    },
    {
      "type": "Stop",
      "id": 1390717557,
      "name": "Leningradskoye sh 30"
    },
    {
      "type": "Stop",
      "id": 564351016,
      "name": "Kiyevskoye sh 80"
    },
    {
      "type": "Stop",
      "id": 1692384196,
      "name": "Mezhdunarodnoye sh 80"
    },
    {
      "type": "Route",
      "id": 1122137607,
      "to": "Luzhniki",
      "from": "Leningradskiy pr 50"
    },
    {
      "type": "Route",
      "id": 537821418,
      "to": "Mezhdunarodnoye sh 70",
      "from": "Troparyovo"
    },
    {
      "type": "Stop",
      "id": 1660040291,
      "name": "Komsomolskiy pr 20"
    },
    {
      "type": "Route",
      "id": 927662762,
      "to": "Pr Vernadskogo 60",
      "from": "Leningradskoye sh 20"
    },
    {
      "type": "Bus",
      "id": 1862530785,
      "name": "12"
    },
    {
      "type": "Route",
      "id": 58787258,
      "to": "Mezhdunarodnoye sh 40",
      "from": "Kremlin"
    },
    {
      "type": "Route",
      "id": 1661752801,
      "to": "Pr Vernadskogo 80",
      "from": "Kiyevskoye sh 60"
    },
    {
      "type": "Route",
      "id": 2128947949,
      "to": "Komsomolskiy pr 50",
      "from": "Ostozhenka 20"
    },
    {
      "type": "Stop",
      "id": 580409940,
      "name": "gIf4z"
    },
    {
      "type": "Stop",
      "id": 1091618186,
      "name": "Ostozhenka 20"
    },
    {
      "type": "Route",
      "id": 169761597,
      "to": "Ostozhenka 40",
      "from": "Yandex"
    },
    {
      "type": "Route",
      "id": 1293344924,
      "to": "Sokol",
      "from": "Ostozhenka 30"
    },
    {
      "type": "Bus",
      "id": 1184095094,
      "name": "64k"
    },
    {
      "type": "Route",
      "id": 1321562796,
      "to": "Leningradskoye sh 80",
      "from": "Mokhovaya ul 100"
    },
    {
      "type": "Bus",
      "id": 1935737650,
      "name": "xrTrtmWIdYgGNtroQ6ltc"
    },
    {
      "type": "Route",
      "id": 1322594298,
      "to": "Sokol",
      "from": "Ostozhenka 100"
    },
    {
      "type": "Bus",
      "id": 106969856,
      "name": "41"
    },
    {
      "type": "Route",
      "id": 1368473599,
      "to": "Khimki",
      "from": "Vnukovo"
    },
    {
      "type": "Stop",
      "id": 504117283,
      "name": "O0NOozt7BKCFVZ1vXHkpR"
    },
    {
      "type": "Route",
      "id": 796760446,
      "to": "Komsomolskiy pr 10",
      "from": "Mezhdunarodnoye sh 80"
    },
    {
      "type": "Route",
      "id": 655316634,
      "to": "Komsomolskiy pr 60",
      "from": "Kremlin"
    },
    {
      "type": "Stop",
      "id": 1215181049,
      "name": "Tverskaya ul 40"
    },
    {
      "type": "Route",
      "id": 887544822,
      "to": "Mezhdunarodnoye sh 50",
      "from": "Tverskaya ul 30"
    },
    {
      "type": "Route",
      "id": 2071661666,
      "to": "Ostozhenka 60",
      "from": "Tverskaya ul 70"
    },
    {
      "type": "Route",
      "id": 1115987963,
      "to": "Leningradskoye sh 40",
      "from": "Leningradskiy pr 80"
    },
    {
      "type": "Route",
      "id": 1792931277,
      "to": "Mezhdunarodnoye sh 30",
      "from": "Tverskaya ul 100"
    },
    {
      "type": "Stop",
      "id": 1882423715,
      "name": "Kremlin"
    },
    {
      "type": "Stop",
      "id": 1034325494,
      "name": "Mezhdunarodnoye sh 10"
    },
    {
      "type": "Route",
      "id": 460372014,
      "to": "Mokhovaya ul 20",
      "from": "Leningradskiy pr 50"
    },
    {
      "type": "Route",
      "id": 1208396912,
      "to": "Mezhdunarodnoye sh 80",
      "from": "Leningradskiy pr 60"
    },
    {
      "type": "Bus",
      "id": 1994418887,
      "name": "49"
    },
    {
      "type": "Route",
      "id": 1874395400,
      "to": "Komsomolskiy pr 30",
      "from": "Ostozhenka 80"
    },
    {
      "type": "Bus",
      "id": 870750768,
      "name": "94k"
    },
    {
      "type": "Stop",
      "id": 1460982888,
      "name": "Pr Vernadskogo 80"
    },
    {
      "type": "Route",
      "id": 463606046,
      "to": "Kiyevskoye sh 30",
      "from": "Sokol"
    },
    {
      "type": "Route",
      "id": 841797810,
      "to": "Kiyevskoye sh 80",
      "from": "Leningradskoye sh 10"
    },
    {
      "type": "Route",
      "id": 118181715,
      "to": "Kiyevskoye sh 80",
      "from": "Komsomolskiy pr 60"
    },
    {
      "type": "Route",
      "id": 1534743322,
      "to": "Mezhdunarodnoye sh 100",
      "from": "Pr Vernadskogo 80"
    },
    {
      "type": "Stop",
      "id": 1260382845,
      "name": "Jo6qsOifOIxq2W"
    },
    {
      "type": "Route",
      "id": 555044374,
      "to": "Mezhdunarodnoye sh 100",
      "from": "Mokhovaya ul 20"
    },
    {
      "type": "Bus",
      "id": 2064455607,
      "name": "76k"
    },
    {
      "type": "Route",
      "id": 1471154445,
      "to": "Tverskaya ul 60",
      "from": "Pr Vernadskogo 40"
    },
    {
      "type": "Route",
      "id": 1319218570,
      "to": "Komsomolskiy pr 70",
      "from": "Sheremetyevo"
    },
    {
      "type": "Bus",
      "id": 214763116,
      "name": "27k"
    },
    {
      "type": "Stop",
      "id": 522478233,
      "name": "Mokhovaya ul 70"
    },
    {
      "type": "Bus",
      "id": 2064455607,
      "name": "76k"
    },
    {
      "type": "Route",
      "id": 2078948068,
      "to": "Leningradskiy pr 70",
      "from": "Mezhdunarodnoye sh 40"
    },
    {
      "type": "Route",
      "id": 458512466,
      "to": "Ostozhenka 30",
      "from": "Mezhdunarodnoye sh 60"
    },
    {
      "type": "Bus",
      "id": 589555403,
      "name": "47k"
    },
    {
      "type": "Bus",
      "id": 1047704483,
      "name": "75k"
    },
    {
      "type": "Bus",
      "id": 2127799517,
      "name": "cSj"
    },
    {
      "type": "Route",
      "id": 255479942,
      "to": "Pr Vernadskogo 90",
      "from": "Tverskaya ul 10"
    },
    {
      "type": "Bus",
      "id": 625048715,
      "name": "85k"
    },
    {
      "type": "Stop",
      "id": 1985217913,
      "name": "Pr Vernadskogo 70"
    },
    {
      "type": "Bus",
      "id": 870750768,
      "name": "94k"
    },
    {
      "type": "Bus",
      "id": 1960392737,
      "name": "13"
    },
    {
      "type": "Route",
      "id": 1546407652,
      "to": "Mezhdunarodnoye sh 30",
      "from": "Mokhovaya ul 100"
    },
    {
      "type": "Stop",
      "id": 1215181049,
      "name": "Tverskaya ul 40"
    },
    {
      "type": "Route",
      "id": 524047981,
      "to": "Sokol",
      "from": "Mezhdunarodnoye sh 20"
    },
    {
      "type": "Bus",
      "id": 1972799637,
      "name": "77k"
    },
    {
      "type": "Stop",
      "id": 2077587727,
      "name": "Pr Vernadskogo 40"
    },
    {
      "type": "Stop",
      "id": 978815997,
      "name": "Mokhovaya ul 30"
    },
    {
      "type": "Route",
      "id": 726188767,
      "to": "Mezhdunarodnoye sh 30",
      "from": "Mokhovaya ul 80"
    },
    {
      "type": "Stop",
      "id": 1296232862,
      "name": "VCpyjfJ9J6kCpvIkWgQNmKiMI"
    },
    {
      "type": "Bus",
      "id": 76662407,
      "name": "72k"
    },
    {
      "type": "Route",
      "id": 1543433707,
      "to": "Leningradskiy pr 100",
      "from": "Pr Vernadskogo 20"
    },
    {
      "type": "Route",
      "id": 39113460,
      "to": "Leningradskiy pr 50",
      "from": "Luzhniki"
    },
    {
      "type": "Stop",
      "id": 316546093,
      "name": "Mokhovaya ul 60"
    },
    {
      "type": "Route",
      "id": 289205177,
      "to": "Leningradskoye sh 40",
      "from": "Kiyevskoye sh 60"
    },
    {
      "type": "Route",
      "id": 536040447,
      "to": "Mokhovaya ul 30",
      "from": "Leningradskoye sh 60"
    },
    {
      "type": "Stop",
      "id": 474720704,
      "name": "Pr Vernadskogo 50"
    },
    {
      "type": "Route",
      "id": 874117014,
      "to": "Leningradskoye sh 10",
      "from": "Mokhovaya ul 100"
    },
    {
      "type": "Route",
      "id": 1734266624,
      "to": "Mezhdunarodnoye sh 10",
      "from": "Yandex"
    },
    {
      "type": "Bus",
      "id": 1695308373,
      "name": "15"
    },
    {
      "type": "Route",
      "id": 1669803588,
      "to": "Pr Vernadskogo 90",
      "from": "Leningradskiy pr 90"
    },
    {
      "type": "Stop",
      "id": 1156961276,
      "name": "Mezhdunarodnoye sh 20"
    },
    {
      "type": "Route",
      "id": 1617763738,
      "to": "Mokhovaya ul 90",
      "from": "Komsomolskiy pr 30"
    },
    {
      "type": "Stop",
      "id": 254220526,
      "name": "Khimki"
    },
    {
      "type": "Bus",
      "id": 1742458421,
      "name": "44"
    },
    {
      "type": "Route",
      "id": 762512360,
      "to": "Leningradskoye sh 30",
      "from": "Ostozhenka 60"
    },
    {
      "type": "Bus",
      "id": 589555403,
      "name": "47k"
    },
    {
      "type": "Route",
      "id": 869901478,
      "to": "Mokhovaya ul 90",
      "from": "Leningradskiy pr 70"
    },
    {
      "type": "Bus",
      "id": 474099276,
      "name": "26"
    },
    {
      "type": "Stop",
      "id": 2077587727,
      "name": "Pr Vernadskogo 40"
    },
    {
      "type": "Stop",
      "id": 1660040291,
      "name": "Komsomolskiy pr 20"
    },
    {
      "type": "Route",
      "id": 526466546,
      "to": "Mezhdunarodnoye sh 90",
      "from": "Kiyevskoye sh 100"
    },
    {
      "type": "Route",
      "id": 1871864619,
      "to": "Komsomolskiy pr 10",
      "from": "Kiyevskoye sh 30"
    },
    {
      "type": "Route",
      "id": 627041900,
      "to": "Kiyevskoye sh 90",
      "from": "Mezhdunarodnoye sh 50"
    },
    {
      "type": "Route",
      "id": 1218267870,
      "to": "Mezhdunarodnoye sh 80",
      "from": "Leningradskiy pr 100"
    },
    {
      "type": "Bus",
      "id": 1858267958,
      "name": "48k"
    },
    {
      "type": "Route",
      "id": 526522666,
      "to": "Mokhovaya ul 40",
      "from": "Mokhovaya ul 30"
    },
    {
      "type": "Bus",
      "id": 1197887645,
      "name": "18"
    },
    {
      "type": "Route",
      "id": 117093208,
      "to": "Tverskaya ul 90",
      "from": "Pr Vernadskogo 80"
    },
    {
      "type": "Stop",
      "id": 1748756235,
      "name": "Komsomolskiy pr 100"
    },
    {
      "type": "Route",
      "id": 2031331475,
      "to": "Tverskaya ul 80",
      "from": "Komsomolskiy pr 70"
    },
    {
      "type": "Bus",
      "id": 1674389347,
      "name": "34"
    },
    {
      "type": "Stop",
      "id": 985348739,
      "name": "33esgL 8Oi9rCwkeD"
    },
    {
      "type": "Route",
      "id": 1575783528,
      "to": "Komsomolskiy pr 40",
      "from": "Leningradskoye sh 20"
    },
    {
      "type": "Stop",
      "id": 2059621403,
      "name": "Kiyevskoye sh 30"
    },
    {
      "type": "Route",
      "id": 1297169571,
      "to": "Kiyevskoye sh 90",
      "from": "Mezhdunarodnoye sh 100"
    },
    {
      "type": "Route",
      "id": 835906933,
      "to": "Tverskaya ul 40",
      "from": "Leningradskiy pr 50"
    },
    {
      "type": "Route",
      "id": 684144103,
      "to": "Leningradskiy pr 80",
      "from": "Mezhdunarodnoye sh 50"
    },
    {
      "type": "Route",
      "id": 1897672556,
      "to": "Ostozhenka 60",
      "from": "Kiyevskoye sh 90"
    },
    {
      "type": "Route",
      "id": 1621813696,
      "to": "Leningradskiy pr 100",
      "from": "Leningradskiy pr 30"
    },
    {
      "type": "Route",
      "id": 349878059,
      "to": "Leningradskiy pr 30",
      "from": "Mokhovaya ul 70"
    },
    {
      "type": "Route",
      "id": 1554061756,
      "to": "Mezhdunarodnoye sh 100",
      "from": "Mezhdunarodnoye sh 40"
    },
    {
      "type": "Route",
      "id": 664394049,
      "to": "Ostozhenka 30",
      "from": "Kremlin"
    },
    {
      "type": "Route",
      "id": 423290383,
      "to": "Leningradskiy pr 80",
      "from": "Mezhdunarodnoye sh 70"
    },
    {
      "type": "Route",
      "id": 290587458,
      "to": "Komsomolskiy pr 50",
      "from": "Leningradskiy pr 20"
    },
    {
      "type": "Route",
      "id": 130754089,
      "to": "Leningradskiy pr 60",
      "from": "Mezhdunarodnoye sh 30"
    },
    {
      "type": "Route",
      "id": 1644267564,
      "to": "Mezhdunarodnoye sh 60",
      "from": "Tverskaya ul 70"
    },
    {
      "type": "Route",
      "id": 1710315282,
      "to": "Leningradskoye sh 90",
      "from": "Tverskaya ul 50"
    },
    {
      "type": "Route",
      "id": 2030587926,
      "to": "Leningradskoye sh 60",
      "from": "Komsomolskiy pr 80"
    },
    {
      "type": "Stop",
      "id": 316546093,
      "name": "Mokhovaya ul 60"
    },
    {
      "type": "Route",
      "id": 1266357999,
      "to": "Mezhdunarodnoye sh 90",
      "from": "Komsomolskiy pr 90"
    },
    {
      "type": "Route",
      "id": 1709215366,
      "to": "Kiyevskoye sh 10",
      "from": "Tverskaya ul 90"
    },
    {
      "type": "Route",
      "id": 559423355,
      "to": "Komsomolskiy pr 40",
      "from": "Mokhovaya ul 20"
    },
    {
      "type": "Route",
      "id": 1880248864,
      "to": "Ostozhenka 30",
      "from": "Vnukovo"
    },
    {
      "type": "Route",
      "id": 697524554,
      "to": "Ostozhenka 60",
      "from": "Komsomolskiy pr 30"
    },
    {
      "type": "Route",
      "id": 1329068344,
      "to": "Kiyevskoye sh 50",
      "from": "Komsomolskiy pr 20"
    },
    {
      "type": "Stop",
      "id": 661678309,
      "name": "Leningradskoye sh 10"
    },
    {
      "type": "Route",
      "id": 1546989781,
      "to": "Tverskaya ul 90",
      "from": "Kiyevskoye sh 30"
    },
    {
      "type": "Route",
      "id": 1905682988,
      "to": "Kiyevskoye sh 50",
      "from": "Komsomolskiy pr 70"
    },
    {
      "type": "Route",
      "id": 1241833005,
      "to": "Mezhdunarodnoye sh 90",
      "from": "Mezhdunarodnoye sh 30"
    },
    {
      "type": "Route",
      "id": 839460203,
      "to": "Leningradskoye sh 10",
      "from": "Ostozhenka 100"
    },
    {
      "type": "Route",
      "id": 1818699016,
      "to": "Pr Vernadskogo 100",
      "from": "Ostozhenka 80"
    },
    {
      "type": "Route",
      "id": 17605916,
      "to": "Kiyevskoye sh 70",
      "from": "Mezhdunarodnoye sh 40"
    },
    {
      "type": "Stop",
      "id": 834618510,
      "name": "0K"
    },
    {
      "type": "Stop",
      "id": 1091618186,
      "name": "Ostozhenka 20"
    },
    {
      "type": "Route",
      "id": 817889388,
      "to": "Mokhovaya ul 40",
      "from": "Kiyevskoye sh 80"
    },
    {
      "type": "Route",
      "id": 842499802,
      "to": "Leningradskoye sh 80",
      "from": "Ostozhenka 60"
    },
    {
      "type": "Route",
      "id": 1922674238,
      "to": "Luzhniki",
      "from": "Komsomolskiy pr 60"
    },
    {
      "type": "Stop",
      "id": 1390717557,
      "name": "Leningradskoye sh 30"
    },
    {
      "type": "Route",
      "id": 545559638,
      "to": "Mezhdunarodnoye sh 10",
      "from": "Mezhdunarodnoye sh 20"
    },
    {
      "type": "Route",
      "id": 1808017085,
      "to": "Tverskaya ul 30",
      "from": "Komsomolskiy pr 100"
    },
    {
      "type": "Route",
      "id": 1209721409,
      "to": "Mokhovaya ul 100",
      "from": "Leningradskoye sh 40"
    },
    {
      "type": "Route",
      "id": 2050125980,
      "to": "Tverskaya ul 10",
      "from": "Tverskaya ul 100"
    },
    {
      "type": "Stop",
      "id": 832675135,
      "name": "Mokhovaya ul 50"
    },
    {
      "type": "Stop",
      "id": 570491200,
      "name": "Leningradskoye sh 90"
    },
    {
      "type": "Stop",
      "id": 1054827034,
      "name": "Tverskaya ul 10"
    },
    {
      "type": "Stop",
      "id": 751222496,
      "name": "Mokhovaya ul 100"
    },
    {
      "type": "Route",
      "id": 1581374475,
      "to": "Mokhovaya ul 80",
      "from": "Tverskaya ul 100"
    },
    {
      "type": "Route",
      "id": 175860177,
      "to": "Mezhdunarodnoye sh 50",
      "from": "Komsomolskiy pr 50"
    },
    {
      "type": "Route",
      "id": 1354702004,
      "to": "Sokol",
      "from": "Mokhovaya ul 40"
    },
    {
      "type": "Stop",
      "id": 1964458092,
      "name": "Troparyovo"
    },
    {
      "type": "Route",
      "id": 1433436922,
      "to": "Sokol",
      "from": "Leningradskoye sh 40"
    },
    {
      "type": "Route",
      "id": 1936688346,
      "to": "Leningradskoye sh 70",
      "from": "Pr Vernadskogo 10"
    },
    {
      "type": "Stop",
      "id": 164954391,
      "name": "Tverskaya ul 60"
    },
    {
      "type": "Route",
      "id": 1478459814,
      "to": "Leningradskiy pr 60",
      "from": "Tverskaya ul 40"
    },
    {
      "type": "Stop",
      "id": 2077587727,
      "name": "Pr Vernadskogo 40"
    },
    {
      "type": "Route",
      "id": 215714208,
      "to": "Leningradskiy pr 60",
      "from": "Ostozhenka 30"
    },
    {
      "type": "Bus",
      "id": 1794383709,
      "name": "22k"
    },
    {
      "type": "Route",
      "id": 1957442596,
      "to": "Pr Vernadskogo 30",
      "from": "Leningradskoye sh 90"
    },
    {
      "type": "Route",
      "id": 1173734170,
      "to": "Mokhovaya ul 60",
      "from": "Kiyevskoye sh 100"
    },
    {
      "type": "Route",
      "id": 1655893940,
      "to": "Leningradskiy pr 70",
      "from": "Leningradskoye sh 10"
    },
    {
      "type": "Route",
      "id": 818539230,
      "to": "Kiyevskoye sh 50",
      "from": "Vnukovo"
    },
    {
      "type": "Stop",
      "id": 1721921626,
      "name": "g"
    },
    {
      "type": "Route",
      "id": 236682111,
      "to": "Pr Vernadskogo 50",
      "from": "Ostozhenka 10"
    },
    {
      "type": "Route",
      "id": 2045161687,
      "to": "Komsomolskiy pr 90",
      "from": "Leningradskiy pr 90"
    },
    {
      "type": "Route",
      "id": 758713389,
      "to": "Leningradskoye sh 50",
      "from": "Mezhdunarodnoye sh 20"
    },
    {
      "type": "Route",
      "id": 569136162,
      "to": "Kiyevskoye sh 70",
      "from": "Mokhovaya ul 50"
    },
    {
      "type": "Stop",
      "id": 1680438344,
      "name": "Leningradskiy pr 90"
    },
    {
      "type": "Bus",
      "id": 1865859901,
      "name": "96"
    },
    {
      "type": "Route",
      "id": 1018655959,
      "to": "Kiyevskoye sh 50",
      "from": "Pr Vernadskogo 100"
    },
    {
      "type": "Route",
      "id": 276187499,
      "to": "Tverskaya ul 60",
      "from": "Leningradskiy pr 80"
    },
    {
      "type": "Route",
      "id": 1734327526,
      "to": "Komsomolskiy pr 20",
      "from": "Tverskaya ul 10"
    },
    {
      "type": "Route",
      "id": 21832871,
      "to": "Tverskaya ul 100",
      "from": "Tverskaya ul 10"
    },
    {
      "type": "Route",
      "id": 1838606609,
      "to": "Leningradskoye sh 90",
      "from": "Belorusskiy vokzal"
    },
    {
      "type": "Route",
      "id": 306698184,
      "to": "Pr Vernadskogo 80",
      "from": "Ostozhenka 20"
    },
    {
      "type": "Stop",
      "id": 1603176856,
      "name": "Leningradskoye sh 80"
    },
    {
      "type": "Stop",
      "id": 2067887205,
      "name": "Mokhovaya ul 80"
    },
    {
      "type": "Route",
      "id": 1155138745,
      "to": "Ostozhenka 70",
      "from": "Tverskaya ul 40"
    },
    {
      "type": "Route",
      "id": 1434910278,
      "to": "Leningradskiy pr 100",
      "from": "Mokhovaya ul 50"
    },
    {
      "type": "Stop",
      "id": 697100878,
      "name": "atYRG4"
    },
    {
      "type": "Route",
      "id": 39563224,
      "to": "Tverskaya ul 100",
      "from": "Tverskaya ul 70"
    },
    {
      "type": "Route",
      "id": 1683996484,
      "to": "Pr Vernadskogo 80",
      "from": "Komsomolskiy pr 100"
    },
    {
      "type": "Route",
      "id": 548686797,
      "to": "Leningradskoye sh 10",
      "from": "Kiyevskoye sh 60"
    },
    {
      "type": "Stop",
      "id": 1931871759,
      "name": "qWw4Ys"
    },
    {
      "type": "Bus",
      "id": 1183335362,
      "name": "87k"
    },
    {
      "type": "Route",
      "id": 1574238413,
      "to": "Kiyevskoye sh 50",
      "from": "Leningradskoye sh 20"
    },
    {
      "type": "Route",
      "id": 1605536425,
      "to": "Tverskaya ul 40",
      "from": "Pr Vernadskogo 10"
    },
    {
      "type": "Route",
      "id": 54379879,
      "to": "Kremlin",
      "from": "Mezhdunarodnoye sh 20"
    },
    {
      "type": "Route",
      "id": 1565292828,
      "to": "Manezh",
      "from": "Leningradskiy pr 90"
    },
    {
      "type": "Route",
      "id": 1567802766,
      "to": "Ostozhenka 60",
      "from": "Leningradskoye sh 100"
    },
    {
      "type": "Route",
      "id": 473687497,
      "to": "Ostozhenka 60",
      "from": "Yandex"
    },
    {
      "type": "Bus",
      "id": 725590944,
      "name": "1"
    },
    {
      "type": "Route",
      "id": 2140280974,
      "to": "Leningradskiy pr 20",
      "from": "Tverskaya ul 50"
    },
    {
      "type": "Route",
      "id": 222238841,
      "to": "Pr Vernadskogo 60",
      "from": "Mezhdunarodnoye sh 90"
    },
    {
      "type": "Stop",
      "id": 721281380,
      "name": "Leningradskiy pr 50"
    },
    {
      "type": "Route",
      "id": 619704314,
      "to": "Kiyevskoye sh 20",
      "from": "Komsomolskiy pr 100"
    },
    {
      "type": "Stop",
      "id": 1770268632,
      "name": "Ostozhenka 90"
    },
    {
      "type": "Stop",
      "id": 1054827034,
      "name": "Tverskaya ul 10"
    },
    {
      "type": "Stop",
      "id": 1054827034,
      "name": "Tverskaya ul 10"
    },
    {
      "type": "Route",
      "id": 1866242046,
      "to": "Troparyovo",
      "from": "Ostozhenka 50"
    },
    {
      "type": "Route",
      "id": 718079123,
      "to": "Komsomolskiy pr 50",
      "from": "Komsomolskiy pr 90"
    },
    {
      "type": "Route",
      "id": 799545437,
      "to": "Kiyevskoye sh 10",
      "from": "Ostozhenka 100"
    },
    {
      "type": "Route",
      "id": 624228549,
      "to": "Mezhdunarodnoye sh 20",
      "from": "Komsomolskiy pr 70"
    },
    {
      "type": "Stop",
      "id": 978815997,
      "name": "Mokhovaya ul 30"
    },
    {
      "type": "Stop",
      "id": 1242118002,
      "name": "Sokol"
    },
    {
      "type": "Stop",
      "id": 1071355180,
      "name": "Pr Vernadskogo 20"
    },
    {
      "type": "Route",
      "id": 1105241647,
      "to": "Mokhovaya ul 70",
      "from": "Tverskaya ul 90"
    },
    {
      "type": "Route",
      "id": 896151587,
      "to": "Ostozhenka 50",
      "from": "Komsomolskiy pr 80"
    },
    {
      "type": "Route",
      "id": 272917531,
      "to": "Tverskaya ul 50",
      "from": "Ostozhenka 80"
    },
    {
      "type": "Bus",
      "id": 557108693,
      "name": "61k"
    },
    {
      "type": "Route",
      "id": 1230845179,
      "to": "Kiyevskoye sh 100",
      "from": "Ostozhenka 60"
    },
    {
      "type": "Route",
      "id": 1150171031,
      "to": "Komsomolskiy pr 70",
      "from": "Leningradskoye sh 100"
    },
    {
      "type": "Route",
      "id": 799155238,
      "to": "Pr Vernadskogo 70",
      "from": "Leningradskoye sh 50"
    },
    {
      "type": "Bus",
      "id": 1081123661,
      "name": "65k"
    },
    {
      "type": "Route",
      "id": 1333082517,
      "to": "Tverskaya ul 30",
      "from": "Kiyevskoye sh 70"
    },
    {
      "type": "Route",
      "id": 696092156,
      "to": "Komsomolskiy pr 20",
      "from": "Ostozhenka 50"
    },
    {
      "type": "Stop",
      "id": 1695724973,
      "name": "Pr Vernadskogo 90"
    },
    {
      "type": "Stop",
      "id": 612611093,
      "name": "GSfYVv"
    },
    {
      "type": "Stop",
      "id": 1156961276,
      "name": "Mezhdunarodnoye sh 20"
    },
    {
      "type": "Route",
      "id": 429499919,
      "to": "Komsomolskiy pr 20",
      "from": "Komsomolskiy pr 20"
    },
    {
      "type": "Route",
      "id": 41862999,
      "to": "Mezhdunarodnoye sh 90",
      "from": "Kiyevskoye sh 50"
    },
    {
      "type": "Stop",
      "id": 1071630589,
      "name": "Ostozhenka 50"
    },
    {
      "type": "Stop",
      "id": 1605701212,
      "name": "Komsomolskiy pr 10"
    },
    {
      "type": "Bus",
      "id": 1124213689,
      "name": "25k"
    },
    {
      "type": "Route",
      "id": 280112568,
      "to": "Mokhovaya ul 80",
      "from": "Mokhovaya ul 20"
    },
    {
      "type": "Bus",
      "id": 353672929,
      "name": "16k"
    },
    {
      "type": "Stop",
      "id": 1273991077,
      "name": "Kiyevskoye sh 10"
    },
    {
      "type": "Route",
      "id": 1106489204,
      "to": "Manezh",
      "from": "Luzhniki"
    },
    {
      "type": "Route",
      "id": 1109302539,
      "to": "Komsomolskiy pr 10",
      "from": "Pr Vernadskogo 80"
    },
    {
      "type": "Route",
      "id": 491220834,
      "to": "Leningradskiy pr 40",
      "from": "Pr Vernadskogo 80"
    },
    {
      "type": "Route",
      "id": 1166689630,
      "to": "Ostozhenka 80",
      "from": "Luzhniki"
    },
    {
      "type": "Route",
      "id": 120363152,
      "to": "Leningradskoye sh 60",
      "from": "Mokhovaya ul 40"
    },
    {
      "type": "Route",
      "id": 1813526320,
      "to": "Mokhovaya ul 90",
      "from": "Mokhovaya ul 80"
    },
    {
      "type": "Route",
      "id": 1133214130,
      "to": "Tverskaya ul 20",
      "from": "Leningradskoye sh 70"
    },
    {
      "type": "Route",
      "id": 1342702159,
      "to": "Pr Vernadskogo 60",
      "from": "Pr Vernadskogo 20"
    },
    {
      "type": "Bus",
      "id": 1838653027,
      "name": "8CW6eSJ4lTCroM0IVgmRx"
    },
    {
      "type": "Stop",
      "id": 132064624,
      "name": "Komsomolskiy pr 50"
    },
    {
      "type": "Route",
      "id": 2115391883,
      "to": "Kiyevskoye sh 70",
      "from": "Ostozhenka 10"
    },
    {
      "type": "Route",
      "id": 1129341813,
      "to": "Leningradskoye sh 70",
      "from": "Mokhovaya ul 10"
    },
    {
      "type": "Stop",
      "id": 1091618186,
      "name": "Ostozhenka 20"
    },
    {
      "type": "Route",
      "id": 1532517314,
      "to": "Pr Vernadskogo 30",
      "from": "Leningradskiy pr 80"
    },
    {
      "type": "Route",
      "id": 883392302,
      "to": "Kiyevskoye sh 30",
      "from": "Ostozhenka 50"
    },
    {
      "type": "Route",
      "id": 881335446,
      "to": "Leningradskoye sh 50",
      "from": "Mokhovaya ul 10"
    },
    {
      "type": "Route",
      "id": 1439957026,
      "to": "Pr Vernadskogo 20",
      "from": "Mokhovaya ul 30"
    },
    {
      "type": "Stop",
      "id": 1962613570,
      "name": "WdmGixisOoczPC4VfWJRww"
    },
    {
      "type": "Route",
      "id": 779905969,
      "to": "Leningradskiy pr 10",
      "from": "Sokol"
    },
    {
      "type": "Stop",
      "id": 1850358578,
      "name": "H3C"
    },
    {
      "type": "Stop",
      "id": 1460982888,
      "name": "Pr Vernadskogo 80"
    },
    {
      "type": "Route",
      "id": 1108215571,
      "to": "Mezhdunarodnoye sh 20",
      "from": "Kiyevskoye sh 20"
    },
    {
      "type": "Stop",
      "id": 132064624,
      "name": "Komsomolskiy pr 50"
    },
    {
      "type": "Route",
      "id": 227613338,
      "to": "Luzhniki",
      "from": "Troparyovo"
    },
    {
      "type": "Route",
      "id": 236078065,
      "to": "Leningradskoye sh 20",
      "from": "Mokhovaya ul 80"
    },
    {
      "type": "Stop",
      "id": 1985217913,
      "name": "Pr Vernadskogo 70"
    },
    {
      "type": "Route",
      "id": 449741849,
      "to": "Leningradskoye sh 30",
      "from": "Komsomolskiy pr 60"
    },
    {
      "type": "Route",
      "id": 1433313153,
      "to": "Pr Vernadskogo 30",
      "from": "Komsomolskiy pr 20"
    },
    {
      "type": "Route",
      "id": 1357064407,
      "to": "Vnukovo",
      "from": "Komsomolskiy pr 30"
    }
  ]
}
})");

        ostringstream oss;

        Parse(iss, oss);

        istringstream iss_expect(R"([
  {
    "request_id": 1,
    "stop_count": 7,
    "unique_stop_count": 4,
    "route_length": 24490,
    "curvature": 1.63414
  },
  {
    "request_id": 2,
    "total_time": 29.26,
    "items": [
        {
            "time": 2,
            "type": "Wait",
            "stop_name": "Zagorye"
        },
        {
            "span_count": 1,
            "bus": "289",
            "type": "Bus",
            "time": 0.46
        },
        {
            "time": 2,
            "type": "Wait",
            "stop_name": "Lipetskaya ulitsa 46"
        },
        {
            "span_count": 1,
            "bus": "289",
            "type": "Bus",
            "time": 24.8
        }
    ]
  },
  {
    "request_id": 3,
    "total_time": 22,
    "items": [
        {
            "time": 2,
            "type": "Wait",
            "stop_name": "Moskvorechye"
        },
        {
            "span_count": 1,
            "bus": "289",
            "type": "Bus",
            "time": 20
        }
    ]
  },
  {
    "request_id": 4,
    "total_time": 0,
    "items": [
    ]
  }
])");

        string str = oss.str();
        string str_expect = iss_expect.str();

        // ASSERT_EQUAL(str, str_expect);
    }
}

void TestParseRouteQuery()
{
    {
        StopPtr stop1 = make_shared<Stop>(Stop{"1"});
        StopPtr stop2 = make_shared<Stop>(Stop{"2"});
        StopPtr stop3 = make_shared<Stop>(Stop{"3"});
        StopPtr stop4 = make_shared<Stop>(Stop{"4"});
        StopPtr stop5 = make_shared<Stop>(Stop{"5"});
        StopPtr stop6 = make_shared<Stop>(Stop{"6"});
        StopPtr stop7 = make_shared<Stop>(Stop{"7"});
        StopPtr stop8 = make_shared<Stop>(Stop{"8"});

        BusPtr bus1 = make_shared<Bus>(Bus{"841", {stop1, stop2, stop3}});
        BusPtr bus2 = make_shared<Bus>(Bus{"842", {stop3, stop4, stop5}});
        BusPtr bus3 = make_shared<Bus>(Bus{"843", {stop3, stop6}});
        BusPtr bus4 = make_shared<Bus>(Bus{"844", {stop7, stop8}});

        DataBase db;
        db.stops = {stop1, stop2, stop3, stop4, stop5, stop6, stop7, stop8};
        db.buses = {bus1, bus2, bus3, bus4};

        db.road_route_length[stop1][stop2] = 1000;
        db.road_route_length[stop2][stop1] = 1000;
        db.road_route_length[stop2][stop3] = 500;
        db.road_route_length[stop3][stop2] = 500;
        db.road_route_length[stop3][stop4] = 800;
        db.road_route_length[stop4][stop3] = 800;
        db.road_route_length[stop4][stop5] = 300;
        db.road_route_length[stop5][stop4] = 300;
        db.road_route_length[stop3][stop6] = 200;
        db.road_route_length[stop6][stop3] = 200;
        db.road_route_length[stop7][stop8] = 100;
        db.road_route_length[stop8][stop7] = 200;

        db.CreateInfo(6/*bus_wait_time*/, 40.0/*bus_velocity km/hour*/);

        /* bus1: 1 <-> 2 <-> 3
        * bus2: 3 <-> 4 <-> 5
        * bus3: 3 <-> 6
        * bus4: 7 <-> 8
        * bus1-bus2: 3 <-> 3
        * bus1-bus3: 3 <-> 3
        * bus2-bus3: 3 <-> 3
        * 3 (transfer) <-> 3-bus1
        * 3 (transfer) <-> 3-bus2
        * 3 (transfer) <-> 3-bus3
        */
        // ASSERT_EQUAL(db.graph.GetVertexCount(), 11U);
        // ASSERT_EQUAL(db.graph.GetEdgeCount(), 24U);

        Router router{db.graph};

        {
            std::optional<RouteQueryAnswer> answer = ParseRouteQuery(stop1, stop2, db, router);
            ASSERT(answer.has_value());
            ASSERT(AssertDouble(answer->total_time, 7.5));
            const vector<Item> &items = answer->items;
            const WaitItem *wait_item = get_if<WaitItem>(&items[0]);
            ASSERT(wait_item);
            ASSERT_EQUAL(wait_item->stop, stop1);
            const BusItem *bus_item = get_if<BusItem>(&items[1]);
            ASSERT(bus_item);
            ASSERT_EQUAL(bus_item->bus, bus1);
            ASSERT_EQUAL(bus_item->span_count, 1U);
            ASSERT(AssertDouble(bus_item->time, 1.5));
        }
        {
            std::optional<RouteQueryAnswer> answer = ParseRouteQuery(stop1, stop3, db, router);
            ASSERT(answer.has_value());
            ASSERT(AssertDouble(answer->total_time, 8.25));
            const vector<Item> &items = answer->items;
            const WaitItem *wait_item = get_if<WaitItem>(&items[0]);
            ASSERT(wait_item);
            ASSERT_EQUAL(wait_item->stop, stop1);
            const BusItem *bus_item = get_if<BusItem>(&items[1]);
            ASSERT(bus_item);
            ASSERT_EQUAL(bus_item->bus, bus1);
            ASSERT_EQUAL(bus_item->span_count, 2U);
            ASSERT(AssertDouble(bus_item->time, 2.25));
        }
        {
            std::optional<RouteQueryAnswer> answer = ParseRouteQuery(stop3, stop1, db, router);
            ASSERT(answer.has_value());
            ASSERT(AssertDouble(answer->total_time, 8.25));
            const vector<Item> &items = answer->items;
            const WaitItem *wait_item = get_if<WaitItem>(&items[0]);
            ASSERT(wait_item);
            ASSERT_EQUAL(wait_item->stop, stop3);
            const BusItem *bus_item = get_if<BusItem>(&items[1]);
            ASSERT(bus_item);
            ASSERT_EQUAL(bus_item->bus, bus1);
            ASSERT_EQUAL(bus_item->span_count, 2U);
            ASSERT(AssertDouble(bus_item->time, 2.25));
        }
        {
            std::optional<RouteQueryAnswer> answer = ParseRouteQuery(stop1, stop4, db, router);
            ASSERT(answer.has_value());
            ASSERT(AssertDouble(answer->total_time, 15.45));
            const vector<Item> &items = answer->items;
            const WaitItem *wait_item = get_if<WaitItem>(&items[0]);
            ASSERT(wait_item);
            ASSERT_EQUAL(wait_item->stop, stop1);
            const BusItem *bus_item = get_if<BusItem>(&items[1]);
            ASSERT(bus_item);
            ASSERT_EQUAL(bus_item->bus, bus1);
            ASSERT_EQUAL(bus_item->span_count, 2U);
            ASSERT(AssertDouble(bus_item->time, 2.25));
            wait_item = get_if<WaitItem>(&items[2]);
            ASSERT(wait_item);
            ASSERT_EQUAL(wait_item->stop, stop3);
            bus_item = get_if<BusItem>(&items[3]);
            ASSERT(bus_item);
            ASSERT_EQUAL(bus_item->bus, bus2);
            ASSERT_EQUAL(bus_item->span_count, 1U);
            ASSERT(AssertDouble(bus_item->time, 1.2));
        }
        {
            std::optional<RouteQueryAnswer> answer = ParseRouteQuery(stop1, stop6, db, router);
            ASSERT(answer.has_value());
            ASSERT(AssertDouble(answer->total_time, 14.55));
            const vector<Item> &items = answer->items;
            const WaitItem *wait_item = get_if<WaitItem>(&items[0]);
            ASSERT(wait_item);
            ASSERT_EQUAL(wait_item->stop, stop1);
            const BusItem *bus_item = get_if<BusItem>(&items[1]);
            ASSERT(bus_item);
            ASSERT_EQUAL(bus_item->bus, bus1);
            ASSERT_EQUAL(bus_item->span_count, 2U);
            ASSERT(AssertDouble(bus_item->time, 2.25));
            wait_item = get_if<WaitItem>(&items[2]);
            ASSERT(wait_item);
            ASSERT_EQUAL(wait_item->stop, stop3);
            bus_item = get_if<BusItem>(&items[3]);
            ASSERT(bus_item);
            ASSERT_EQUAL(bus_item->bus, bus3);
            ASSERT_EQUAL(bus_item->span_count, 1U);
            ASSERT(AssertDouble(bus_item->time, 0.3));
        }
        {
            std::optional<RouteQueryAnswer> answer = ParseRouteQuery(stop6, stop1, db, router);
            ASSERT(answer.has_value());
            ASSERT(AssertDouble(answer->total_time, 14.55));
            const vector<Item> &items = answer->items;
            const WaitItem *wait_item = get_if<WaitItem>(&items[0]);
            ASSERT(wait_item);
            ASSERT_EQUAL(wait_item->stop, stop6);
            const BusItem *bus_item = get_if<BusItem>(&items[1]);
            ASSERT(bus_item);
            ASSERT_EQUAL(bus_item->bus, bus3);
            ASSERT_EQUAL(bus_item->span_count, 1U);
            ASSERT(AssertDouble(bus_item->time, 0.3));
            wait_item = get_if<WaitItem>(&items[2]);
            ASSERT(wait_item);
            ASSERT_EQUAL(wait_item->stop, stop3);
            bus_item = get_if<BusItem>(&items[3]);
            ASSERT(bus_item);
            ASSERT_EQUAL(bus_item->bus, bus1);
            ASSERT_EQUAL(bus_item->span_count, 2U);
            ASSERT(AssertDouble(bus_item->time, 2.25));
        }
        {
            std::optional<RouteQueryAnswer> answer = ParseRouteQuery(stop1, stop7, db, router);
            ASSERT(not answer.has_value());
            answer = ParseRouteQuery(stop1, stop8, db, router);
            ASSERT(not answer.has_value());
            answer = ParseRouteQuery(stop8, stop1, db, router);
            ASSERT(not answer.has_value());
            answer = ParseRouteQuery(stop8, stop2, db, router);
            ASSERT(not answer.has_value());
            answer = ParseRouteQuery(stop7, stop4, db, router);
            ASSERT(not answer.has_value());
            answer = ParseRouteQuery(stop7, stop3, db, router);
            ASSERT(not answer.has_value());
        }
    }

    {
        StopPtr stop1 = make_shared<Stop>(Stop{"1"});
        StopPtr stop2 = make_shared<Stop>(Stop{"2"});
        StopPtr stop3 = make_shared<Stop>(Stop{"3"});
        StopPtr stop4 = make_shared<Stop>(Stop{"4"});
        StopPtr stop5 = make_shared<Stop>(Stop{"5"});
        StopPtr stop6 = make_shared<Stop>(Stop{"6"});
        StopPtr stop7 = make_shared<Stop>(Stop{"7"});

        BusPtr bus1 = make_shared<Bus>(Bus{"841", {stop1, stop2, stop7}});
        BusPtr bus2 = make_shared<Bus>(Bus{"842", {stop1, stop3, stop7}});
        BusPtr bus3 = make_shared<Bus>(Bus{"843", {stop1, stop4, stop7}});
        BusPtr bus4 = make_shared<Bus>(Bus{"844", {stop1, stop5, stop7}});
        BusPtr bus5 = make_shared<Bus>(Bus{"845", {stop1, stop6, stop7}});

        DataBase db;
        db.stops = {stop1, stop2, stop3, stop4, stop5, stop6, stop7};
        db.buses = {bus1, bus2, bus3, bus4, bus5};

        db.road_route_length[stop1][stop2] = 100;
        db.road_route_length[stop2][stop1] = 100;
        db.road_route_length[stop1][stop3] = 200;
        db.road_route_length[stop3][stop1] = 200;
        db.road_route_length[stop1][stop4] = 300;
        db.road_route_length[stop4][stop1] = 300;
        db.road_route_length[stop1][stop5] = 99;
        db.road_route_length[stop5][stop1] = 99;
        db.road_route_length[stop1][stop6] = 400;
        db.road_route_length[stop6][stop1] = 400;

        db.road_route_length[stop2][stop7] = 1000;
        db.road_route_length[stop3][stop7] = 1000;
        db.road_route_length[stop4][stop7] = 1000;
        db.road_route_length[stop5][stop7] = 1000;
        db.road_route_length[stop6][stop7] = 1000;
        db.road_route_length[stop7][stop2] = 1000;
        db.road_route_length[stop7][stop3] = 1000;
        db.road_route_length[stop7][stop4] = 1000;
        db.road_route_length[stop7][stop5] = 1000;
        db.road_route_length[stop7][stop6] = 1000;

        db.CreateInfo(6/*bus_wait_time*/, 40.0/*bus_velocity km/hour*/);

        /* bus1: 1 <-> 2 <-> 7
         * bus2: 1 <-> 3 <-> 7
         * bus3: 1 <-> 4 <-> 7
         * bus4: 1 <-> 5 <-> 7
         * bus5: 1 <-> 6 <-> 7
         * bus1-bus2: 1 <-> 1
         * bus1-bus3: 1 <-> 1
         * bus1-bus4: 1 <-> 1
         * bus1-bus5: 1 <-> 1
         * bus2-bus3: 1 <-> 1
         * bus2-bus4: 1 <-> 1
         * bus2-bus5: 1 <-> 1
         * bus3-bus4: 1 <-> 1
         * bus3-bus5: 1 <-> 1
         * bus4-bus5: 1 <-> 1
         * bus1-bus2: 7 <-> 7
         * bus1-bus3: 7 <-> 7
         * bus1-bus4: 7 <-> 7
         * bus1-bus5: 7 <-> 7
         * bus2-bus3: 7 <-> 7
         * bus2-bus4: 7 <-> 7
         * bus2-bus5: 7 <-> 7
         * bus3-bus4: 7 <-> 7
         * bus3-bus5: 7 <-> 7
         * bus4-bus5: 7 <-> 7
         * 1 (transfer) <-> 1-bus1
         * 1 (transfer) <-> 1-bus2
         * 1 (transfer) <-> 1-bus3
         * 1 (transfer) <-> 1-bus4
         * 1 (transfer) <-> 1-bus5
         * 7 (transfer) <-> 7-bus1
         * 7 (transfer) <-> 7-bus2
         * 7 (transfer) <-> 7-bus3
         * 7 (transfer) <-> 7-bus4
         * 7 (transfer) <-> 7-bus5
        */
        // ASSERT_EQUAL(db.graph.GetVertexCount(), 17U);
        // ASSERT_EQUAL(db.graph.GetEdgeCount(), 80U);

        Router router{db.graph};

        {
            std::optional<RouteQueryAnswer> answer = ParseRouteQuery(stop1, stop7, db, router);
            ASSERT(answer.has_value());
            ASSERT(AssertDouble(answer->total_time, 6 + 0.148 + 1.5));
            const vector<Item> &items = answer->items;
            const WaitItem *wait_item = get_if<WaitItem>(&items[0]);
            ASSERT(wait_item);
            ASSERT_EQUAL(wait_item->stop, stop1);
            const BusItem *bus_item = get_if<BusItem>(&items[1]);
            ASSERT(bus_item);
            ASSERT_EQUAL(bus_item->bus, bus4);
            ASSERT_EQUAL(bus_item->span_count, 2U);
            ASSERT(AssertDouble(bus_item->time, 0.148 + 1.5));
        }
        {
            std::optional<RouteQueryAnswer> answer = ParseRouteQuery(stop7, stop1, db, router);
            ASSERT(answer.has_value());
            ASSERT(AssertDouble(answer->total_time, 6 + 0.148 + 1.5));
            const vector<Item> &items = answer->items;
            const WaitItem *wait_item = get_if<WaitItem>(&items[0]);
            ASSERT(wait_item);
            ASSERT_EQUAL(wait_item->stop, stop7);
            const BusItem *bus_item = get_if<BusItem>(&items[1]);
            ASSERT(bus_item);
            ASSERT_EQUAL(bus_item->bus, bus4);
            ASSERT_EQUAL(bus_item->span_count, 2U);
            ASSERT(AssertDouble(bus_item->time, 0.148 + 1.5));
        }
        {
            std::optional<RouteQueryAnswer> answer = ParseRouteQuery(stop6, stop3, db, router);
            ASSERT(answer.has_value());
            ASSERT(AssertDouble(answer->total_time, 6 + 0.6 + 6 + 0.3));
            const vector<Item> &items = answer->items;
            const WaitItem *wait_item = get_if<WaitItem>(&items[0]);
            ASSERT(wait_item);
            ASSERT_EQUAL(wait_item->stop, stop6);
            const BusItem *bus_item = get_if<BusItem>(&items[1]);
            ASSERT(bus_item);
            ASSERT_EQUAL(bus_item->bus, bus5);
            ASSERT_EQUAL(bus_item->span_count, 1U);
            ASSERT(AssertDouble(bus_item->time, 0.6));
            wait_item = get_if<WaitItem>(&items[2]);
            ASSERT(wait_item);
            ASSERT_EQUAL(wait_item->stop, stop1);
            bus_item = get_if<BusItem>(&items[3]);
            ASSERT(bus_item);
            ASSERT_EQUAL(bus_item->bus, bus2);
            ASSERT_EQUAL(bus_item->span_count, 1U);
            ASSERT(AssertDouble(bus_item->time, 0.3));
        }
    }

    {
        StopPtr stop1 = make_shared<Stop>(Stop{"1"});
        StopPtr stop2 = make_shared<Stop>(Stop{"2"});
        StopPtr stop3 = make_shared<Stop>(Stop{"3"});

        BusPtr bus1 = make_shared<Bus>(Bus{"841", {stop1, stop2}});
        BusPtr bus2 = make_shared<Bus>(Bus{"842", {stop2, stop3}});
        BusPtr bus3 = make_shared<Bus>(Bus{"843", {stop1, stop3}});

        DataBase db;
        db.stops = {stop1, stop2, stop3};
        db.buses = {bus1, bus2, bus3};

        db.road_route_length[stop1][stop2] = 100;
        db.road_route_length[stop2][stop1] = 100;

        db.road_route_length[stop2][stop3] = 200;
        db.road_route_length[stop3][stop2] = 200;

        db.road_route_length[stop1][stop3] = 4310;
        db.road_route_length[stop3][stop1] = 4310;

        db.CreateInfo(6/*bus_wait_time*/, 40.0/*bus_velocity km/hour*/);

        // ASSERT_EQUAL(db.graph.GetVertexCount(), 9U);
        // ASSERT_EQUAL(db.graph.GetEdgeCount(), 24U);

        Router router{db.graph};

        {
            std::optional<RouteQueryAnswer> answer = ParseRouteQuery(stop1, stop3, db, router);
            ASSERT(answer.has_value());
            ASSERT(AssertDouble(answer->total_time, 6 + 6.45));
            const vector<Item> &items = answer->items;
            const WaitItem *wait_item = get_if<WaitItem>(&items[0]);
            ASSERT(wait_item);
            ASSERT_EQUAL(wait_item->stop, stop1);
            const BusItem *bus_item = get_if<BusItem>(&items[1]);
            ASSERT(bus_item);
            ASSERT_EQUAL(bus_item->bus, bus1);
            ASSERT_EQUAL(bus_item->span_count, 1U);
            ASSERT(AssertDouble(bus_item->time, 0.15));
            wait_item = get_if<WaitItem>(&items[2]);
            ASSERT(wait_item);
            ASSERT_EQUAL(wait_item->stop, stop2);
            bus_item = get_if<BusItem>(&items[3]);
            ASSERT(bus_item);
            ASSERT_EQUAL(bus_item->bus, bus2);
            ASSERT_EQUAL(bus_item->span_count, 1U);
            ASSERT(AssertDouble(bus_item->time, 0.3));
        }
    }

    {
        StopPtr stop1 = make_shared<Stop>(Stop{"1"});
        StopPtr stop2 = make_shared<Stop>(Stop{"2"});
        StopPtr stop3 = make_shared<Stop>(Stop{"3"});

        BusPtr bus1 = make_shared<Bus>(Bus{"841", {stop1, stop2}});
        BusPtr bus2 = make_shared<Bus>(Bus{"842", {stop2, stop3}});
        BusPtr bus3 = make_shared<Bus>(Bus{"843", {stop1, stop3}});

        DataBase db;
        db.stops = {stop1, stop2, stop3};
        db.buses = {bus1, bus2, bus3};

        db.road_route_length[stop1][stop2] = 100;
        db.road_route_length[stop2][stop1] = 100;

        db.road_route_length[stop2][stop3] = 200;
        db.road_route_length[stop3][stop2] = 200;

        db.road_route_length[stop1][stop3] = 4295;
        db.road_route_length[stop3][stop1] = 4295;

        db.CreateInfo(6/*bus_wait_time*/, 40.0/*bus_velocity km/hour*/);

        // ASSERT_EQUAL(db.graph.GetVertexCount(), 9U);
        // ASSERT_EQUAL(db.graph.GetEdgeCount(), 24U);

        Router router{db.graph};

        {
            std::optional<RouteQueryAnswer> answer = ParseRouteQuery(stop1, stop3, db, router);
            ASSERT(answer.has_value());
            ASSERT(AssertDouble(answer->total_time, 6 + 6.44));
            const vector<Item> &items = answer->items;
            const WaitItem *wait_item = get_if<WaitItem>(&items[0]);
            ASSERT(wait_item);
            ASSERT_EQUAL(wait_item->stop, stop1);
            const BusItem *bus_item = get_if<BusItem>(&items[1]);
            ASSERT(bus_item);
            ASSERT_EQUAL(bus_item->bus, bus3);
            ASSERT_EQUAL(bus_item->span_count, 1U);
            ASSERT(AssertDouble(bus_item->time, 6.44));
        }
    }
    {
        StopPtr stop1 = make_shared<Stop>(Stop{"1"});
        StopPtr stop2 = make_shared<Stop>(Stop{"2"});
        StopPtr stop3 = make_shared<Stop>(Stop{"3"});
        StopPtr stop4 = make_shared<Stop>(Stop{"4"});

        BusPtr bus1 = make_shared<Bus>(Bus{"841", {stop1, stop2, stop3, stop4, stop1}});
        bus1->ring = true;

        DataBase db;
        db.stops = {stop1, stop2, stop3, stop4};
        db.buses = {bus1};

        db.road_route_length[stop1][stop2] = 100;
        db.road_route_length[stop2][stop3] = 200;
        db.road_route_length[stop3][stop4] = 300;
        db.road_route_length[stop4][stop1] = 400;

        db.CreateInfo(10/*bus_wait_time*/, 6.0/*bus_velocity km/hour*/);
        // 100 метров/минуту

        // ASSERT_EQUAL(db.graph.GetVertexCount(), 9U);
        // ASSERT_EQUAL(db.graph.GetEdgeCount(), 24U);

        Router router{db.graph};

        {
            std::optional<RouteQueryAnswer> answer = ParseRouteQuery(stop3, stop2, db, router);
            ASSERT(answer.has_value());
            ASSERT(AssertDouble(answer->total_time, 10 + 7 + 10 + 1));
            const vector<Item> &items = answer->items;
            const WaitItem *wait_item = get_if<WaitItem>(&items[0]);
            ASSERT(wait_item);
            ASSERT_EQUAL(wait_item->stop, stop3);
            const BusItem *bus_item = get_if<BusItem>(&items[1]);
            ASSERT(bus_item);
            ASSERT_EQUAL(bus_item->bus, bus1);
            ASSERT_EQUAL(bus_item->span_count, 2U);
            ASSERT(AssertDouble(bus_item->time, 7));
            wait_item = get_if<WaitItem>(&items[2]);
            ASSERT(wait_item);
            ASSERT_EQUAL(wait_item->stop, stop1);
            bus_item = get_if<BusItem>(&items[3]);
            ASSERT(bus_item);
            ASSERT_EQUAL(bus_item->bus, bus1);
            ASSERT_EQUAL(bus_item->span_count, 1U);
            ASSERT(AssertDouble(bus_item->time, 1));
        }
    }
}

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, TestParseJson);
    RUN_TEST(tr, TestParseAddStopQuery);
    RUN_TEST(tr, TestParseAddBusQuery);
    RUN_TEST(tr, TestCalcGeoDistance);
    RUN_TEST(tr, TestDataBaseCreateInfo);
    RUN_TEST(tr, TestDataBaseCreateGraph);
    RUN_TEST(tr, TestBuildRoute);
    RUN_TEST(tr, TestParseRouteQuery);
    // RUN_TEST(tr, TestParse);
}

void Profile()
{
}

int main()
{
    using namespace std::chrono;

    TestAll();

    Parse(cin, cout);
    return 0;
}