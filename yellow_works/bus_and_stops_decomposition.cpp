#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <optional>
#include <variant>
#include <cmath>
#include <numeric>
#include <limits>
#include <tuple>

using namespace std;

enum class QueryType
{
    NewBus,
    BusesForStop,
    StopsForBus,
    AllBuses
};

struct Query
{
    QueryType type;
    string bus;
    string stop;
    vector<string> stops;
};

istream &operator>>(istream &is, Query &q)
{
    string query;
    is >> query;
    if (query == "NEW_BUS")
    {
        q.type = QueryType::NewBus;

        is >> q.bus;
        int stop_count;
        is >> stop_count;
        q.stops.resize(stop_count);
        for (string &stop : q.stops)
        {
            is >> stop;
        }
    }
    else if (query == "BUSES_FOR_STOP")
    {
        q.type = QueryType::BusesForStop;

        is >> q.stop;
    }
    else if (query == "STOPS_FOR_BUS")
    {
        q.type = QueryType::StopsForBus;

        is >> q.bus;
    }
    else if (query == "ALL_BUSES")
    {
        q.type = QueryType::AllBuses;
    }
    return is;
}

struct BusesForStopResponse
{
    const vector<string> buses;
};

ostream &operator<<(ostream &os, const BusesForStopResponse &r)
{
    if (r.buses.empty())
    {
        os << "No stop";
    }
    else
    {
        for (const string &bus : r.buses)
        {
            os << bus << " ";
        }
    }
    return os;
}

struct StopsForBusResponse
{
    StopsForBusResponse(const string &ref_bus, const map<string, vector<string>> &ref_buses_to_stops, const map<string, vector<string>> &ref_stops_to_buses)
        : bus(ref_bus),
          buses_to_stops(ref_buses_to_stops),
          stops_to_buses(ref_stops_to_buses) {}

    const string &bus;
    const map<string, vector<string>> &buses_to_stops;
    const map<string, vector<string>> &stops_to_buses;
};

ostream &operator<<(ostream &os, const StopsForBusResponse &r)
{
    if (r.buses_to_stops.count(r.bus) == 0)
    {
        os << "No bus";
    }
    else
    {
        auto &stops = r.buses_to_stops.at(r.bus);
        for (auto stop = stops.begin(); stop != stops.end(); ++stop)
        {
            os << "Stop " << *stop << ": ";
            if (r.stops_to_buses.at(*stop).size() == 1)
            {
                os << "no interchange";
            }
            else
            {
                for (const string &other_bus : r.stops_to_buses.at(*stop))
                {
                    if (r.bus != other_bus)
                    {
                        os << other_bus << " ";
                    }
                }
            }

            if (stop != prev(stops.end()))
            {
                os << endl;
            }
        }
    }
    return os;
}

struct AllBusesResponse
{
    AllBusesResponse(const map<string, vector<string>> &ref_buses_to_stops)
        : buses_to_stops(ref_buses_to_stops) {}
    const map<string, vector<string>> &buses_to_stops;
};

ostream &operator<<(ostream &os, const AllBusesResponse &r)
{
    if (r.buses_to_stops.empty())
    {
        os << "No buses";
    }
    else
    {
        for (const auto &bus_item : r.buses_to_stops)
        {
            os << "Bus " << bus_item.first << ": ";
            for (const string &stop : bus_item.second)
            {
                os << stop << " ";
            }
            os << endl;
        }
    }
    return os;
}

class BusManager
{
public:
    void AddBus(const string &bus, const vector<string> &stops)
    {
        buses_to_stops[bus] = stops;
        for (const string &stop : stops)
        {
            stops_to_buses[stop].push_back(bus);
        }
    }

    BusesForStopResponse GetBusesForStop(const string &stop) const
    {
        if (stops_to_buses.count(stop))
        {
            return {stops_to_buses.at(stop)};
        }
        return {};
    }

    StopsForBusResponse GetStopsForBus(const string &bus) const
    {
        return StopsForBusResponse{bus, buses_to_stops, stops_to_buses};
    }

    AllBusesResponse GetAllBuses() const
    {
        return AllBusesResponse{buses_to_stops};
    }

private:
    map<string, vector<string>> buses_to_stops;
    map<string, vector<string>> stops_to_buses;
};

int main()
{
    int query_count;
    Query q;

    cin >> query_count;

    BusManager bm;
    for (int i = 0; i < query_count; ++i)
    {
        cin >> q;
        switch (q.type)
        {
        case QueryType::NewBus:
            bm.AddBus(q.bus, q.stops);
            break;
        case QueryType::BusesForStop:
            cout << bm.GetBusesForStop(q.stop) << endl;
            break;
        case QueryType::StopsForBus:
            cout << bm.GetStopsForBus(q.bus) << endl;
            break;
        case QueryType::AllBuses:
            cout << bm.GetAllBuses() << endl;
            break;
        }
    }
    return 0;
}
