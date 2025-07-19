#include <profile.h>
#include <test_runner.h>

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

    bool operator==(const Stop &o) const
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

    template <typename Key, typename Value>
    using Map = unordered_map<Key, Value, NamePtrHasher<Key>, NamePtrKeyEqual<Key>>;

    Map<BusPtr, BusInfo> buses_info;
    Map<StopPtr, Map<StopPtr, size_t>> road_route_length;

    void CreateBusesInfo()
    {
        for (const BusPtr &bus : buses)
        {
            auto &info = buses_info[bus];

            if (bus->ring)
            {
                info.stops_on_route = bus->stops.size() + 1U;
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
                if (it_next != bus->stops.end())
                {
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
                else
                    break;
            }

            if (bus->ring)
            {
                info.route_length_geo += CalcGeoDistance(
                    bus->stops.back()->latitude,   bus->stops.back()->longitude,
                    bus->stops.front()->latitude, bus->stops.front()->longitude
                );

                std::optional<size_t> road_distance = CalcRoadDistance(bus->stops.back(), bus->stops.front());
                if (road_distance)
                    info.route_length_road += *road_distance;
            }
            else
            {
                info.route_length_geo *= 2.0;
            }
        }
    }

private:
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
};

// X: latitude, longitude
StopPtr ParseAddStopQuery(istream &is, DataBase &db)
{
    string s;
    getline(is, s);
    istringstream iss(s);
    StopPtr result = make_shared<Stop>();

    if (iss.peek() == ' ')
        iss.ignore(1); // skip " "
    getline(iss, s, ':');
    result->name = move(s);
    iss.ignore(1); // skip " "

    getline(iss, s, ',');
    result->latitude = stod(s);
    iss.ignore(1); // skip " "

    getline(iss, s, ',');
    result->longitude = stod(s);

    if (not iss.rdbuf()->in_avail())
        return result;

    // parse D[i]m to stop#
    auto &road_route_length = db.road_route_length[result];

    while (iss.rdbuf()->in_avail())
    {
        iss >> s;
        if (s.back() != 'm')
            throw runtime_error("Expected 'm'");
        s.pop_back();

        const size_t meters = stoi(s);

        iss >> s;
        if (s != "to")
            throw runtime_error("Expected 'to'");

        iss.ignore(1); // skip space

        getline(iss, s, ',');

        auto [it_stop, inserted] = db.stops.insert(make_shared<Stop>(Stop{ move(s) }));

        road_route_length[*it_stop] = meters;

        auto &road_route_length_rhs = db.road_route_length[*it_stop];

        if (auto it = road_route_length_rhs.find(result);
            it == road_route_length_rhs.end())
        {
            road_route_length_rhs[result] = meters;
        }
    }

    return result;
}

// Bus X: описание маршрута
BusPtr ParseAddBusQuery(istream &is, Stops &stops)
{
    BusPtr result = make_shared<Bus>();

    string s;
    getline(is, s);
    istringstream iss(move(s));

    if (iss.peek() == ' ')
        iss.ignore(1); // skip " "
    getline(iss, s, ':');
    result->name = move(s);
    iss.ignore(1); // skip " "

    string stop_name;
    char separator;

    while (iss and (iss.peek() != '-' and iss.peek() != '>'))
    {
        stop_name += iss.peek();
        iss.ignore(1);
    }
    stop_name.pop_back();
    if (iss)
    {
        separator = iss.peek();
        if (separator != '>' and separator != '-')
            throw runtime_error("Invalid separator");
        result->ring = separator == '>';
    }

    iss.ignore(2); // skip separator and space

    auto push_stop = [&stops, &result](string && name)
    {
        StopPtr stop = make_shared<Stop>(move(Stop{ move(name) }));
        auto [it, inserted] = stops.insert(stop);
        result->stops.push_back(*it);
        it->get()->buses.insert(result);
    };

    push_stop(move(stop_name));

    while (iss)
    {
        getline(iss, stop_name, separator);
        if (iss.peek() == ' ')
            iss.ignore(1); // skip space
        if (iss)
            stop_name.pop_back();

        // последнюю остановку не надо добавлять
        // для кольцевого маршрута
        if (result->ring and not iss)
            break;

        push_stop(move(stop_name));
    }
    return result;
}

void Parse(istream &is, ostream &os)
{
    DataBase db{};

    os.precision(6);

    size_t data_base_requests_count = 0U;
    is >> data_base_requests_count;

    for (size_t i = 0U; i < data_base_requests_count; ++i)
    {
        string first_word;
        is >> first_word;

        if (first_word == "Stop")
        {
            StopPtr stop = ParseAddStopQuery(is, db);
            auto [it, inserted] = db.stops.insert(stop);
            if (not inserted)
            {
                it->get()->latitude = stop->latitude;
                it->get()->longitude = stop->longitude;
            }
        }
        else if (first_word == "Bus")
        {
            BusPtr bus = ParseAddBusQuery(is, db.stops);
            db.buses.insert(move(bus));
        }
    }

    db.CreateBusesInfo();

    size_t requests_count = 0U;
    is >> requests_count;

    for (size_t i = 0U; i < requests_count; ++i)
    {
        string request;
        is >> request;
        is.ignore(1); // ignore space

        if (request == "Bus")
        {
            auto bus = make_shared<Bus>(Bus{});
            getline(is, bus->name);

            os << "Bus " << bus->name << ": ";

            if (auto it = db.buses_info.find(bus); it != db.buses_info.end())
            {
                auto &info = it->second;
                os << info.stops_on_route    << " stops on route, "
                   << info.unique_stops      << " unique stops, "
                   << info.route_length_road << " route length, "
                   << info.curvature()       << " curvature";
            }
            else
                os << "not found";
        }
        else if (request == "Stop")
        {
            auto stop = make_shared<Stop>(Stop{});
            getline(is, stop->name);

            os << "Stop " << stop->name << ": ";

            if (auto it = db.stops.find(stop); it != db.stops.end())
            {
                const StopPtr &stop = *it;
                if (not stop->buses.empty())
                {
                    os << "buses ";
                    bool first = true;
                    for (const auto &bus : stop->buses)
                    {
                        if (first)
                            first = false;
                        else
                            os << " ";
                        os << bus->name;
                    }
                }
                else
                    os << "no buses";
            }
            else
                os << "not found";
        }

        os << '\n';
    }
}

void TestParseAddStopQuery()
{
    {
        DataBase db;
        istringstream is("Empire   Street Building 5: 55.611087, 2.34");
        StopPtr stop = ParseAddStopQuery(is, db);
        Stop expect{
            .name = "Empire   Street Building 5",
            .latitude = 55.611087,
            .longitude = 2.34
        };
        ASSERT_EQUAL(stop->name, expect.name);
        ASSERT_EQUAL(stop->latitude, expect.latitude);
        ASSERT_EQUAL(stop->longitude, expect.longitude);
    }
    {
        DataBase db;
        istringstream is("E: 21, 89.5");
        StopPtr stop = ParseAddStopQuery(is, db);
        Stop expect{
            .name = "E",
            .latitude = 21.0,
            .longitude = 89.5
        };
        ASSERT_EQUAL(stop->name, expect.name);
        ASSERT_EQUAL(stop->latitude, expect.latitude);
        ASSERT_EQUAL(stop->longitude, expect.longitude);
    }

    // with Dim to stop#
    {
        DataBase db;
        istringstream is("Univer: 21.666, 89.591, 3900m to Marushkino");
        StopPtr stop = ParseAddStopQuery(is, db);
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
        DataBase db;
        istringstream is("Univer: 21.666, 89.591, 3900m to Marushkino, 1000m to Sabotajnaya, 100000m to Meteornaya");
        StopPtr stop_univer = ParseAddStopQuery(is, db);
        is = istringstream("Meteornaya: 21.1, 89.2, 500m to Univer");
        StopPtr stop_meteornaya = ParseAddStopQuery(is, db);
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
    {
        StopPtr stop1 = make_shared<Stop>(Stop{"Biryulyovo  Zapadnoye"});
        StopPtr stop2 = make_shared<Stop>(Stop{"Biryusinka"});
        StopPtr stop3 = make_shared<Stop>(Stop{"Universam"});
        StopPtr stop4 = make_shared<Stop>(Stop{"Biryulyovo  Tovarnaya"});
        Stops stops{stop1, stop2, stop3, stop4};
        istringstream is("256: Biryulyovo  Zapadnoye - Biryusinka - Universam - Biryulyovo  Tovarnaya");
        BusPtr bus = ParseAddBusQuery(is, stops);
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
        StopPtr stop1 = make_shared<Stop>(Stop{"Biryulyovo Zapadnoye"});
        StopPtr stop2 = make_shared<Stop>(Stop{"Biryusinka"});
        StopPtr stop3 = make_shared<Stop>(Stop{"Universam"});
        StopPtr stop4 = make_shared<Stop>(Stop{"Biryulyovo Tovarnaya"});
        StopPtr stop5 = make_shared<Stop>(Stop{"Biryulyovo Passazhirskaya"});
        Stops stops{stop1, stop2, stop3, stop4, stop5};
        istringstream is("256: Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Tovarnaya > Biryulyovo Passazhirskaya > Biryulyovo Zapadnoye");
        BusPtr bus = ParseAddBusQuery(is, stops);
        Bus expect{
            .name = "256",
            .stops = {stop1, stop2, stop3, stop4, stop5},
            .ring = true
        };
        ASSERT_EQUAL(bus->stops.size(), expect.stops.size());
        ASSERT_EQUAL(bus->name, expect.name);
        for (int i = 0; i < 5; ++i)
            ASSERT(bus->stops[i].get() == expect.stops[i].get());
        ASSERT_EQUAL(bus->ring, expect.ring);
    }
    {
        StopPtr stop1 = make_shared<Stop>(Stop{"Biryulyovo Zapadnoye"});
        StopPtr stop2 = make_shared<Stop>(Stop{"Tomash"});
        Stops stops{stop1, stop2};
        istringstream is("My Bus: Biryulyovo Zapadnoye - Tomash");
        BusPtr bus = ParseAddBusQuery(is, stops);
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
        StopPtr stop1 = make_shared<Stop>(Stop{"Biryulyovo"});
        StopPtr stop2 = make_shared<Stop>(Stop{"Tomash"});
        Stops stops{stop1, stop2};
        istringstream is("My Bus  HardBass: Biryulyovo - Tomash");
        BusPtr bus = ParseAddBusQuery(is, stops);
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

void TestDataBaseCreateBusesInfo()
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
        BusPtr bus3 = make_shared<Bus>(Bus{"843", {stop6, stop5, stop4, stop3, stop7}});
        bus3->ring = true;

        // уникальные остановки: Biryulyovo Zapadnoye, Biryusinka, Lepeshkina

        DataBase db;
        db.stops = {stop1, stop2, stop3, stop4, stop5, stop6, stop7};
        db.buses = {bus1, bus2, bus3};

        db.CreateBusesInfo();

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

        BusPtr bus1 = make_shared<Bus>(Bus{"256", {stop4, stop5, stop6, stop7, stop8}});
        bus1->ring = true;
        BusPtr bus2 = make_shared<Bus>(Bus{"750", {stop1, stop2, stop3}});

        DataBase db;
        db.stops = {stop1, stop2, stop3, stop4, stop5, stop6, stop7, stop8};
        db.buses = {bus1, bus2};

        db.CreateBusesInfo();

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
        db.road_route_length[stop2][stop3] = 1500;

        db.CreateBusesInfo();

        ASSERT_EQUAL(db.buses_info[bus1].stops_on_route, 5U);
        ASSERT_EQUAL(db.buses_info[bus1].unique_stops, 3U);
        ASSERT(db.buses_info[bus1].route_length_geo > 20938.0 and db.buses_info[bus1].route_length_geo < 20940.0);
    }
}

void TestParse()
{
    {
        istringstream iss(R"(
10
Stop Tolstopaltsevo: 55.611087, 37.20829
Stop Marushkino: 55.595884, 37.209755
Bus 256: Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Tovarnaya > Biryulyovo Passazhirskaya > Biryulyovo Zapadnoye
Bus 750: Tolstopaltsevo - Marushkino - Rasskazovka
Stop Rasskazovka: 55.632761, 37.333324
Stop Biryulyovo Zapadnoye: 55.574371, 37.6517
Stop Biryusinka: 55.581065, 37.64839
Stop Universam: 55.587655, 37.645687
Stop Biryulyovo Tovarnaya: 55.592028, 37.653656
Stop Biryulyovo Passazhirskaya: 55.580999, 37.659164
3
Bus 256
Bus 750
Bus 751
        )");

        ostringstream oss;

        Parse(iss, oss);

        istringstream iss_expect(R"(Bus 256: 6 stops on route, 5 unique stops, 0 route length, 0 curvature
Bus 750: 5 stops on route, 3 unique stops, 0 route length, 0 curvature
Bus 751: not found
)");

        string str = oss.str();
        string str_expect = iss_expect.str();

        ASSERT_EQUAL(str, str_expect);
    }
    {
        istringstream iss(R"(
12
Stop Tolstopaltsevo: 55.611087, 37.20829
Stop Marushkino: 55.595884, 37.209755
Bus Malina Kalina: Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Tovarnaya > Biryulyovo Passazhirskaya > Biryulyovo Zapadnoye
Bus 750: Tolstopaltsevo - Marushkino - Rasskazovka
Stop Rasskazovka: 55.632761, 37.333324
Stop Biryulyovo Zapadnoye: 55.574371, 37.6517
Stop Biryusinka: 55.581065, 37.64839
Stop Universam: 55.587655, 37.645687
Stop Biryulyovo Tovarnaya: 55.592028, 37.653656
Stop Biryulyovo Passazhirskaya: 55.580999, 37.659164
Bus 257: Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Tovarnaya > Universam > Biryusinka > Biryulyovo Zapadnoye
Bus 258: Biryulyovo Zapadnoye - Biryusinka - Universam - Biryulyovo Tovarnaya
5
Bus Malina Kalina
Bus 750
Bus 751
Bus 257
Bus 258
        )");

        ostringstream oss;

        Parse(iss, oss);

        istringstream iss_expect(R"(Bus Malina Kalina: 6 stops on route, 5 unique stops, 0 route length, 0 curvature
Bus 750: 5 stops on route, 3 unique stops, 0 route length, 0 curvature
Bus 751: not found
Bus 257: 7 stops on route, 4 unique stops, 0 route length, 0 curvature
Bus 258: 7 stops on route, 4 unique stops, 0 route length, 0 curvature
)");

        string str = oss.str();
        string str_expect = iss_expect.str();

        ASSERT_EQUAL(str, str_expect);
    }
    {
        istringstream iss(R"(
10
Stop Tolstopaltsevo: 55.611087, 37.20829
Stop Marushkino: 56.60, 50.35
Stop Parushka: 55.611087, 37.20829
Stop Sasushka: 56.60, 50.35
Bus 256: Tolstopaltsevo > Marushkino > Tolstopaltsevo
Bus 750: Tolstopaltsevo - Marushkino
Bus 755: Parushka - Sasushka
Bus 756: Parushka - Sasushka
Bus 757: Parushka - Sasushka - Tolstopaltsevo - Marushkino
Bus 758: Parushka - Sasushka - Marushkino - Tolstopaltsevo
8
Bus 256
Bus 750
Bus 256
Bus 751
Bus 755
Bus 756
Bus 757
Bus 758
        )");

        ostringstream oss;

        Parse(iss, oss);

        istringstream iss_expect(R"(Bus 256: 3 stops on route, 2 unique stops, 0 route length, 0 curvature
Bus 750: 3 stops on route, 2 unique stops, 0 route length, 0 curvature
Bus 256: 3 stops on route, 2 unique stops, 0 route length, 0 curvature
Bus 751: not found
Bus 755: 3 stops on route, 2 unique stops, 0 route length, 0 curvature
Bus 756: 3 stops on route, 2 unique stops, 0 route length, 0 curvature
Bus 757: 7 stops on route, 4 unique stops, 0 route length, 0 curvature
Bus 758: 7 stops on route, 4 unique stops, 0 route length, 0 curvature
)");

        string str = oss.str();
        string str_expect = iss_expect.str();

        ASSERT_EQUAL(str, str_expect);
    }
    {
        istringstream iss(R"(
11
Stop X1: 56.60, 50.31
Stop X2: 56.60, 50.32
Stop X3: 56.60, 50.33
Stop X4: 56.60, 50.34
Stop X5: 56.60, 50.35
Stop X6: 56.60, 50.36
Stop X7: 56.60, 50.37
Stop X8: 56.60, 50.38
Stop X9: 56.60, 50.39
Stop X10 + X1 + X5 + X9: 56.60, 50.40
Bus X: X1 - X2 - X3 - X4 - X5 - X6 - X7 - X8 - X9 - X10 + X1 + X5 + X9
2
Bus X
Bus X
        )");

        ostringstream oss;

        Parse(iss, oss);

        istringstream iss_expect(R"(Bus X: 19 stops on route, 10 unique stops, 0 route length, 0 curvature
Bus X: 19 stops on route, 10 unique stops, 0 route length, 0 curvature
)");

        string str = oss.str();
        string str_expect = iss_expect.str();

        ASSERT_EQUAL(str, str_expect);
    }
    {
        istringstream iss(R"(
11
Stop X1: 56.60, 50.31
Stop X2: 56.60, 50.32
Stop X3: 56.60, 50.33
Stop X4: 56.60, 50.34
Stop X5: 56.60, 50.35
Stop X6: 56.60, 50.36
Stop X7: 56.60, 50.37
Stop X8: 56.60, 50.38
Stop X9: 56.60, 50.39
Stop X10 + X1 + X5 + X9: 56.60, 50.40
Bus X: X1 - X2 - X3 - X4 - X5 - X6 - X7 - X8 - X9 - X10 + X1 + X5 + X9
2
Bus X
Bus X
        )");

        ostringstream oss;

        Parse(iss, oss);

        istringstream iss_expect(R"(Bus X: 19 stops on route, 10 unique stops, 0 route length, 0 curvature
Bus X: 19 stops on route, 10 unique stops, 0 route length, 0 curvature
)");

        string str = oss.str();
        string str_expect = iss_expect.str();

        ASSERT_EQUAL(str, str_expect);
    }
    {
        istringstream iss(R"(
13
Stop Tolstopaltsevo: 55.611087, 37.20829
Stop Marushkino: 55.595884, 37.209755
Bus 256: Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Tovarnaya > Biryulyovo Passazhirskaya > Biryulyovo Zapadnoye
Bus 750: Tolstopaltsevo - Marushkino - Rasskazovka
Stop Rasskazovka: 55.632761, 37.333324
Stop Biryulyovo Zapadnoye: 55.574371, 37.6517
Stop Biryusinka: 55.581065, 37.64839
Stop Universam: 55.587655, 37.645687
Stop Biryulyovo Tovarnaya: 55.592028, 37.653656
Stop Biryulyovo Passazhirskaya: 55.580999, 37.659164
Bus 828: Biryulyovo Zapadnoye > Universam > Rossoshanskaya ulitsa > Biryulyovo Zapadnoye
Stop Rossoshanskaya ulitsa: 55.595579, 37.605757
Stop Prazhskaya: 55.611678, 37.603831
6
Bus 256
Bus 750
Bus 751
Stop Samara
Stop Prazhskaya
Stop Biryulyovo Zapadnoye
        )");

        ostringstream oss;

        Parse(iss, oss);

        istringstream iss_expect(R"(Bus 256: 6 stops on route, 5 unique stops, 0 route length, 0 curvature
Bus 750: 5 stops on route, 3 unique stops, 0 route length, 0 curvature
Bus 751: not found
Stop Samara: not found
Stop Prazhskaya: no buses
Stop Biryulyovo Zapadnoye: buses 256 828
)");

        string str = oss.str();
        string str_expect = iss_expect.str();

        ASSERT_EQUAL(str, str_expect);
    }
    {
        istringstream iss(R"(
13
Stop Tolstopaltsevo: 55.611087, 37.20829, 3900m to Marushkino
Stop Marushkino: 55.595884, 37.209755, 9900m to Rasskazovka
Bus 256: Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Tovarnaya > Biryulyovo Passazhirskaya > Biryulyovo Zapadnoye
Bus 750: Tolstopaltsevo - Marushkino - Rasskazovka
Stop Rasskazovka: 55.632761, 37.333324
Stop Biryulyovo Zapadnoye: 55.574371, 37.6517, 7500m to Rossoshanskaya ulitsa, 1800m to Biryusinka, 2400m to Universam
Stop Biryusinka: 55.581065, 37.64839, 750m to Universam
Stop Universam: 55.587655, 37.645687, 5600m to Rossoshanskaya ulitsa, 900m to Biryulyovo Tovarnaya
Stop Biryulyovo Tovarnaya: 55.592028, 37.653656, 1300m to Biryulyovo Passazhirskaya
Stop Biryulyovo Passazhirskaya: 55.580999, 37.659164, 1200m to Biryulyovo Zapadnoye
Bus 828: Biryulyovo Zapadnoye > Universam > Rossoshanskaya ulitsa > Biryulyovo Zapadnoye
Stop Rossoshanskaya ulitsa: 55.595579, 37.605757
Stop Prazhskaya: 55.611678, 37.603831
6
Bus 256
Bus 750
Bus 751
Stop Samara
Stop Prazhskaya
Stop Biryulyovo Zapadnoye
        )");

        ostringstream oss;

        Parse(iss, oss);

        istringstream iss_expect(R"(Bus 256: 6 stops on route, 5 unique stops, 5950 route length, 1.36124 curvature
Bus 750: 5 stops on route, 3 unique stops, 27600 route length, 1.31808 curvature
Bus 751: not found
Stop Samara: not found
Stop Prazhskaya: no buses
Stop Biryulyovo Zapadnoye: buses 256 828
)");

        string str = oss.str();
        string str_expect = iss_expect.str();

        ASSERT_EQUAL(str, str_expect);
    }
}

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, TestParseAddStopQuery);
    RUN_TEST(tr, TestParseAddBusQuery);
    RUN_TEST(tr, TestCalcGeoDistance);
    RUN_TEST(tr, TestDataBaseCreateBusesInfo);
    RUN_TEST(tr, TestParse);
}

void Profile()
{
}

int main()
{
    TestAll();

    Parse(cin, cout);
    return 0;
}