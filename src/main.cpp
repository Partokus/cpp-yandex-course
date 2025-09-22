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

int main()
{
    TestAll();

    vector<size_t> v{8};
    v.reserve(2);
    size_t i = 1U;
    cout << v[i] << endl;
    return 0;
}