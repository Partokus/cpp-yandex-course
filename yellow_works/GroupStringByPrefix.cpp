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

struct Comp
{
    bool operator()(const string &s, char c) const { return s[0] < c; }
    bool operator()(char c, const string &s) const { return c < s[0]; }
};

template <typename RandomIt>
pair<RandomIt, RandomIt> FindStartsWith(RandomIt range_begin, RandomIt range_end, char prefix)
{
    return equal_range(range_begin, range_end, prefix, Comp{});
}

void TestFindStartsWith()
{
    {
        const vector<string> sorted_strings = {"moscow", "murmansk", "vologda"};
        Assert(FindStartsWith(sorted_strings.begin(), sorted_strings.end(), 'm') == pair{sorted_strings.begin() + 0, sorted_strings.begin() + 2}, "FindStartsWith 0");
        Assert(FindStartsWith(sorted_strings.begin(), sorted_strings.end(), 'p') == pair{sorted_strings.begin() + 2, sorted_strings.begin() + 2}, "FindStartsWith 1");
        Assert(FindStartsWith(sorted_strings.begin(), sorted_strings.end(), 'z') == pair{sorted_strings.end(), sorted_strings.end()}, "FindStartsWith 2");
    }
    {
        const vector<string> sorted_strings{};
        Assert(FindStartsWith(sorted_strings.begin(), sorted_strings.end(), 'm') == pair{sorted_strings.begin(), sorted_strings.begin()}, "FindStartsWith 3");
    }
    {
        const vector<string> sorted_strings{"moscow", "murmansk", "perm", "saransk", "savana", "vologda"};
        Assert(FindStartsWith(sorted_strings.begin(), sorted_strings.end(), 'p') == pair{sorted_strings.begin() + 2, sorted_strings.begin() + 3}, "FindStartsWith 4");
        Assert(FindStartsWith(sorted_strings.begin(), sorted_strings.end(), 's') == pair{sorted_strings.begin() + 3, sorted_strings.begin() + 5}, "FindStartsWith 5");
        Assert(FindStartsWith(sorted_strings.begin(), sorted_strings.end(), 'v') == pair{sorted_strings.begin() + 5, sorted_strings.begin() + 6}, "FindStartsWith 6");
        Assert(FindStartsWith(sorted_strings.begin(), sorted_strings.end(), 'r') == pair{sorted_strings.begin() + 3, sorted_strings.begin() + 3}, "FindStartsWith 7");
        Assert(FindStartsWith(sorted_strings.begin(), sorted_strings.end(), 't') == pair{sorted_strings.begin() + 5, sorted_strings.begin() + 5}, "FindStartsWith 8");
        Assert(FindStartsWith(sorted_strings.begin(), sorted_strings.end(), 'k') == pair{sorted_strings.begin() + 0, sorted_strings.begin() + 0}, "FindStartsWith 9");
    }
}

void TestAll()
{
    TestRunner tr = {};

    tr.RunTest(TestFindStartsWith, "TestFindStartsWith");
}

int main()
{
    TestAll();

    return 0;
}
