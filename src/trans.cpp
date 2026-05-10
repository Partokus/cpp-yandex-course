#include "trans.h"

using namespace std;

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

void DataBase::CreateInfo(size_t bus_wait_time, double bus_velocity, RenderSettings rs, bool output)
{
    CreateRoutingSettings(bus_wait_time, bus_velocity);
    render_settings = rs;

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
            size_t stop_position_in_bus = it - bus->stops.begin();
            route_unit_to_vertex_id[stop][bus][stop_position_in_bus] = _vertex_id;
            vertex_id_to_route_unit[_vertex_id] = make_tuple(stop, bus, stop_position_in_bus);
            ++_vertex_id;

            if (it_next == bus->stops.end())
                break;

            const StopPtr &next_stop = *it_next;

            info.route_length_geo += CalcGeoDistance(
                stop->latitude, stop->longitude,
                next_stop->latitude, next_stop->longitude
            );

            std::optional<size_t> road_distance = CalcRoadDistance(stop, next_stop);
            if (road_distance)
                info.route_length_road += *road_distance;
            else
            {
                throw runtime_error("Can't calculate road distance: bus " +
                    bus->name + ", stop " + stop->name + ", next_stop " + next_stop->name);
            }
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

void DataBase::CreateRoutingSettings(size_t bus_wait_time, double bus_velocity)
{
    routing_settings.bus_wait_time = bus_wait_time;
    routing_settings.bus_velocity = bus_velocity;
    routing_settings.bus_velocity_meters_min = bus_velocity * 1000.0 / 60.0;
    routing_settings.meters_past_while_wait_bus = bus_wait_time *
                                                    routing_settings.bus_velocity_meters_min;
}

// return meters
std::optional<size_t> DataBase::CalcRoadDistance(const StopPtr &lhs, const StopPtr &rhs) const
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

void DataBase::CreateGraph(bool debug)
{
    if (debug)
    {
        size_t num = 0U;
        for (const auto &[stop, bus_to_stop_pos] : route_unit_to_vertex_id)
        {
            for (const auto &[bus, stop_pos_to_vertex_id] : bus_to_stop_pos)
            {
                for (const auto &[stop_pos, vertex_id] : stop_pos_to_vertex_id)
                {
                    cout << num++ << ": vertex_id[" << vertex_id << "] = { " <<
                        "stop( " << stop->name << " ), bus( " << bus->name << "), " <<
                        "pos( " << stop_pos << " ) }" << endl;
                }
            }
        }
    }

    // формируем абстрактные вершины для пересадок
    for (const auto &[stop, bus_to_stop_pos] : route_unit_to_vertex_id)
    {
        // если у остановки только один автобус и у этой остановки
        // на данном автобусе нет её копий с другой следующей остановкой
        // (то есть у автобуса все остановки уникальны)
        if (bus_to_stop_pos.size() <= 1U and bus_to_stop_pos.begin()->second.size() <= 1)
            continue;

        abstract_stop_to_vertex_id[stop] = _vertex_id;
        vertex_id_to_abstract_stop[_vertex_id] = stop;
        ++_vertex_id;
    }

    if (debug)
    {
        size_t num = 0U;
        for (const auto &[stop, vertex_id] : abstract_stop_to_vertex_id)
        {
            cout << num++ << ": vertex_id[" << vertex_id << "] = { " <<
                "stop( " << stop->name << " )" << endl;
        }
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
            size_t from_pos = it - bus->stops.begin();
            size_t to_pos = it_next - bus->stops.begin();

            auto it_road_route = road_route_length.find(from);
            if (it_road_route == road_route_length.end())
                return;
            auto it_length = it_road_route->second.find(to);
            if (it_length == it_road_route->second.end())
                return;
            double road_distance = it_length->second; // weight, в метрах

            Graph::VertexId vertex_id_from = route_unit_to_vertex_id.at(from).at(bus).at(from_pos);
            Graph::VertexId vertex_id_to = route_unit_to_vertex_id.at(to).at(bus).at(to_pos);

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
                    size_t last_stop_pos = bus->stops.size() - 1U;
                    Graph::VertexId vertex_id_from_with_wait_bus = route_unit_to_vertex_id.at(from).at(bus).at(last_stop_pos);
                    edge.from = vertex_id_from_with_wait_bus;
                    edge.weight += routing_settings.meters_past_while_wait_bus;
                    graph.AddEdge(edge);
                }
            }
        }
    }

    for (const auto &[stop, bus_to_stop_pos] : route_unit_to_vertex_id)
    {
        // если у остановки только один автобус и у этой остановки
        // на данном автобусе нет её копий с другой следующей остановкой
        // (то есть у автобуса все остановки уникальны)
        if (bus_to_stop_pos.size() <= 1U and bus_to_stop_pos.begin()->second.size() <= 1)
            continue;

        for (auto it = bus_to_stop_pos.begin(); it != bus_to_stop_pos.end(); ++it)
        {
            const auto &[bus, stop_pos_to_vertex_id] = *it;

            for (auto it2 = stop_pos_to_vertex_id.begin(); it2 != stop_pos_to_vertex_id.end(); ++it2)
            {
                const auto &[stop_pos, vertex_id] = *it2;

                Graph::VertexId shadow_vertex_id = abstract_stop_to_vertex_id[stop];

                Edge edge{
                    .from = shadow_vertex_id,
                    .to = vertex_id,
                    .weight = routing_settings.meters_past_while_wait_bus / 2.0
                };
                graph.AddEdge(edge);

                edge.from = vertex_id;
                edge.to = shadow_vertex_id;
                graph.AddEdge(edge);
            }
        }
    }
}

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
        auto [it, inserted] = stops.insert(move(stop));
        result->stops.push_back(*it);
        it->get()->buses.insert(result);
    };

    const vector<Node> json_stops = req.at("stops"s).AsArray();

    if (json_stops.empty())
        throw runtime_error("bus don't have stops");
    if (json_stops.size() == 1U)
        throw runtime_error("bus have only one stop");

    for (auto it = json_stops.begin(); it != json_stops.end(); ++it)
    {
        push_stop(it->AsString());
    }

    if (result->ring)
    {
        const string &first_stop = json_stops.front().AsString();
        const string &last_stop = json_stops.back().AsString();
        if (first_stop != last_stop)
            throw runtime_error("for ring bus " + result->name + "first and last stops not equal");
    }

    return result;
}

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

    bool is_from_abstract_vertex = false;
    bool is_to_abstract_vertex = false;

    if (auto it = db.abstract_stop_to_vertex_id.find(from);
        it != db.abstract_stop_to_vertex_id.end())
    {
        vertex_id_from = it->second;
        is_from_abstract_vertex = true;
    }
    else
        vertex_id_from = db.route_unit_to_vertex_id.at(from).cbegin()->second.cbegin()->second;

    if (auto it = db.abstract_stop_to_vertex_id.find(to);
        it != db.abstract_stop_to_vertex_id.end())
    {
        vertex_id_to = it->second;
        is_to_abstract_vertex = true;
    }
    else
        vertex_id_to = db.route_unit_to_vertex_id.at(to).cbegin()->second.cbegin()->second;

    std::optional<RouteInfo> router_info = router.BuildRoute(vertex_id_from, vertex_id_to);

    if (not router_info)
        return std::nullopt;

    RouteQueryAnswer result{};

    // суммарное время равно длина дороги в метрах, делённая на скорость (метры/мин), плюс
    // время на ожидание первого автобуса
    result.total_time = router_info->weight / db.routing_settings.bus_velocity_meters_min +
        db.routing_settings.bus_wait_time;

    // если первая вершина или последняя абстрактные, сделанная для пересадки между автобусами,
    // то вычитаем время ожидания автобуса, так как для перехода между абстрактной и физической остановками
    // тратится время на ожидание автобуса
    if (is_from_abstract_vertex)
        result.total_time -= db.routing_settings.bus_wait_time / 2.0;
    if (is_to_abstract_vertex)
        result.total_time -= db.routing_settings.bus_wait_time / 2.0;

    size_t edge_count = router_info->edge_count;

    // последнее ребро не учитываем, так как оно ведёт к абстрактной остановке
    if (is_to_abstract_vertex)
        --edge_count;

    result.items.push_back(WaitItem{ .stop = from });

    BusItem bus_item;

    // не учитываем первое ребро, если он ведёт от абстрактной вершины до настоящей,
    // принадлежащей конкретной остановке
    size_t begin_edge_idx = is_from_abstract_vertex ? 1U : 0U;

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

        const auto &[stop_from, bus_from, stop_pos_from] = db.vertex_id_to_route_unit.at(edge.from);

        auto it_to = db.vertex_id_to_route_unit.find(edge.to);
        if (it_to == db.vertex_id_to_route_unit.end())
        {
            StopPtr stop_to = db.vertex_id_to_abstract_stop.at(edge.to);
            bus_item.bus = bus_from;
            push_items(bus_item, WaitItem{ .stop = stop_to });
            // в следующем цикле stop_from будет равен абстрактной вершине,
            // поэтому прыгаем через одну вершину
            ++i;
            continue;
        }

        size_t last_stop_pos = bus_from->stops.size() - 1U;
        if (bus_from->ring and stop_pos_from == last_stop_pos)
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

void Parse(istream &is, ostream &os, DataBase &db)
{
    using namespace Json;

    os.precision(6);

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

    db.sorted_stops = { db.stops.begin(), db.stops.end() };

    const map<string, Node> &routing_settings = root.at("routing_settings"s).AsMap();
    const size_t bus_wait_time = routing_settings.at("bus_wait_time"s).AsInt();
    const double bus_velocity = routing_settings.at("bus_velocity"s).AsDouble();

    const map<string, Node> &render_settings_json = root.at("render_settings"s).AsMap();

    db.CreateInfo(bus_wait_time, bus_velocity, MakeRenderSettigs(render_settings_json), true);

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
                        os << "      \"" << it_bus->lock()->name << '\"';
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

            // if (answer and answer->total_time > 1508.20 and answer->total_time < 2000.0)
            // {
            //     throw runtime_error("got here!");
            // }

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
        else if (type == "Map")
        {
            if (db.map_svg.empty())
            {
                db.map_svg = CreateMap(db);
            }
            os << "    \"map\": \"" << db.map_svg << "\"\n";
        }

        os << "  }";
        if (next(it) != stat_requests.end())
            os << ',';
        os << '\n';
    }

    os << "]";
}