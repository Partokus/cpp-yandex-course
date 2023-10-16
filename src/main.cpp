#include <test_runner.h>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <optional>
#include <tuple>
#include <ctime>
#include <cstring>
#include <iomanip>

using namespace std;

template <typename RandomIt>
pair<RandomIt, RandomIt> FindStartsWith(RandomIt range_begin, RandomIt range_end, const string &prefix)
{
    // Все строки, начинающиеся с prefix, больше или равны строке "<prefix>"
    auto left = lower_bound(range_begin, range_end, prefix);

    // Составим строку, которая в рамках буквенных строк является
    // точной верхней гранью множества строк, начинающихся с prefix
    string upper_bound = prefix;
    ++upper_bound[upper_bound.size() - 1];

    // Первое встреченное слово, не меньшее upper_bound,
    // обязательно является концом полуинтервала
    auto right = lower_bound(range_begin, range_end, upper_bound);

    return {left, right};
}

void TestFindStartsWith()
{
}

void TestAll()
{
    TestRunner tr = {};

    tr.RunTest(TestFindStartsWith, "TestFindStartsWith");
}

int main()
{
    TestAll();

    const vector<string> sorted_strings = {"moscow", "motovilikha", "murmansk", "m", "p"};

    const auto mo_result =
        FindStartsWith(begin(sorted_strings), end(sorted_strings), "mo");
    for (auto it = mo_result.first; it != mo_result.second; ++it)
    {
        cout << *it << " ";
    }
    cout << endl;

    const auto mt_result =
        FindStartsWith(begin(sorted_strings), end(sorted_strings), "mt");
    cout << (mt_result.first - begin(sorted_strings)) << " " << (mt_result.second - begin(sorted_strings)) << endl;

    const auto na_result =
        FindStartsWith(begin(sorted_strings), end(sorted_strings), "na");
    cout << (na_result.first - begin(sorted_strings)) << " " << (na_result.second - begin(sorted_strings)) << endl;

    return 0;
}
