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

template <typename Key, typename Value>
Value &GetRefStrict(map<Key, Value> &m, Key k)
{
    if (not m.count(k))
    {
        throw runtime_error("key didn't find");
    }
    return m[k];
}

int main()
{
    map<int, string> m = {{0, "value"}};
    string &item = GetRefStrict(m, 0);
    item = "newvalue";
    cout << m[0] << endl; // выведет newvalue
    return 0;
}
