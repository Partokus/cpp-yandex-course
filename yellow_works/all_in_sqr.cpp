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

template <typename T>
T Sqr(T obj);

template <typename T>
vector<T> operator*(const vector<T> &lhs, const vector<T> &rhs);

template <typename First, typename Second>
pair<First, Second> operator*(const pair<First, Second> &lhs, const pair<First, Second> &rhs);

template <typename Key, typename Value>
map<Key, Value> operator*(const map<Key, Value> &lhs, const map<Key, Value> &rhs);

template <typename T>
T Sqr(T obj)
{
    return obj * obj;
}

template <typename T>
vector<T> operator*(const vector<T> &lhs, const vector<T> &rhs)
{
    vector<T> result(lhs.size());
    transform(lhs.cbegin(), lhs.cend(), result.begin(), [](const T &i)
              { return i * i; });
    return result;
}

template <typename First, typename Second>
pair<First, Second> operator*(const pair<First, Second> &lhs, const pair<First, Second> &rhs)
{
    return {lhs.first * lhs.first, lhs.second * lhs.second};
}

template <typename Key, typename Value>
map<Key, Value> operator*(const map<Key, Value> &lhs, const map<Key, Value> &rhs)
{
    map<Key, Value> result = lhs;
    for (auto &[key, value] : result)
    {
        value = value * value;
    }
    return result;
}

int main()
{
    vector<int> v = {1, 2, 3};
    cout << "vector:";
    for (int x : Sqr(v))
    {
        cout << ' ' << x;
    }
    cout << endl;

    map<int, pair<int, int>> map_of_pairs = {
        {4, {2, 2}},
        {7, {4, 3}}};
    cout << "map of pairs:" << endl;
    for (const auto &x : Sqr(map_of_pairs))
    {
        cout << x.first << ' ' << x.second.first << ' ' << x.second.second << endl;
    }
    return 0;
}
