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

    if (left >= 0 and right >= 0)
    {
        uint64_t result = static_cast<uint64_t>(left) + static_cast<uint64_t>(right);
        if (result > std::numeric_limits<int64_t>::max())
            cout << "Overflow!";
        else
            cout << left + right << endl;
    }
    else if (left <= 0 and right <= 0)
    {
        uint64_t u_left = left == std::numeric_limits<int64_t>::min() ? static_cast<uint64_t>(left)
                                                                      : left * (-1);
        uint64_t u_right = right == std::numeric_limits<int64_t>::min() ? static_cast<uint64_t>(right)
                                                                        : right * (-1);
        uint64_t result = u_left + u_right;
        
        if (result > static_cast<uint64_t>( std::numeric_limits<int64_t>::min() ))
            cout << "Overflow!";
        else
            cout << left + right << endl;
    }
    else
    {
        cout << left + right << endl;
    }

    return 0;
}