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

using namespace std;

void TestAll();
void Profile();

struct Stop
{
    string name;
    double latitude = 0.0;
    double longitude = 0.0;

    bool operator==(const Stop &o) const
    { return tie(name, latitude, longitude) == tie(o.name, o.latitude, o.longitude); }
};

using StopPtr = shared_ptr<Stop>;

struct StopPtrHasher
{
    size_t operator()(const StopPtr &value) const
    {
        return str_hasher(value->name);
    }

    hash<string> str_hasher;
};


struct StopPtrKeyEqual
{
    bool operator()(const StopPtr& lhs, const StopPtr& rhs) const
    {
        return lhs->name == rhs->name;
    }
};

using Stops = unordered_set<StopPtr, StopPtrHasher, StopPtrKeyEqual>;

struct Bus
{
    string name;
    vector<StopPtr> stops;
    bool ring = false;
};

struct DataBase
{
    Stops stops;
    vector<Bus> buses;
};

class TransportGuide
{
public:

private:
    DataBase _base;
};

// Stop X: latitude, longitude
Stop ParseStopQuery(istream &is)
{
    Stop result;

    string s;
    is >> s; // skip "Stop"
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

Bus ParseBusQuery(istream &is, Stops &stops)
{
    Bus result;

    string s;
    is >> s; // skip "Stop"
    is.ignore(1); // skip " "
    getline(is, s, ':');
    result.name = move(s);
    is.ignore(1); // skip " "

    string stop_name;
    char separator;

    while (is and (is.peek() != '-' and is.peek() != '>'))
    {
        is >> s;
        is.ignore(1);
        stop_name += move(s) + ' ';
    }
    stop_name.pop_back();
    if (is)
    {
        separator = is.peek();
        result.ring = separator == '>';
    }

    is.ignore(2); // skip separator and space

    auto push_stop = [&stops, &result](string && name)
    {
        StopPtr stop = make_shared<Stop>(move(Stop{move(name)}));
        auto it = stops.find(stop);
        if (it == stops.end())
        {
            stops.insert(stop);
            result.stops.push_back(move(stop));
        }
        else
        {
            result.stops.push_back(*it);
        }
    };

    push_stop(move(stop_name));

    while (is)
    {
        getline(is, stop_name, separator);
        is.ignore(1); // skip space
        if (is)
            stop_name.pop_back();

        // последнюю остановку не надо добавлять
        // для кольцевого маршрута
        if (result.ring and not is)
            break;

        push_stop(move(stop_name));
    }
    return result;
}

void Parse(istream &is = cin, ostream &os = cout)
{

}

int main()
{
    TestAll();

    return 0;
}

void TestParseStopQuery()
{
    {
        istringstream is("Stop Empire Street Building 5: 55.611087, 2.34");
        Stop stop = ParseStopQuery(is);
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
        istringstream is("Stop E: 21, 89.5");
        Stop stop = ParseStopQuery(is);
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

void TestParseBusQuery()
{
    {
        StopPtr stop1 = make_shared<Stop>(Stop{"Biryulyovo Zapadnoye"});
        StopPtr stop2 = make_shared<Stop>(Stop{"Biryusinka"});
        StopPtr stop3 = make_shared<Stop>(Stop{"Universam"});
        StopPtr stop4 = make_shared<Stop>(Stop{"Biryulyovo Tovarnaya"});
        Stops stops{stop1, stop2, stop3, stop4};
        istringstream is("Bus 256: Biryulyovo Zapadnoye - Biryusinka - Universam - Biryulyovo Tovarnaya");
        Bus bus = ParseBusQuery(is, stops);
        Bus expect{
            .name = "256",
            .stops{stop1, stop2, stop3, stop4},
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
        istringstream is("Bus 256: Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Tovarnaya > Biryulyovo Passazhirskaya > Biryulyovo Zapadnoye");
        Bus bus = ParseBusQuery(is, stops);
        Bus expect{
            .name = "256",
            .stops{stop1, stop2, stop3, stop4, stop5},
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
        istringstream is("Bus My Bus: Biryulyovo Zapadnoye");
        Bus bus = ParseBusQuery(is, stops);
        Bus expect{
            .name = "My Bus",
            .stops{stop},
            .ring = false
        };
        ASSERT_EQUAL(bus.name, expect.name);
        ASSERT(bus.stops[0].get() == expect.stops[0].get());
        ASSERT_EQUAL(bus.ring, expect.ring);
    }
    {
        StopPtr stop = make_shared<Stop>(Stop{"Biryulyovo"});
        Stops stops{stop};
        istringstream is("Bus My Bus: Biryulyovo");
        Bus bus = ParseBusQuery(is, stops);
        Bus expect{
            .name = "My Bus",
            .stops{stop},
            .ring = false
        };
        ASSERT_EQUAL(bus.name, expect.name);
        ASSERT(bus.stops[0].get() == expect.stops[0].get());
        ASSERT_EQUAL(bus.ring, expect.ring);
    }
}

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, TestParseStopQuery);
    RUN_TEST(tr, TestParseBusQuery);
}

void Profile()
{
}