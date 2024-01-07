#include <test_runner.h>
#include <profile.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <string>
#include <set>
#include <bitset>

using namespace std;

class RouteManager
{
public:
    void AddRoute(int start, int finish)
    {
        _reachable_lists[start].insert(finish);
        _reachable_lists[finish].insert(start);
    }

    int FindNearestFinish(int start, int finish) const
    {
        int result = abs(start - finish);
        if (not _reachable_lists.count(start))
        {
            return result;
        }
        const set<int> &reachable_stations = _reachable_lists.at(start);

        result = min(result, abs(finish - CalcNearestElem(finish, reachable_stations)));
        return result;
    }

private:
    map<int, set<int>> _reachable_lists{};

    int CalcNearestElem(int elem, const set<int> &s) const
    {
        if (s.size() == 1)
        {
            return *s.cbegin();
        }

        auto it_lower_bound = s.lower_bound(elem);

        if (it_lower_bound == s.cend())
        {
            return *prev(it_lower_bound);
        }
        if (*it_lower_bound == elem)
        {
            return elem;
        }
        if (it_lower_bound == s.cbegin())
        {
            return *it_lower_bound;
        }

        int left = *prev(it_lower_bound);
        int right = *it_lower_bound;
        int diff_from_left = elem - left;
        int diff_from_right = right - elem;

        return diff_from_left < diff_from_right ? left : right;
    }
};

void TestAll();

int main()
{
    TestAll();

    RouteManager routes{};

    int query_count = 0;
    cin >> query_count;

    for (int query_id = 0; query_id < query_count; ++query_id)
    {
        string query_type{};
        cin >> query_type;

        int start = 0;
        int finish = 0;

        cin >> start >> finish;
        if (query_type == "ADD")
        {
            routes.AddRoute(start, finish);
        }
        else if (query_type == "GO")
        {
            cout << routes.FindNearestFinish(start, finish) << "\n";
        }
    }
    return 0;
}

void TestRouteManager()
{
    {
        RouteManager r{};
        r.AddRoute(-2, 5);
        r.AddRoute(10, 4);
        r.AddRoute(5, 8);

        ASSERT_EQUAL(r.FindNearestFinish(0, 5), 5);
        ASSERT_EQUAL(r.FindNearestFinish(-2, -2), 0);
        ASSERT_EQUAL(r.FindNearestFinish(-2, 1), 3);
        ASSERT_EQUAL(r.FindNearestFinish(-2, 2), 3);
        ASSERT_EQUAL(r.FindNearestFinish(-2, 3), 2);
        ASSERT_EQUAL(r.FindNearestFinish(-2, 4), 1);
        ASSERT_EQUAL(r.FindNearestFinish(-2, 5), 0);
        ASSERT_EQUAL(r.FindNearestFinish(-2, 6), 1);

        r.AddRoute(-2, 6);

        ASSERT_EQUAL(r.FindNearestFinish(-2, 6), 0);
        ASSERT_EQUAL(r.FindNearestFinish(-2, 7), 1);
        ASSERT_EQUAL(r.FindNearestFinish(-2, -10), 8);
        ASSERT_EQUAL(r.FindNearestFinish(10, 3), 1);
        ASSERT_EQUAL(r.FindNearestFinish(3, 10), 7);
        ASSERT_EQUAL(r.FindNearestFinish(-2, 4), 1);
        ASSERT_EQUAL(r.FindNearestFinish(5, 8), 0);
        ASSERT_EQUAL(r.FindNearestFinish(5, 7), 1);
        ASSERT_EQUAL(r.FindNearestFinish(5, 6), 1);
        ASSERT_EQUAL(r.FindNearestFinish(5, -1), 1);
    }
}

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, TestRouteManager);
}
