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


    }
    else if (query == "ALL_BUSES")
    {
        q.type = QueryType::AllBuses;


    }
    return is;
}

struct BusesForStopResponse
{
    vector<string> buses = {};
};

ostream &operator<<(ostream &os, const BusesForStopResponse &r)
{
    if (r.buses.empty())
    {
        os << "No stop" << endl;
    }
    else
    {
        for (const string &bus : r.buses)
        {
            os << bus << " ";
        }
        os << endl;
    }
    return os;
}

struct StopsForBusResponse
{
    // Наполните полями эту структуру
};

ostream &operator<<(ostream &os, const StopsForBusResponse &r)
{
    // Реализуйте эту функцию
    return os;
}

struct AllBusesResponse
{
    // Наполните полями эту структуру
};

ostream &operator<<(ostream &os, const AllBusesResponse &r)
{
    // Реализуйте эту функцию
    return os;
}

class BusManager
{
public:
    void AddBus(const string &bus, const vector<string> &stops)
    {
        for (const string &stop : stops)
        {
            stops_to_buses[stop].push_back(bus);
        }
    }

    BusesForStopResponse GetBusesForStop(const string &stop) const
    {
        if (stops_to_buses.count(stop) == 0)
        {
            return {};
        }
        else
        {
            return {.buses = stops_to_buses.at(stop)};
        }
    }

    StopsForBusResponse GetStopsForBus(const string &bus) const
    {
        // Реализуйте этот метод
    }

    AllBusesResponse GetAllBuses() const
    {
        // Реализуйте этот метод
    }

private:
    map<string, vector<string>> buses_to_stops;
    map<string, vector<string>> stops_to_buses;
};

int main()
{
    int q;
    cin >> q;

    map<string, vector<string>> buses_to_stops, stops_to_buses;

    for (int i = 0; i < q; ++i)
    {
        string operation_code;
        cin >> operation_code;

        if (operation_code == "NEW_BUS")
        {

            string bus;
            cin >> bus;
            int stop_count;
            cin >> stop_count;
            vector<string> &stops = buses_to_stops[bus];
            stops.resize(stop_count);
            for (string &stop : stops)
            {
                cin >> stop;
                stops_to_buses[stop].push_back(bus);
            }
        }
        else if (operation_code == "BUSES_FOR_STOP")
        {
            string stop;
            cin >> stop;
            if (stops_to_buses.count(stop) == 0)
            {
                cout << "No stop" << endl;
            }
            else
            {
                for (const string &bus : stops_to_buses[stop])
                {
                    cout << bus << " ";
                }
                cout << endl;
            }
        }
        else if (operation_code == "STOPS_FOR_BUS")
        {

            string bus;
            cin >> bus;
            if (buses_to_stops.count(bus) == 0)
            {
                cout << "No bus" << endl;
            }
            else
            {
                for (const string &stop : buses_to_stops[bus])
                {
                    cout << "Stop " << stop << ": ";
                    if (stops_to_buses[stop].size() == 1)
                    {
                        cout << "no interchange";
                    }
                    else
                    {
                        for (const string &other_bus : stops_to_buses[stop])
                        {
                            if (bus != other_bus)
                            {
                                cout << other_bus << " ";
                            }
                        }
                    }
                    cout << endl;
                }
            }
        }
        else if (operation_code == "ALL_BUSES")
        {

            if (buses_to_stops.empty())
            {
                cout << "No buses" << endl;
            }
            else
            {
                for (const auto &bus_item : buses_to_stops)
                {
                    cout << "Bus " << bus_item.first << ": ";
                    for (const string &stop : bus_item.second)
                    {
                        cout << stop << " ";
                    }
                    cout << endl;
                }
            }
        }
    }
    return 0;
}
