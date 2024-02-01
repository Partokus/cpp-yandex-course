#include <profile.h>
#include <test_runner.h>
#include <iomanip>
#include <iostream>
#include <queue>
#include <set>
#include <string>
#include <stdexcept>
#include <numeric>
#include <algorithm>
#include <array>
#include <list>

using namespace std;

void TestAll();
void Profile();

int main()
{
    TestAll();
    Profile();

    // для ускорения чтения данных отключается синхронизация
    // cin и cout с stdio,
    ios::sync_with_stdio(false);

    list<int> sportsmans{};

    static constexpr size_t MaxSportsmanCount = 100'000 + 1;
    vector<list<int>::iterator> sportsmans_iters(MaxSportsmanCount);
    sportsmans_iters.assign(MaxSportsmanCount, sportsmans.end());

    size_t n = 0U;
    cin >> n;

    for (size_t i = 0U; i < n; ++i)
    {
        int sportsman_num = 0;
        int next_sportsman_num = 0;

        cin >> sportsman_num >> next_sportsman_num;

        if (auto it_next_sportsman = sportsmans_iters.at(next_sportsman_num); it_next_sportsman != sportsmans.end())
        {
            auto it = sportsmans.insert(it_next_sportsman, sportsman_num);
            sportsmans_iters[sportsman_num] = it;
        }
        else
        {
            sportsmans.push_back(sportsman_num);
            sportsmans_iters[sportsman_num] = prev(sportsmans.end());
        }
    }

    for (const auto &sportsman_num : sportsmans)
    {
        cout << sportsman_num << " ";
    }
    return 0;
}

void TestAll()
{
    TestRunner tr{};
}

void Profile()
{
}