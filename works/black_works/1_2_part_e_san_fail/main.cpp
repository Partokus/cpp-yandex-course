#include <profile.h>
#include <test_runner.h>

#include "descriptions.h"
#include "json.h"
#include "requests.h"
#include "sphere.h"
#include "transport_catalog.h"
#include "utils.h"

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
#include <cassert>

using namespace std;
using namespace std::chrono;
using namespace std::chrono_literals;

void TestAll();
void Profile();

void TestAll()
{
    TestRunner tr{};
}

void Profile()
{
}

int main()
{
    TestAll();

    const auto input_doc = Json::Load(cin);
    const auto &input_map = input_doc.GetRoot().AsMap();

    const TransportCatalog db(
        Descriptions::ReadDescriptions(input_map.at("base_requests").AsArray()),
        input_map.at("routing_settings").AsMap());

    Json::PrintValue(
        Requests::ProcessAll(db, input_map.at("stat_requests").AsArray()),
        cout);

    cout << endl;

    return 0;
}