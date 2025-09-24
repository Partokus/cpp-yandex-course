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
#include <istream>
#include <variant>
#include <tuple>
#include <bitset>

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

auto GetTuple()
{
    int x = 8, y = 21;
    return tie(x, y);
}

int main()
{
    TestAll();

    auto [x, y] = GetTuple();
    cout << x << ' ' << y << endl;

    return 0;
}