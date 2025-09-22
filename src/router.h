#pragma once
#include "graph.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iterator>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>
#include <queue>
#include <functional>

namespace Graph
{

template <typename Weight>
class Router
{
private:
    using Graph = DirectedWeightedGraph<Weight>;

public:
    Router(const Graph &graph);

    using RouteId = uint64_t;

    struct RouteInfo
    {
        RouteId id;
        Weight weight;
        size_t edge_count;
    };

    std::optional<RouteInfo> BuildRoute(VertexId from, VertexId to) const;
    EdgeId GetRouteEdge(RouteId route_id, size_t edge_idx) const;
    void ReleaseRoute(RouteId route_id);

private:
    const Graph &graph_;

    // Структура для хранения предварительно вычисленных маршрутов
    struct VertexRoutes
    {
        std::vector<Weight> distances;
        std::vector<std::optional<EdgeId>> prev_edges;
    };

    std::vector<VertexRoutes> all_routes_;

    // Кэш развернутых маршрутов
    using ExpandedRoute = std::vector<EdgeId>;
    mutable RouteId next_route_id_ = 0;
    mutable std::unordered_map<RouteId, ExpandedRoute> expanded_routes_cache_;

    // Вспомогательная структура для очереди с приоритетом
    struct QueueElement
    {
        VertexId vertex;
        Weight distance;

        bool operator>(const QueueElement &other) const
        {
            return distance > other.distance;
        }
    };

    // Вычисление маршрутов из одной вершины с помощью Дейкстры
    VertexRoutes ComputeRoutesFromVertex(VertexId source) const;
};

template <typename Weight>
Router<Weight>::Router(const Graph &graph)
    : graph_(graph),
        all_routes_(graph.GetVertexCount())
{
    // Предварительно вычисляем маршруты из каждой вершины
    for (VertexId vertex = 0; vertex < graph.GetVertexCount(); ++vertex)
    {
        all_routes_[vertex] = ComputeRoutesFromVertex(vertex);
    }
}

template <typename Weight>
typename Router<Weight>::VertexRoutes Router<Weight>::ComputeRoutesFromVertex(VertexId source) const
{
    const size_t vertex_count = graph_.GetVertexCount();
    VertexRoutes result;
    result.distances.assign(vertex_count, std::numeric_limits<Weight>::max());
    result.prev_edges.assign(vertex_count, std::nullopt);

    // Приоритетная очередь для выбора вершины с минимальным расстоянием
    std::priority_queue<QueueElement, std::vector<QueueElement>, std::greater<QueueElement>> queue;

    result.distances[source] = 0;
    queue.push({source, 0});

    while (!queue.empty())
    {
        auto [current_vertex, current_distance] = queue.top();
        queue.pop();

        // Если мы уже нашли более короткий путь до этой вершины, пропускаем
        if (current_distance > result.distances[current_vertex])
        {
            continue;
        }

        // Релаксация всех исходящих ребер
        for (EdgeId edge_id : graph_.GetIncidentEdges(current_vertex))
        {
            const auto &edge = graph_.GetEdge(edge_id);
            Weight new_distance = result.distances[current_vertex] + edge.weight;

            if (new_distance < result.distances[edge.to])
            {
                result.distances[edge.to] = new_distance;
                result.prev_edges[edge.to] = edge_id;
                queue.push({edge.to, new_distance});
            }
        }
    }

    return result;
}

template <typename Weight>
std::optional<typename Router<Weight>::RouteInfo> Router<Weight>::BuildRoute(VertexId from, VertexId to) const
{
    const auto &routes = all_routes_[from];

    // Если путь не существует
    if (routes.distances[to] == std::numeric_limits<Weight>::max())
    {
        return std::nullopt;
    }

    // Восстанавливаем путь по обратным ссылкам
    std::vector<EdgeId> edges;
    VertexId current = to;

    while (current != from)
    {
        const auto &edge_id = routes.prev_edges[current];
        if (!edge_id)
        {
            // Это не должно происходить при корректных данных
            return std::nullopt;
        }
        edges.push_back(*edge_id);
        current = graph_.GetEdge(*edge_id).from;
    }

    std::reverse(edges.begin(), edges.end());

    const RouteId route_id = next_route_id_++;
    const size_t route_edge_count = edges.size();
    expanded_routes_cache_[route_id] = std::move(edges);

    return RouteInfo{route_id, routes.distances[to], route_edge_count};
}

template <typename Weight>
EdgeId Router<Weight>::GetRouteEdge(RouteId route_id, size_t edge_idx) const
{
    return expanded_routes_cache_.at(route_id)[edge_idx];
}

template <typename Weight>
void Router<Weight>::ReleaseRoute(RouteId route_id)
{
    expanded_routes_cache_.erase(route_id);
}

} // namespace Graph