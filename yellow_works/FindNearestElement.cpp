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

set<int>::const_iterator FindNearestElement(const set<int> &numbers, int border)
{
    if (numbers.empty())
    {
        return numbers.cend();
    }

    const auto lower = numbers.lower_bound(border);
    const auto upper = numbers.upper_bound(border);

    if (lower == numbers.cend() and upper == numbers.cend())
    {
        return prev(numbers.cend());
    }
    else if (lower == numbers.cbegin() and upper == numbers.cbegin())
    {
        return numbers.cbegin();
    }

    if (lower != upper)
    {
        return lower;
    }

    if ((border - *(prev(lower))) <= (*upper - border))
    {
        return prev(lower);
    }
    return upper;
}

void TestFindNearestElement()
{
    {
        set<int> s;
        Assert(FindNearestElement(s, 1) == s.end(), "0");
        Assert(s.lower_bound(2) == s.end(), "1");
        Assert(s.upper_bound(2) == s.end(), "2");
    }
    {
        set<int> s = {1};
        AssertEqual(*FindNearestElement(s, 1), 1, "3");
        AssertEqual(*FindNearestElement(s, 5), 1, "4");
        Assert(s.upper_bound(2) == s.end(), "5");
    }
    {
        set<int> s = {1, 2};
        AssertEqual(*FindNearestElement(s, 0), 1, "6");
        AssertEqual(*FindNearestElement(s, 2), 2, "7");
    }
    {
        set<int> s = {1, 2, 3, 4, 5};
        AssertEqual(*FindNearestElement(s, 4), 4, "8");
        AssertEqual(*FindNearestElement(s, 5), 5, "9");
        AssertEqual(*FindNearestElement(s, 6), 5, "10");
        AssertEqual(*FindNearestElement(s, 100), 5, "11");
        AssertEqual(*FindNearestElement(s, 1), 1, "12");
        AssertEqual(*FindNearestElement(s, 0), 1, "13");
    }
    {
        set<int> s = {1, 2, 3, 4, 5, 8, 10, 14};
        AssertEqual(*FindNearestElement(s, 6), 5, "14");
        AssertEqual(*FindNearestElement(s, 7), 8, "15");
        AssertEqual(*FindNearestElement(s, 10), 10, "16");
        AssertEqual(*FindNearestElement(s, 12), 10, "17");
        AssertEqual(*FindNearestElement(s, 13), 14, "18");
    }
    {
        set<int> s = {1, 4, 6};
        AssertEqual(*FindNearestElement(s, 0), 1, "19");
        AssertEqual(*FindNearestElement(s, 3), 4, "20");
        AssertEqual(*FindNearestElement(s, 5), 4, "21");
        AssertEqual(*FindNearestElement(s, 6), 6, "22");
        AssertEqual(*FindNearestElement(s, 100), 6, "23");
    }
}

void TestAll()
{
    TestRunner tr = {};
    tr.RunTest(TestFindNearestElement, "TestPersonalBudjet");
}

int main()
{
    TestAll();

    return 0;
}
