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

using Domens = vector<string>;

struct AllDomens
{
    Domens bad_domens;
    Domens domens_for_check;
};

AllDomens Parse(istream &is)
{
    auto parse_single_domens = [](istream &is)
    {
        size_t domens_count = 0U;
        is >> domens_count;
        Domens domens(domens_count);
        for (size_t i = 0; i < domens_count; ++i)
        {
            is >> domens[i];
        }
        return domens;
    };

    return
    {
        .bad_domens = parse_single_domens(is),
        .domens_for_check = parse_single_domens(is)
    };
}

void FilterAndPrint(const AllDomens &all_domens, ostream &os)
{
    for (const string_view domen_for_check : all_domens.domens_for_check)
    {
        bool bad_detected = false;
        for (const string_view bad_domen : all_domens.bad_domens)
        {
            if (bad_domen.size() < domen_for_check.size())
            {
                if (domen_for_check[domen_for_check.size() - bad_domen.size() - 1] != '.')
                {
                    continue;
                }
            }
            else if (bad_domen.size() > domen_for_check.size())
            {
                continue;
            }

            if (domen_for_check.substr(domen_for_check.size() - bad_domen.size()) == bad_domen)
            {
                os << "Bad\n";
                bad_detected = true;
                break;
            }
        }
        if (not bad_detected)
        {
            os << "Good\n";
        }
    }
}

int main()
{
    TestAll();

    cin.tie(nullptr);
    ios_base::sync_with_stdio(false);

    const AllDomens all_domens = Parse(cin);
    FilterAndPrint(all_domens, cout);
    return 0;
}

void TestParse()
{
    istringstream iss(R"(
        4
        ya.ru
        maps.me
        m.ya.ru
        com
        7
        ya.ru
        ya.com
        m.maps.me
        moscow.m.ya.ru
        maps.com
        maps.ru
        ya.ya
    )");

    Domens bad_domens_expect{
        "ya.ru",
        "maps.me",
        "m.ya.ru",
        "com"
    };

    Domens domens_for_check_expect{
        "ya.ru",
        "ya.com",
        "m.maps.me",
        "moscow.m.ya.ru",
        "maps.com",
        "maps.ru",
        "ya.ya"
    };

    AllDomens all_domens = Parse(iss);

    ASSERT_EQUAL(all_domens.bad_domens, bad_domens_expect);
    ASSERT_EQUAL(all_domens.domens_for_check, domens_for_check_expect);
}

void TestFilter()
{
    {
        istringstream iss(R"(
            4
            ya.ru
            maps.me
            m.ya.ru
            com
            7
            ya.ru
            ya.com
            m.maps.me
            moscow.m.ya.ru
            maps.com
            maps.ru
            ya.ya
        )");

        const AllDomens all_domens = Parse(iss);
        //FilterAndPrint(all_domens, cout);
    }

    //     vector<bool> expect{
    //         true, true, true, true, true, false, false
    //     };

    //     ASSERT_EQUAL(is_bad, expect);
    // }

    // {
    //     istringstream iss(R"(
    //         2
    //         com
    //         maps.ru
    //         7
    //         maps.com.ru
    //         ya.com
    //         ya.common
    //         ya.common.com
    //         com.ru
    //         com.ru.com
    //         maps.ru.c.maps.ru
    //     )");

    //     const AllDomens all_domens = Parse(iss);
    //     const vector<bool> is_bad = Filter(all_domens);

    //     const vector<bool> expect{
    //        false, true, false, true, false, true, true
    //     };

    //     ASSERT_EQUAL(is_bad, expect);
    // }

    // {
    //     istringstream iss(R"(
    //         2
    //         com
    //         maps.ru
    //         6
    //         ya.com
    //         ya.common
    //         ya.common.com
    //         com.ru
    //         com.ru.com
    //         maps.ru.c.maps.ru
    //     )");

    //     const AllDomens all_domens = Parse(iss);
    //     const vector<bool> is_bad = Filter(all_domens);

    //     const vector<bool> expect{
    //         true, false, true, false, true, true
    //     };

    //     ASSERT_EQUAL(is_bad, expect);
    // }

    // {
    //     istringstream iss(R"(
    //         7
    //         common.com
    //         pop
    //         com.com
    //         mem.sem.kem.pem
    //         a
    //         e
    //         p
    //         13
    //         ya.com
    //         ya.common
    //         ya.common.com
    //         com
    //         common
    //         pol.mem.sem.kem.pem.mol
    //         mem.sem.kem.pem.mol
    //         pol.mem.sem.kem.pem
    //         pol.mem.sam.kem.pem
    //         mem.sem.kem
    //         poppop
    //         po.p
    //         pop.pop
    //     )");

    //     const AllDomens all_domens = Parse(iss);
    //     const vector<bool> is_bad = Filter(all_domens);

    //     const vector<bool> expect{
    //         false, false, true, false, false, false, false,
    //         true, false, false, false, true, true
    //     };

    //     ASSERT_EQUAL(is_bad, expect);
    // }
}

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, TestParse);
    RUN_TEST(tr, TestFilter);
}

void Profile()
{
}