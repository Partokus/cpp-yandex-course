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
        return str_hasher(value->name);
    }

    hash<string> str_hasher;
};

template <typename PtrWithName>
struct NamePtrKeyEqual
{
    bool operator()(const PtrWithName &lhs, const PtrWithName &rhs) const
    {
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

    void CreateInfo(size_t bus_wait_time = 0U, double bus_velocity = 0.0)
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

            for (const StopPtr &stop : unique_stops)
            {
                stop_and_bus_to_vertex_id[stop][bus] = _vertex_id;
                vertex_id_to_stop_and_bus[_vertex_id] = pair{stop, bus};
                ++_vertex_id;
            }

            for (auto it = bus->stops.begin(); it != bus->stops.end(); ++it)
            {
                auto it_next = next(it);
                if (it_next == bus->stops.end())
                    break;

                const StopPtr &lhs = *it;
                const StopPtr &rhs = *it_next;

                info.route_length_geo += CalcGeoDistance(
                    lhs->latitude, lhs->longitude,
                    rhs->latitude, rhs->longitude
                );

                std::optional<size_t> road_distance = CalcRoadDistance(lhs, rhs);
                if (road_distance)
                    info.route_length_road += *road_distance;
                if (not bus->ring)
                {
                    road_distance = CalcRoadDistance(rhs, lhs);
                    if (road_distance)
                        info.route_length_road += *road_distance;
                }
            }

            if (not bus->ring)
                info.route_length_geo *= 2.0;
        }

        CreateGraph();
    }

    Map<StopPtr, Map<BusPtr, Graph::VertexId>> stop_and_bus_to_vertex_id;
    unordered_map<Graph::VertexId, pair<StopPtr, BusPtr>> vertex_id_to_stop_and_bus;

    Map<StopPtr, Graph::VertexId> begin_end_transfer; // Для хранения вершин, которые необходимы для
                                                      // выбора нужного автобуса в начале и в конце пути.
                                                      // Без этого непонятно будет какой автобус в начале выбрать,
                                                      // а это в свою очередь приводит к неправильному выбору оптимального пути
                                                      // за счёт того, что переход между остановками тоже занимает время

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

    void CreateGraph()
    {
        size_t edgs_count = 0U;
        size_t shadow_edges = 0U;
        size_t trans_edges = 0U;
        // формируем исскуственные вершины для пересадок в начале и в конце пути
        for (const auto &[stop, bus_to_vertex_id] : stop_and_bus_to_vertex_id)
        {
            // Если у остановки больше, чем 1 автобус, то создаём искусственную вершину для
            // перехода между остановками в начале и в конце пути
            if (bus_to_vertex_id.size() > 1U)
                begin_end_transfer[stop] = _vertex_id++;
        }

        graph = DirectedWeightedGraph{ _vertex_id };

        for (const BusPtr &bus : buses)
        {
            for (auto it = bus->stops.begin(); it != bus->stops.end(); ++it)
            {
                auto it_next = next(it);
                if (it_next == bus->stops.end())
                    break;

                StopPtr &from = *it;
                StopPtr &to = *it_next;

                auto it_road_route = road_route_length.find(from);
                if (it_road_route == road_route_length.end())
                    return;
                auto it_length = it_road_route->second.find(to);
                if (it_length == it_road_route->second.end())
                    return;
                double road_distance = it_length->second; // weight, в метрах

                Graph::VertexId vertex_id_from = stop_and_bus_to_vertex_id[from][bus];
                Graph::VertexId vertex_id_to = stop_and_bus_to_vertex_id[to][bus];

                Edge edge{
                    .from = vertex_id_from,
                    .to = vertex_id_to,
                    .weight = road_distance
                };

                graph.AddEdge(edge);
                edgs_count++;

                if (not bus->ring)
                {
                    road_distance = road_route_length.at(to).at(from);
                    edge = {
                        .from = vertex_id_to,
                        .to = vertex_id_from,
                        .weight = road_distance
                    };
                    graph.AddEdge(edge);
                    edgs_count++;
                }
            }
        }

        // формируем рёбра пересадок для смены автобуса
        for (const auto &[stop, bus_to_vertex_id] : stop_and_bus_to_vertex_id)
        {
            bool is_few_buses = bus_to_vertex_id.size() > 1U;

            if (not is_few_buses)
                continue;

            for (auto it = bus_to_vertex_id.begin(); it != bus_to_vertex_id.end(); ++it)
            {
                const auto &[bus_from, vertex_id_from] = *it;

                Graph::VertexId begin_end_transfer_vertex_id = begin_end_transfer[stop];

                Edge edge{
                    .from = begin_end_transfer_vertex_id,
                    .to = vertex_id_from,
                    .weight = routing_settings.meters_past_while_wait_bus
                };
                graph.AddEdge(edge);
                edgs_count++;
                shadow_edges++;

                edge.from = vertex_id_from;
                edge.to = begin_end_transfer_vertex_id;
                graph.AddEdge(edge);
                edgs_count++;
                shadow_edges++;

                auto it_next = next(it);
                if (it_next == bus_to_vertex_id.end())
                    break;

                while (it_next != bus_to_vertex_id.end())
                {
                    const auto &[bus_to, vertex_id_to] = *it_next;

                    edge.from = vertex_id_from;
                    edge.to = vertex_id_to;
                    graph.AddEdge(edge);
                    edgs_count++;
                    trans_edges++;

                    edge.from = vertex_id_to;
                    edge.to = vertex_id_from;
                    graph.AddEdge(edge);
                    edgs_count++;
                    trans_edges++;

                    ++it_next;
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
    if (from == to)
        throw runtime_error("Same stops passed");

    // проверяем, что для остановок имеются вершины
    if (db.stop_and_bus_to_vertex_id.find(from) == db.stop_and_bus_to_vertex_id.end() or
        db.stop_and_bus_to_vertex_id.find(to) == db.stop_and_bus_to_vertex_id.end())
    {
        return std::nullopt;
    }

    Graph::VertexId vertex_id_from = 0U;
    Graph::VertexId vertex_id_to = 0U;

    bool is_from_vertex_transfer = false;
    bool is_to_vertex_transfer = false;

    if (auto it = db.begin_end_transfer.find(from);
        it != db.begin_end_transfer.end())
    {
        vertex_id_from = it->second;
        is_from_vertex_transfer = true;
    }
    else
        vertex_id_from = db.stop_and_bus_to_vertex_id[from].begin()->second;

    if (auto it = db.begin_end_transfer.find(to);
        it != db.begin_end_transfer.end())
    {
        vertex_id_to = it->second;
        is_to_vertex_transfer = true;
    }
    else
        vertex_id_to = db.stop_and_bus_to_vertex_id[to].begin()->second;

    std::optional<RouteInfo> router_info = router.BuildRoute(vertex_id_from, vertex_id_to);

    if (not router_info)
        return std::nullopt;

    RouteQueryAnswer result{
        .total_time = router_info->weight / db.routing_settings.bus_velocity_meters_min +
            db.routing_settings.bus_wait_time
    };
    if (is_from_vertex_transfer)
        result.total_time -= db.routing_settings.bus_wait_time;
    if (is_to_vertex_transfer)
        result.total_time -= db.routing_settings.bus_wait_time;

    size_t edge_count = router_info->edge_count;
    if (is_to_vertex_transfer)
        --edge_count;

    result.items.push_back(WaitItem{ .stop = from });

    BusItem bus_item;

    size_t begin_edge_idx = is_from_vertex_transfer ? 1U : 0U;

    for (size_t i = begin_edge_idx; i < edge_count; ++i)
    {
        Graph::EdgeId edge_id = router.GetRouteEdge(router_info->id, i);
        Graph::Edge edge = db.graph.GetEdge(edge_id);

        const auto &[stop_from, bus_from] = db.vertex_id_to_stop_and_bus[edge.from];
        const auto &[stop_to, bus_to] = db.vertex_id_to_stop_and_bus[edge.to];

        if (bus_from == bus_to)
        {
            ++bus_item.span_count;
            bus_item.time += edge.weight / db.routing_settings.bus_velocity_meters_min;
        }
        else
        {
            if (bus_item.span_count == 0U)
                throw runtime_error("span_count must not to be zero");

            bus_item.bus = bus_from;
            result.items.push_back(bus_item);
            bus_item = {};

            result.items.push_back(WaitItem{ .stop = stop_to });
        }

        if (i == (edge_count - 1U))
        {
            bus_item.bus = bus_from;
            result.items.push_back(bus_item);
        }
    }

    router.ReleaseRoute(router_info->id);
    return result;
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

    const vector<Node> &stat_requests = root.at("stat_requests"s).AsArray();

    os << "[" << '\n';

    for (auto it = stat_requests.begin(); it != stat_requests.end(); ++it)
    {
        const map<string, Node> &req = it->AsMap();
        const string &type = req.at("type"s).AsString();
        const string &name = req.at("name"s).AsString();
        int id = req.at("id"s).AsInt();

        os << "  {" << '\n';

        os << "    \"request_id\": " << id << ",\n";

        if (type == "Bus")
        {
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
        ASSERT_EQUAL(db.graph.GetVertexCount(), 7U);
        ASSERT_EQUAL(db.graph.GetEdgeCount(), 14U);
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
        ASSERT_EQUAL(db.graph.GetVertexCount(), 9U);
        ASSERT_EQUAL(db.graph.GetEdgeCount(), 22U);
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
        ASSERT_EQUAL(db.graph.GetVertexCount(), 5U);
        ASSERT_EQUAL(db.graph.GetEdgeCount(), 6U);
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
        ASSERT_EQUAL(db.graph.GetVertexCount(), 7U);
        ASSERT_EQUAL(db.graph.GetEdgeCount(), 13U);
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
        ASSERT_EQUAL(db.graph.GetVertexCount(), 9U);
        ASSERT_EQUAL(db.graph.GetEdgeCount(), 22U);
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
        ASSERT_EQUAL(db.graph.GetVertexCount(), 11U);
        ASSERT_EQUAL(db.graph.GetEdgeCount(), 24U);

        Router router{db.graph};

        Graph::VertexId stop1_bus1_vertex_id = db.stop_and_bus_to_vertex_id[stop1][bus1];
        Graph::VertexId stop2_bus1_vertex_id = db.stop_and_bus_to_vertex_id[stop2][bus1];
        Graph::VertexId stop3_bus1_vertex_id = db.stop_and_bus_to_vertex_id[stop3][bus1];

        Graph::VertexId stop3_bus2_vertex_id = db.stop_and_bus_to_vertex_id[stop3][bus2];
        Graph::VertexId stop4_bus2_vertex_id = db.stop_and_bus_to_vertex_id[stop4][bus2];
        Graph::VertexId stop5_bus2_vertex_id = db.stop_and_bus_to_vertex_id[stop5][bus2];

        Graph::VertexId stop3_bus3_vertex_id = db.stop_and_bus_to_vertex_id[stop3][bus3];
        Graph::VertexId stop6_bus3_vertex_id = db.stop_and_bus_to_vertex_id[stop6][bus3];

        Graph::VertexId stop7_bus4_vertex_id = db.stop_and_bus_to_vertex_id[stop7][bus4];
        Graph::VertexId stop8_bus4_vertex_id = db.stop_and_bus_to_vertex_id[stop8][bus4];

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
        ASSERT_EQUAL(edge.to, stop3_bus2_vertex_id);
        edge_id = router.GetRouteEdge(route_info.id, 3);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop3_bus2_vertex_id);
        ASSERT_EQUAL(edge.to, stop4_bus2_vertex_id);
        edge_id = router.GetRouteEdge(route_info.id, 4);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop4_bus2_vertex_id);
        ASSERT_EQUAL(edge.to, stop5_bus2_vertex_id);

        route_info = router.BuildRoute(stop5_bus2_vertex_id, stop1_bus1_vertex_id).value();
        ASSERT(AssertDouble(route_info.weight, 6600.0));
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
        ASSERT_EQUAL(edge.to, stop3_bus1_vertex_id);
        edge_id = router.GetRouteEdge(route_info.id, 3);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop3_bus1_vertex_id);
        ASSERT_EQUAL(edge.to, stop2_bus1_vertex_id);
        edge_id = router.GetRouteEdge(route_info.id, 4);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop2_bus1_vertex_id);
        ASSERT_EQUAL(edge.to, stop1_bus1_vertex_id);

        router.ReleaseRoute(route_info.id);

        route_info = router.BuildRoute(stop1_bus1_vertex_id, stop6_bus3_vertex_id).value();
        ASSERT(AssertDouble(route_info.weight, 5700.0));
        ASSERT_EQUAL(route_info.edge_count, 4U);
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
        ASSERT_EQUAL(edge.to, stop3_bus3_vertex_id);
        edge_id = router.GetRouteEdge(route_info.id, 3);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop3_bus3_vertex_id);
        ASSERT_EQUAL(edge.to, stop6_bus3_vertex_id);

        router.ReleaseRoute(route_info.id);

        route_info = router.BuildRoute(stop5_bus2_vertex_id, stop6_bus3_vertex_id).value();
        ASSERT(AssertDouble(route_info.weight, 5300.0));
        ASSERT_EQUAL(route_info.edge_count, 4U);
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
        ASSERT_EQUAL(edge.to, stop3_bus3_vertex_id);
        edge_id = router.GetRouteEdge(route_info.id, 3);
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
}

void TestParseRouteQuery()
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
        * 3 (transfer) <-> 3-bus1
        * 3 (transfer) <-> 3-bus2
        * 3 (transfer) <-> 3-bus3
        */
        ASSERT_EQUAL(db.graph.GetVertexCount(), 11U);
        ASSERT_EQUAL(db.graph.GetEdgeCount(), 24U);

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
        ASSERT_EQUAL(db.graph.GetVertexCount(), 17U);
        ASSERT_EQUAL(db.graph.GetEdgeCount(), 80U);

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

        ASSERT_EQUAL(db.graph.GetVertexCount(), 9U);
        ASSERT_EQUAL(db.graph.GetEdgeCount(), 24U);

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

        ASSERT_EQUAL(db.graph.GetVertexCount(), 9U);
        ASSERT_EQUAL(db.graph.GetEdgeCount(), 24U);

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
    RUN_TEST(tr, TestParse);
}

void Profile()
{
}

int main()
{
    using namespace std::chrono;

    TestAll();

    // Parse(cin, cout);
    return 0;
}