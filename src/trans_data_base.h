#pragma once
#include "trans_types.h"
#include "render_types.h"

struct DataBase
{
    Stops stops;
    Buses buses;

    StopsSorted sorted_stops;

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

    RenderSettings render_settings{};

    template <typename Key, typename Value>
    using UnorderedMap = unordered_map<Key, Value, NamePtrHasher<Key>, NamePtrKeyEqual<Key>>;

    UnorderedMap<BusPtr, BusInfo> buses_info;
    UnorderedMap<StopPtr, UnorderedMap<StopPtr, size_t>> road_route_length; // в метрах

    DirectedWeightedGraph graph{0};

    string map_svg;

    void CreateInfo(size_t bus_wait_time = 0U, double bus_velocity = 0.0, RenderSettings rs = {}, bool output = false);

    // Дорожная единица представляет из себя структуру, в которой содержится
    // остановка, маршрут и номер остановки в этом маршруте. Комбинации этих трёх состовляющих
    // достаточно, чтобы задать уникальную вершину без возникновения конфликтов с другими вершинами
    // Сигнатура: [stop][bus][position_in_bus] = vertex_id
    UnorderedMap<StopPtr, UnorderedMap<BusPtr, unordered_map<size_t, Graph::VertexId>>> route_unit_to_vertex_id;
    // Сигнатура: vertex_id = [stop][bus][position_in_bus]
    unordered_map<Graph::VertexId, tuple<StopPtr, BusPtr, size_t>> vertex_id_to_route_unit;

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