#pragma once
#include "graph.h"
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
struct NamePtrKeyLess
{
    bool operator()(const PtrWithName &lhs, const PtrWithName &rhs) const
    {
        return lhs->name < rhs->name;
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
using StopsSorted = set<StopPtr, NamePtrKeyLess<StopPtr>>;

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