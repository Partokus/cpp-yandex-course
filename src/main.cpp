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
#include <cassert>
#include <limits>

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

    int64_t left = 0;
    int64_t right = 0;

    cin >> left >> right;

    uint64_t resource = std::numeric_limits<int64_t>::max();
    if (left >= right)
    {
        resoure = std::numeric_limits<int64_t>::max() - left;
        if (right > resource)
        {
            cout << "Overflow!";
        }
        else
        {
            cout << left + right;
        }
    }
    else
    {
        resoure = std::abs(left - std::numeric_limits<int64_t>::min());
        if (right) {
            
        }
    }
    return 0;
}