#include <profile.h>
#include <test_runner.h>

#include "stats_aggregator.h"

#include <cassert>
#include <algorithm>
#include <iterator>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <string>
#include <forward_list>
#include <iterator>
#include <deque>
#include <tuple>
#include <random>
#include <functional>
#include <fstream>
#include <random>
#include <tuple>

using namespace std;

void TestAll();
void Profile();

unique_ptr<StatsAggregator> ReadAggregators(istream &input)
{
    using namespace StatsAggregators;

    const unordered_map<string, std::function<unique_ptr<StatsAggregator>()>> known_builders = {
        {"sum", []
         { return make_unique<Sum>(); }},
        {"min", []
         { return make_unique<Min>(); }},
        {"max", []
         { return make_unique<Max>(); }},
        {"avg", []
         { return make_unique<Average>(); }},
        {"mode", []
         { return make_unique<Mode>(); }}};

    auto result = make_unique<Composite>();

    int aggr_count;
    input >> aggr_count;

    string line;
    for (int i = 0; i < aggr_count; ++i)
    {
        input >> line;
        result->Add(known_builders.at(line)());
    }

    return result;
}

int main()
{
    TestAll();
    Profile();

    auto stats_aggregator = ReadAggregators(cin);

    for (int value; cin >> value;)
    {
        stats_aggregator->Process(value);
    }
    stats_aggregator->PrintValue(cout);
    return 0;
}

void TestAll()
{
    TestRunner tr{};
    using namespace StatsAggregators;
    RUN_TEST(tr, TestSum);
    RUN_TEST(tr, TestMin);
    RUN_TEST(tr, TestMax);
    RUN_TEST(tr, TestAverage);
    RUN_TEST(tr, TestMode);
    RUN_TEST(tr, TestComposite);
}

void Profile()
{
}