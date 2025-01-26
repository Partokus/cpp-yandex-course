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
            domens[i].insert(domens[i].begin(), '.');
            domens[i].push_back('.');
        }
        return domens;
    };

    return
    {
        .bad_domens = parse_single_domens(is),
        .domens_for_check = parse_single_domens(is)
    };
}

vector<string> Filter(const AllDomens &all_domens)
{
    vector<string> good_bad;
    good_bad.reserve(all_domens.domens_for_check.size());

    for (const string &domen_for_check : all_domens.domens_for_check)
    {
        bool bad_detected = false;
        for (const string &bad_domen : all_domens.bad_domens)
        {
            if (const size_t pos = domen_for_check.rfind(bad_domen); pos != domen_for_check.npos)
            {
                if ((pos + bad_domen.size()) != domen_for_check.size())
                {
                    // Здесь мы зафиксировали, что плохой домен находится не в конце
                    // проверяемого доммена, а также проверяемый доммен не равен в точности
                    // плохому домену.
                    // То естьдомен com является поддоменом hate.description.com,
                    // и не является поддоменом hate.com.description
                    // Также при плохом домене com, домен com.ru считается хорошим,
                    // так как com не является поддоменом com.ru
                    continue;
                }

                good_bad.push_back("Bad");
                bad_detected = true;
                break;
            }
        }
        if (not bad_detected)
        {
            good_bad.push_back("Good");
        }
    }

    return good_bad;
}

void Print(ostream &os, const vector<string> &good_bad)
{
    for (const string &s : good_bad)
    {
        os << s << endl;
    }
}

int main()
{
    TestAll();

    const AllDomens all_domens = Parse(cin);
    const vector<string> good_bad = Filter(all_domens);
    Print(cout, good_bad);
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
        ".ya.ru.",
        ".maps.me.",
        ".m.ya.ru.",
        ".com."
    };

    Domens domens_for_check_expect{
        ".ya.ru.",
        ".ya.com.",
        ".m.maps.me.",
        ".moscow.m.ya.ru.",
        ".maps.com.",
        ".maps.ru.",
        ".ya.ya."
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
        const vector<string> good_bad = Filter(all_domens);

        vector<string> expect{
            "Bad", "Bad", "Bad", "Bad", "Bad", "Good", "Good"
        };

        ASSERT_EQUAL(good_bad, expect);
    }

    {
        istringstream iss(R"(
            2
            com
            maps.ru
            7
            maps.com.ru
            ya.com
            ya.common
            ya.common.com
            com.ru
            com.ru.com
            maps.ru.c.maps.ru
        )");

        const AllDomens all_domens = Parse(iss);
        const vector<string> good_bad = Filter(all_domens);

        const vector<string> expect{
           "Good", "Bad", "Good", "Bad", "Good", "Bad", "Bad"
        };

        ASSERT_EQUAL(good_bad, expect);
    }

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
    //     const vector<string> good_bad = Filter(all_domens);

    //     const vector<string> expect{
    //         "Bad", "Good", "Bad", "Good", "Bad", "Good"
    //     };

    //     ASSERT_EQUAL(good_bad, expect);
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
    //     const vector<string> good_bad = Filter(all_domens);

    //     const vector<string> expect{
    //         "Good", "Good", "Bad", "Good", "Good", "Bad", "Good",
    //         "Bad", "Good", "Good", "Good", "Bad", "Bad"
    //     };

    //     ASSERT_EQUAL(good_bad, expect);
    // }
}

void TestAll()
{
    TestRunner tr{};
    // RUN_TEST(tr, TestParse);
    RUN_TEST(tr, TestFilter);
}

void Profile()
{
}