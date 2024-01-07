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

template <typename Collection>
ostream &join(ostream &out, const Collection &c, const string &d)
{
    bool first = true;

    for (const auto &i : c)
    {
        if (not first)
        {
            out << d;
        }
        first = false;

        out << i;
    }
    return out;
}

template <typename T>
ostream &operator<<(ostream &out, const vector<T> &v)
{
    out << "[";
    join(out, v, ", ");
    return out << "]";
}

template <typename T>
ostream &operator<<(ostream &out, const set<T> &s)
{
    out << "[";
    join(out, s, ", ");
    return out << "]";
}

template <typename First, typename Second>
ostream &operator<<(ostream &out, const pair<First, Second> &p)
{
    return out << "(" << p.first << ", " << p.second << ")";
}

template <typename Key, typename Value>
ostream &operator<<(ostream &out, const map<Key, Value> &m)
{
    out << "{";
    join(out, m, ", ");
    return out << "}";
}

int main()
{
    vector<double> v = {1.4, 2, 3};
    set<double> s = {2.5, 1.4, 3};
    map<int, int> m = {{1, 2}, {3, 4}};

    cout << v << endl;
    cout << s << endl;
    cout << m << endl;
    return 0;
}
