#include <profile.h>
#include <test_runner.h>

#include <vector>
#include <string>
#include <iostream>
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

using namespace std;

void TestAll();
void Profile();

struct Domens
{
    unordered_set<string> bad;
    vector<string> for_check;
};

Domens Parse(istream &is)
{
    Domens result;

    size_t domens_count = 0U;
    is >> domens_count;
    for (size_t i = 0; i < domens_count; ++i)
    {
        string domen;
        is >> domen;
        result.bad.insert(move(domen));
    }

    is >> domens_count;
    result.for_check.resize(domens_count);
    for (size_t i = 0; i < domens_count; ++i)
    {
        is >> result.for_check[i];
    }

    return result;
}

vector<string_view> SplitIntoSubdomens(string_view domen)
{
    vector<string_view> result{domen};

    // rdot_pos - reverse dot position
    for (size_t rdot_pos = domen.rfind('.', domen.npos);
         rdot_pos != domen.npos;
         rdot_pos = domen.rfind('.', rdot_pos - 1))
    {
        const string_view sub_domen = domen.substr(rdot_pos + 1);
        result.push_back(sub_domen);
    }

    return result;
}

using IsBad = vector<bool>;

IsBad Filter(const Domens &domens)
{
    IsBad result;
    result.reserve(domens.for_check.size());

    const unordered_set<string_view> bad_domens{domens.bad.cbegin(), domens.bad.cend()};

    for (string_view domen_for_check : domens.for_check)
    {
        bool bad_detected = false;

        vector<string_view> sub_domens = SplitIntoSubdomens(domen_for_check);

        for (const string_view sub_domen : sub_domens)
        {
            if (bad_domens.find(sub_domen) != bad_domens.cend())
            {
                bad_detected = true;
                result.push_back(true);
                break;
            }
        }

        if (not bad_detected)
        {
            result.push_back(false);
        }
    }

    return result;
}

void Print(ostream &os, const IsBad &is_bad)
{
    for (const bool bad : is_bad)
    {
        if (bad)
        {
            os << "Bad" << '\n';
        }
        else
        {
            os << "Good" << '\n';
        }
    }
}

int main()
{
    TestAll();

    cin.tie(nullptr);
    ios_base::sync_with_stdio(false);

    const Domens domens = Parse(cin);
    const IsBad is_bad = Filter(domens);
    Print(cout, is_bad);

    return 0;
}

// void TestParse()
// {
//     istringstream iss(R"(
//         4
//         ya.ru
//         maps.me
//         m.ya.ru
//         com
//         7
//         ya.ru
//         ya.com
//         m.maps.me
//         moscow.m.ya.ru
//         maps.com
//         maps.ru
//         ya.ya
//     )");

//     unordered_set<string> bad_domens_expect{
//         "ya.ru",
//         "maps.me",
//         "m.ya.ru",
//         "com"
//     };

//     vector<string> domens_for_check_expect{
//         "ya.ru",
//         "ya.com",
//         "m.maps.me",
//         "moscow.m.ya.ru",
//         "maps.com",
//         "maps.ru",
//         "ya.ya"
//     };

//     Domens domens = Parse(iss);

//     ASSERT_EQUAL(domens.bad, bad_domens_expect);
//     ASSERT_EQUAL(domens.for_check, domens_for_check_expect);
// }

// void TestFilter()
// {
//     {
//         istringstream iss(R"(
//             4
//             ya.ru
//             maps.me
//             m.ya.ru
//             com
//             7
//             ya.ru
//             ya.com
//             m.maps.me
//             moscow.m.ya.ru
//             maps.com
//             maps.ru
//             ya.ya
//         )");

//         const Domens domens = Parse(iss);

//         const IsBad is_bad = Filter(domens);

//         const IsBad expect{
//            true, true, true, true, true, false, false
//         };

//         ASSERT_EQUAL(is_bad, expect);
//     }

//     {
//         istringstream iss(R"(
//             2
//             com
//             maps.ru
//             7
//             maps.com.ru
//             ya.com
//             ya.common
//             ya.common.com
//             com.ru
//             com.ru.com
//             maps.ru.c.maps.ru
//         )");

//         const Domens domens = Parse(iss);
//         const IsBad is_bad = Filter(domens);

//         const IsBad expect{
//            false, true, false, true, false, true, true
//         };

//         ASSERT_EQUAL(is_bad, expect);
//     }

//     {
//         istringstream iss(R"(
//             2
//             com
//             maps.ru
//             6
//             ya.com
//             ya.common
//             ya.common.com
//             com.ru
//             com.ru.com
//             maps.ru.c.maps.ru
//         )");

//         const Domens domens = Parse(iss);
//         const IsBad is_bad = Filter(domens);

//         const IsBad expect{
//             true, false, true, false, true, true
//         };

//         ASSERT_EQUAL(is_bad, expect);
//     }

//     {
//         istringstream iss(R"(
//             7
//             common.com
//             pop
//             com.com
//             mem.sem.kem.pem
//             a
//             e
//             p
//             13
//             ya.com
//             ya.common
//             ya.common.com
//             com
//             common
//             pol.mem.sem.kem.pem.mol
//             mem.sem.kem.pem.mol
//             pol.mem.sem.kem.pem
//             pol.mem.sam.kem.pem
//             mem.sem.kem
//             poppop
//             po.p
//             pop.pop
//         )");

//         const Domens domens = Parse(iss);
//         const IsBad is_bad = Filter(domens);

//         const IsBad expect{
//             false, false, true, false, false, false, false,
//             true, false, false, false, true, true
//         };

//         ASSERT_EQUAL(is_bad, expect);
//     }
// }

void TestAll()
{
    TestRunner tr{};
    // RUN_TEST(tr, TestParse);
    // RUN_TEST(tr, TestFilter);
}

void Profile()
{
}