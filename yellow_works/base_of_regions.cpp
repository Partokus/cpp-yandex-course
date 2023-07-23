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

/*
struct Region
{
    string std_name;
    string parent_std_name;
    map<Lang, string> names;
    int64_t population;
};

enum class Lang
{
    DE,
    FR,
    IT
};
*/

bool operator<(const Region &lhs, const Region &rhs)
{
    return tie(lhs.std_name, lhs.parent_std_name, lhs.names, lhs.population) < tie(rhs.std_name, rhs.parent_std_name, rhs.names, rhs.population);
}

int FindMaxRepetitionCount(const vector<Region> &regions)
{
    int result = 0;
    map<Region, int> repetition_count;
    for (const auto &region : regions)
    {
        result = max(result, ++repetition_count[region]);
    }
    return result;
}

int main()
{
    return 0;
}
