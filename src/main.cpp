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

struct Stop
{
    string name;
    double latitude = 0.0;
    double longitude = 0.0;

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
using BusPtr = shared_ptr<Bus>;

using Stops = unordered_set<StopPtr, NamePtrHasher<StopPtr>, NamePtrKeyEqual<StopPtr>>;
using Buses = unordered_set<BusPtr, NamePtrHasher<BusPtr>, NamePtrKeyEqual<BusPtr>>;

double ToRadians(double deg)
{
    static constexpr double Pi = 3.1415926535;
    return deg * Pi / 180.0;
}

// Возвращает метры
// Haversine formula implementation
double CalcDistance(double lat1, double lon1, double lat2, double lon2)
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
        double route_length = 0.0;
    };

    unordered_map<BusPtr, BusInfo, NamePtrHasher<BusPtr>, NamePtrKeyEqual<BusPtr>> buses_info;

    void CreateBusesInfo()
    {
        const Stops not_unique_stops = [this]() {
            Stops unique_stops;
            Stops result;

            for (const BusPtr &bus : buses)
            {
                for (const StopPtr &stop : bus->stops)
                {
                    auto [it, inserted] = unique_stops.insert(stop);
                    if (not inserted)
                        result.insert(stop);
                }
            }
            return result;
        }();

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

            for (auto it = bus->stops.begin(); it != bus->stops.end(); ++it)
            {
                if (not_unique_stops.find(*it) == not_unique_stops.end())
                {
                    ++info.unique_stops;
                }

                auto it_next = next(it);
                if (it_next != bus->stops.end())
                {
                    info.route_length += CalcDistance(
                        it->get()->latitude,      it->get()->longitude,
                        it_next->get()->latitude, it_next->get()->longitude
                    );
                }
            }

            if (bus->ring)
            {
                info.route_length += CalcDistance(
                    bus->stops.back()->latitude,   bus->stops.back()->longitude,
                    bus->stops.front()->latitude, bus->stops.front()->longitude
                );
            }
            else
                info.route_length *= 2.0;
        }


    }
};

class TransportGuide
{
public:
    
private:
    DataBase _data_base;
};

// X: latitude, longitude
Stop ParseAddStopQuery(istream &is)
{
    Stop result;

    string s;
    if (is.peek() == ' ')
        is.ignore(1); // skip " "
    getline(is, s, ':');
    result.name = move(s);
    is.ignore(1); // skip " "

    getline(is, s, ',');
    result.latitude = stod(s);
    is.ignore(1); // skip " "
    is >> result.longitude;

    return result;
}

// Bus X: описание маршрута
Bus ParseAddBusQuery(istream &is, Stops &stops)
{
    Bus result;

    string s;
    getline(is, s);
    istringstream iss(move(s));

    if (iss.peek() == ' ')
        iss.ignore(1); // skip " "
    getline(iss, s, ':');
    result.name = move(s);
    iss.ignore(1); // skip " "

    string stop_name;
    char separator;

    while (iss and (iss.peek() != '-' and iss.peek() != '>'))
    {
        iss >> s;
        iss.ignore(1);
        stop_name += move(s) + ' ';
    }
    stop_name.pop_back();
    if (iss)
    {
        separator = iss.peek();
        result.ring = separator == '>';
    }

    iss.ignore(2); // skip separator and space

    auto push_stop = [&stops, &result](string && name)
    {
        StopPtr stop = make_shared<Stop>(move(Stop{move(name)}));
        auto it = stops.find(stop);
        if (it != stops.end())
        {
            result.stops.push_back(*it);
        }
        else
        {
            stops.insert(stop);
            result.stops.push_back(move(stop));
        }
    };

    push_stop(move(stop_name));

    while (iss)
    {
        getline(iss, stop_name, separator);
        iss.ignore(1); // skip space
        if (iss)
            stop_name.pop_back();

        // последнюю остановку не надо добавлять
        // для кольцевого маршрута
        if (result.ring and not iss)
            break;

        push_stop(move(stop_name));
    }
    return result;
}

void ParseGetBusInfo(istream &is)
{

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
            Stop stop = ParseAddStopQuery(is);
            auto [it, inserted] = db.stops.insert(make_shared<Stop>( move(stop) ));
            if (not inserted)
            {
                it->get()->latitude = stop.latitude;
                it->get()->longitude = stop.longitude;
            }
        }
        else if (first_word == "Bus")
        {
            Bus bus = ParseAddBusQuery(is, db.stops);
            db.buses.insert(make_shared<Bus>( move(bus) ));
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
                os << info.stops_on_route << " stops on route, "
                << info.unique_stops   << " unique stops, "
                << info.route_length   << " route length";
            }
            else
                os << "not found";

            os << '\n';
        }
    }
}

void TestParseAddStopQuery()
{
    {
        istringstream is("Empire Street Building 5: 55.611087, 2.34");
        Stop stop = ParseAddStopQuery(is);
        Stop expect{
            .name = "Empire Street Building 5",
            .latitude = 55.611087,
            .longitude = 2.34
        };
        ASSERT_EQUAL(stop.name, expect.name);
        ASSERT_EQUAL(stop.latitude, expect.latitude);
        ASSERT_EQUAL(stop.longitude, expect.longitude);
    }
    {
        istringstream is("E: 21, 89.5");
        Stop stop = ParseAddStopQuery(is);
        Stop expect{
            .name = "E",
            .latitude = 21.0,
            .longitude = 89.5
        };
        ASSERT_EQUAL(stop.name, expect.name);
        ASSERT_EQUAL(stop.latitude, expect.latitude);
        ASSERT_EQUAL(stop.longitude, expect.longitude);
    }
}

void TestParseAddBusQuery()
{
    {
        StopPtr stop1 = make_shared<Stop>(Stop{"Biryulyovo Zapadnoye"});
        StopPtr stop2 = make_shared<Stop>(Stop{"Biryusinka"});
        StopPtr stop3 = make_shared<Stop>(Stop{"Universam"});
        StopPtr stop4 = make_shared<Stop>(Stop{"Biryulyovo Tovarnaya"});
        Stops stops{stop1, stop2, stop3, stop4};
        istringstream is("256: Biryulyovo Zapadnoye - Biryusinka - Universam - Biryulyovo Tovarnaya");
        Bus bus = ParseAddBusQuery(is, stops);
        Bus expect{
            .name = "256",
            .stops = {stop1, stop2, stop3, stop4},
            .ring = false
        };
        ASSERT_EQUAL(bus.name, expect.name);
        for (int i = 0; i < 4; ++i)
        {
            ASSERT(bus.stops[i].get() == expect.stops[i].get());
            ASSERT(*bus.stops[i] == *expect.stops[i]);
        }
        ASSERT_EQUAL(bus.ring, expect.ring);
    }
    {
        StopPtr stop1 = make_shared<Stop>(Stop{"Biryulyovo Zapadnoye"});
        StopPtr stop2 = make_shared<Stop>(Stop{"Biryusinka"});
        StopPtr stop3 = make_shared<Stop>(Stop{"Universam"});
        StopPtr stop4 = make_shared<Stop>(Stop{"Biryulyovo Tovarnaya"});
        StopPtr stop5 = make_shared<Stop>(Stop{"Biryulyovo Passazhirskaya"});
        Stops stops{stop1, stop2, stop3, stop4, stop5};
        istringstream is("256: Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Tovarnaya > Biryulyovo Passazhirskaya > Biryulyovo Zapadnoye");
        Bus bus = ParseAddBusQuery(is, stops);
        Bus expect{
            .name = "256",
            .stops = {stop1, stop2, stop3, stop4, stop5},
            .ring = true
        };
        ASSERT_EQUAL(bus.stops.size(), expect.stops.size());
        ASSERT_EQUAL(bus.name, expect.name);
        for (int i = 0; i < 5; ++i)
            ASSERT(bus.stops[i].get() == expect.stops[i].get());
        ASSERT_EQUAL(bus.ring, expect.ring);
    }
    {
        StopPtr stop = make_shared<Stop>(Stop{"Biryulyovo Zapadnoye"});
        Stops stops{stop};
        istringstream is("My Bus: Biryulyovo Zapadnoye");
        Bus bus = ParseAddBusQuery(is, stops);
        Bus expect{
            .name = "My Bus",
            .stops = {stop},
            .ring = false
        };
        ASSERT_EQUAL(bus.name, expect.name);
        ASSERT(bus.stops[0].get() == expect.stops[0].get());
        ASSERT_EQUAL(bus.ring, expect.ring);
    }
    {
        StopPtr stop = make_shared<Stop>(Stop{"Biryulyovo"});
        Stops stops{stop};
        istringstream is("My Bus: Biryulyovo");
        Bus bus = ParseAddBusQuery(is, stops);
        Bus expect{
            .name = "My Bus",
            .stops = {stop},
            .ring = false
        };
        ASSERT_EQUAL(bus.name, expect.name);
        ASSERT(bus.stops[0].get() == expect.stops[0].get());
        ASSERT_EQUAL(bus.ring, expect.ring);
    }
}

void TestCalcDistance()
{
    {
        double lat1 = 55.611087;
        double lon1 = 37.20829;
        double lat2 = 55.595884;
        double lon2 = 37.209755;

        double length = CalcDistance(lat1, lon1, lat2, lon2);
        ASSERT(length > 1692.0 and length < 1694.0);
        length = CalcDistance(lat2, lon2, lat1, lon1);
        ASSERT(length > 1692.0 and length < 1694.0);
        length = CalcDistance(lat1, lat2, lat1, lat2);
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

        ASSERT_EQUAL(db.buses_info[bus1].unique_stops, 2U);
        ASSERT_EQUAL(db.buses_info[bus2].unique_stops, 0U);
        ASSERT_EQUAL(db.buses_info[bus3].unique_stops, 1U);
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

        ASSERT(db.buses_info[bus1].route_length > 4370.0 and db.buses_info[bus1].route_length < 4372.0);
        ASSERT(db.buses_info[bus2].route_length > 20938.0 and db.buses_info[bus2].route_length < 20940.0);
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

        istringstream iss_expect(R"(Bus 256: 6 stops on route, 5 unique stops, 4371.02 route length
Bus 750: 5 stops on route, 3 unique stops, 20939.5 route length
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

        istringstream iss_expect(R"(Bus Malina Kalina: 6 stops on route, 1 unique stops, 4371.02 route length
Bus 750: 5 stops on route, 3 unique stops, 20939.5 route length
Bus 751: not found
Bus 257: 7 stops on route, 0 unique stops, 4446.15 route length
Bus 258: 7 stops on route, 0 unique stops, 4446.15 route length
)");

        string str = oss.str();
        string str_expect = iss_expect.str();

        ASSERT_EQUAL(str, str_expect);
    }
    {
        istringstream iss(R"(
4
Stop Tolstopaltsevo: 55.611087, 37.20829
Stop Marushkino: 56.60, 50.35
Bus 256: Tolstopaltsevo > Marushkino > Tolstopaltsevo
Bus 750: Tolstopaltsevo - Marushkino
4
Bus 256
Bus 750
Bus 256
Bus 751
        )");

        ostringstream oss;

        Parse(iss, oss);

        istringstream iss_expect(R"(Bus 256: 3 stops on route, 0 unique stops, 1.642e+06 route length
Bus 750: 3 stops on route, 0 unique stops, 1.642e+06 route length
Bus 256: 3 stops on route, 0 unique stops, 1.642e+06 route length
Bus 751: not found
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
    RUN_TEST(tr, TestCalcDistance);
    RUN_TEST(tr, TestDataBaseCreateBusesInfo);
    RUN_TEST(tr, TestParse);
}

void Profile()
{
}

int main()
{
    // TestAll();

    Parse(cin, cout);
    return 0;
}