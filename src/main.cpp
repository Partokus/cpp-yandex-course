#include <profile.h>
#include <test_runner.h>

#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
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
#include <ctime>

using namespace std;

void TestAll();
void Profile();

class Date
{
public:
    time_t AsTimestamp() const
    {
        std::tm t;
        t.tm_sec = 0;
        t.tm_min = 0;
        t.tm_hour = 0;
        t.tm_mday = day;
        t.tm_mon = month - 1;
        t.tm_year = year - 1900;
        t.tm_isdst = 0;
        return mktime(&t);
    }

    int day = 1U;
    int month = 1U;
    int year = 2000U;

    bool operator<(const Date &other) const
    {
        return tie(year, month, day) < tie(other.year, other.month, other.day);
    }

    bool operator==(const Date &other) const
    {
        return tie(year, month, day) == tie(other.year, other.month, other.day);
    }

    bool operator<=(const Date &other) const
    {
        return tie(year, month, day) <= tie(other.year, other.month, other.day);
    }

    bool operator!=(const Date &other) const
    {
        return tie(year, month, day) != tie(other.year, other.month, other.day);
    }

    Date & AddDay()
    {
        std::tm t;
        t.tm_sec = 0;
        t.tm_min = 0;
        t.tm_hour = 0;
        t.tm_mday = day + 1;
        t.tm_mon = month - 1;
        t.tm_year = year - 1900;
        t.tm_isdst = 0;
        time_t time = mktime(&t);
        t = *localtime(&time);
        day = t.tm_mday;
        month = t.tm_mon + 1;
        year = t.tm_year + 1900;
        return *this;
    }
};

class PersonalBudjet
{
public:
    void Earn(const Date &from, const Date &to, double value)
    {
        _partial_sum_updated = false;

        const double value_for_day = value / static_cast<double>((ComputeDaysDiff(from, to) + 1));

        for (Date date = from; date <= to; date.AddDay())
        {
            _budjet[date].value += value_for_day;
        }
    }

    void PayTax(const Date &from, const Date &to)
    {
        _partial_sum_updated = false;

        static constexpr double TaxPay = 0.87; // 13%

        for (auto it = _budjet.lower_bound(from);
             it != _budjet.cend() and it->first <= to;
             ++it)
        {
            it->second.value *= TaxPay;
        }
    }

    double ComputeIncome(const Date &from, const Date &to)
    {
        auto it_from = _budjet.lower_bound(from);
        if (it_from == _budjet.cend())
        {
            return 0.0;
        }
        if (to < it_from->first)
        {
            return 0.0;
        }

        auto it_to = prev(_budjet.upper_bound(to));

        if (not _partial_sum_updated)
        {
            UpdatePartialSum(_budjet);
            _partial_sum_updated = true;
        }

        return ComputeIncome(it_from->second, it_to->second);
    }

private:
    friend class PersonalBudjetTester;

    struct ValueStat
    {
        double value = 0.0;
        double partial_sum = 0.0;
    };

    map<Date, ValueStat> _budjet;
    bool _partial_sum_updated = false;

    int ComputeDaysDiff(const Date &from, const Date &to) const
    {
        const time_t timestamp_to = to.AsTimestamp();
        const time_t timestamp_from = from.AsTimestamp();
        static constexpr int SecondsInDay = 60 * 60 * 24;
        return (timestamp_to - timestamp_from) / SecondsInDay;
    }

    void UpdatePartialSum(map<Date, ValueStat> &budjet)
    {
        double sum = 0.0;
        for (auto &[date, value_stat] : budjet)
        {
            value_stat.partial_sum = sum;
            sum += value_stat.value;
        }
    }

    double ComputeIncome(const ValueStat &from, const ValueStat &to) const
    {
        double result = to.partial_sum - from.partial_sum + to.value;
        return result;
    }
};

Date ParseDate(istream &is)
{
    Date result;
    char separ;

    is >> result.year >> separ >> result.month >> separ >> result.day;

    return result;
}

void ProcessQuery(istream &is, ostream &os)
{
    PersonalBudjet pb;

    int query_count = 0;
    is >> query_count;

    for (int i = 0; i < query_count; ++i)
    {
        string query;
        is >> query;

        Date from = ParseDate(is);
        Date to = ParseDate(is);

        if (query == "Earn")
        {
            double value = 0.0;
            is >> value;
            pb.Earn(from, to, value);
        }
        else if (query == "ComputeIncome")
        {
            os << setprecision(25) << pb.ComputeIncome(from, to) << '\n';
        }
        else if (query == "PayTax")
        {
            pb.PayTax(from, to);
        }
    }
}

int main()
{
    // для ускорения чтения данных отключается синхронизация
    // cin и cout с stdio,
    // а также выполняется отвязка cin от cout
    // ios::sync_with_stdio(false);
    // cin.tie(nullptr);

    TestAll();

    ProcessQuery(cin, cout);

    return 0;
}

class PersonalBudjetTester
{
public:
    static void TestEarn()
    {
        {
            PersonalBudjet pb;
            pb.Earn(
                Date{.day = 1, .month = 1, .year = 2000},
                Date{.day = 1, .month = 1, .year = 2000},
                9.0
            );
            ASSERT_EQUAL(pb._budjet.at(Date{.day = 1, .month = 1, .year = 2000}).value, 9.0);
        }
        {
            PersonalBudjet pb;
            pb.Earn(
                Date{.day = 1, .month = 1, .year = 2000},
                Date{.day = 3, .month = 1, .year = 2000},
                9.0
            );
            ASSERT_EQUAL(pb._budjet.at(Date{.day = 1, .month = 1, .year = 2000}).value, 3.0);
            ASSERT_EQUAL(pb._budjet.at(Date{.day = 2, .month = 1, .year = 2000}).value, 3.0);
            ASSERT_EQUAL(pb._budjet.at(Date{.day = 3, .month = 1, .year = 2000}).value, 3.0);
            pb.Earn(
                Date{.day = 1, .month = 1, .year = 2000},
                Date{.day = 3, .month = 1, .year = 2000},
                9.0
            );
            ASSERT_EQUAL(pb._budjet.at(Date{.day = 1, .month = 1, .year = 2000}).value, 6.0);
            ASSERT_EQUAL(pb._budjet.at(Date{.day = 2, .month = 1, .year = 2000}).value, 6.0);
            ASSERT_EQUAL(pb._budjet.at(Date{.day = 3, .month = 1, .year = 2000}).value, 6.0);
        }
        {
            PersonalBudjet pb;
            pb.Earn(
                Date{.day = 1, .month = 1, .year = 2000},
                Date{.day = 3, .month = 1, .year = 2000},
                8.4
            );
            double value = pb._budjet.at(Date{.day = 1, .month = 1, .year = 2000}).value;
            ASSERT(value > 2.7 and value < 2.9);
            value = pb._budjet.at(Date{.day = 2, .month = 1, .year = 2000}).value;
            ASSERT(value > 2.7 and value < 2.9);
            value = pb._budjet.at(Date{.day = 3, .month = 1, .year = 2000}).value;
            ASSERT(value > 2.7 and value < 2.9);
        }
    }

    static void TestPayTax()
    {
        {
            PersonalBudjet pb;
            pb.Earn(
                Date{.day = 1, .month = 1, .year = 2000},
                Date{.day = 1, .month = 1, .year = 2000},
                10.0
            );
            pb.PayTax(Date{.day = 1, .month = 1, .year = 2000},
                      Date{.day = 1, .month = 1, .year = 2000});

            double value = pb._budjet.at(Date{.day = 1, .month = 1, .year = 2000}).value;
            ASSERT_EQUAL(value, 8.7);
        }
        {
            PersonalBudjet pb;
            pb.Earn(
                Date{.day = 1, .month = 1, .year = 2000},
                Date{.day = 3, .month = 1, .year = 2000},
                9.0
            );
            pb.PayTax(Date{.day = 1, .month = 1, .year = 2000},
                      Date{.day = 3, .month = 1, .year = 2000});

            double value = pb._budjet.at(Date{.day = 1, .month = 1, .year = 2000}).value;
            ASSERT(value > 2.6 and value < 2.62);

            pb.PayTax(Date{.day = 1, .month = 1, .year = 2000},
                      Date{.day = 3, .month = 1, .year = 2000});

            value = pb._budjet.at(Date{.day = 1, .month = 1, .year = 2000}).value;
            ASSERT(value > 2.26 and value < 2.28);
        }
    }

    static void TestComputeIncome()
    {
        {
            PersonalBudjet pb;
            pb.Earn(
                Date{.day = 1, .month = 1, .year = 2000},
                Date{.day = 1, .month = 1, .year = 2000},
                9.0
            );

            double income = pb.ComputeIncome(Date{.day = 1, .month = 1, .year = 2000},
                                             Date{.day = 1, .month = 1, .year = 2001});
            ASSERT_EQUAL(income, 9.0);

            pb.Earn(
                Date{.day = 2, .month = 1, .year = 2000},
                Date{.day = 2, .month = 1, .year = 2000},
                9.0
            );

            income = pb.ComputeIncome(Date{.day = 1, .month = 1, .year = 2000},
                                      Date{.day = 1, .month = 1, .year = 2001});
            ASSERT_EQUAL(income, 18.0);

            income = pb.ComputeIncome(Date{.day = 2, .month = 1, .year = 2000},
                                      Date{.day = 2, .month = 1, .year = 2000});
            ASSERT_EQUAL(income, 9.0);

            income = pb.ComputeIncome(Date{.day = 3, .month = 1, .year = 2000},
                                      Date{.day = 1, .month = 1, .year = 2001});
            ASSERT_EQUAL(income, 0.0);

            pb.PayTax(Date{.day = 1, .month = 1, .year = 2000},
                      Date{.day = 2, .month = 1, .year = 2001});

            income = pb.ComputeIncome(Date{.day = 1, .month = 1, .year = 2000},
                                      Date{.day = 1, .month = 1, .year = 2000});
            ASSERT(income > 7.82 and income < 7.84);

            income = pb.ComputeIncome(Date{.day = 1, .month = 1, .year = 2000},
                                      Date{.day = 2, .month = 1, .year = 2000});
            ASSERT(income > 15.65 and income < 15.67);

            pb.PayTax(Date{.day = 1, .month = 1, .year = 2000},
                      Date{.day = 2, .month = 1, .year = 2001});

            income = pb.ComputeIncome(Date{.day = 1, .month = 1, .year = 2000},
                                      Date{.day = 2, .month = 1, .year = 2000});
            ASSERT(income > 13.61 and income < 13.63);

            pb.PayTax(Date{.day = 1, .month = 1, .year = 1999},
                      Date{.day = 31, .month = 12, .year = 1999});

            income = pb.ComputeIncome(Date{.day = 1, .month = 1, .year = 2000},
                                      Date{.day = 2, .month = 1, .year = 2000});
            ASSERT(income > 13.61 and income < 13.63);
        }
        {
            PersonalBudjet pb;
            pb.Earn(
                Date{.day = 1, .month = 1, .year = 2000},
                Date{.day = 3, .month = 1, .year = 2000},
                8.4
            );
            double income = pb.ComputeIncome(Date{.day = 1, .month = 1, .year = 2000},
                                             Date{.day = 1, .month = 1, .year = 2001});
            ASSERT(income > 8.3 and income < 8.5);

            income = pb.ComputeIncome(Date{.day = 1, .month = 1, .year = 2000},
                                      Date{.day = 2, .month = 1, .year = 2000});
            ASSERT(income > 5.5 and income < 5.7);
        }
        {
            PersonalBudjet pb;
            pb.Earn(
                Date{.day = 1, .month = 1, .year = 2000},
                Date{.day = 2, .month = 1, .year = 2000},
                4.0
            );
            pb.Earn(
                Date{.day = 4, .month = 1, .year = 2000},
                Date{.day = 4, .month = 1, .year = 2000},
                2.0
            );
            double income = pb.ComputeIncome(Date{.day = 1, .month = 1, .year = 2000},
                                             Date{.day = 3, .month = 1, .year = 2000});
            ASSERT_EQUAL(income, 4.0);

            income = pb.ComputeIncome(Date{.day = 1, .month = 1, .year = 2000},
                                      Date{.day = 4, .month = 1, .year = 2000});
            ASSERT_EQUAL(income, 6.0);
        }
        {
            PersonalBudjet pb;
            pb.Earn(
                Date{.day = 1, .month = 1, .year = 2000},
                Date{.day = 31, .month = 12, .year = 2000},
                366.0
            );
            double income = pb.ComputeIncome(Date{.day = 1, .month = 1, .year = 2000},
                                             Date{.day = 1, .month = 1, .year = 2000});
            ASSERT_EQUAL(income, 1.0);
            pb.Earn(
                Date{.day = 1, .month = 1, .year = 2001},
                Date{.day = 31, .month = 12, .year = 2001},
                365.0
            );
            income = pb.ComputeIncome(Date{.day = 1, .month = 1, .year = 2001},
                                      Date{.day = 1, .month = 1, .year = 2001});
            ASSERT_EQUAL(income, 1.0);
        }
        {
            PersonalBudjet pb;
            for (int year = 2000; year <= 2099; ++year)
            {
                pb.Earn(
                    Date{.day = 1, .month = 1, .year = year},
                    Date{.day = 1, .month = 1, .year = year},
                    5.0
                );
            }

            double income = pb.ComputeIncome(Date{.day = 1, .month = 1, .year = 2000},
                                             Date{.day = 1, .month = 1, .year = 2099});
            ASSERT_EQUAL(income, 500.0);

            pb.PayTax(Date{.day = 1, .month = 1, .year = 2000},
                      Date{.day = 1, .month = 1, .year = 2099});

            income = pb.ComputeIncome(Date{.day = 1, .month = 1, .year = 2000},
                                      Date{.day = 1, .month = 1, .year = 2099});
            ASSERT(income > 434.0 and income < 436.0);

            pb.PayTax(Date{.day = 1, .month = 1, .year = 2000},
                      Date{.day = 1, .month = 1, .year = 2099});

            income = pb.ComputeIncome(Date{.day = 1, .month = 1, .year = 2000},
                                      Date{.day = 1, .month = 1, .year = 2099});
            ASSERT(income > 378.3 and income < 378.5);

            for (int year = 2000; year <= 2099; ++year)
            {
                pb.Earn(
                    Date{.day = 1, .month = 1, .year = year},
                    Date{.day = 1, .month = 1, .year = year},
                    5.0
                );
            }
            income = pb.ComputeIncome(Date{.day = 1, .month = 1, .year = 2000},
                                      Date{.day = 1, .month = 1, .year = 2099});
            ASSERT(income > 878.3 and income < 878.5);
        }
        {
            PersonalBudjet pb;

            pb.Earn(
                Date{.day = 2, .month = 1, .year = 2000},
                Date{.day = 2, .month = 1, .year = 2000},
                2.0
            );
            pb.Earn(
                Date{.day = 3, .month = 1, .year = 2000},
                Date{.day = 3, .month = 1, .year = 2000},
                2.0
            );
            pb.Earn(
                Date{.day = 4, .month = 1, .year = 2000},
                Date{.day = 4, .month = 1, .year = 2000},
                2.0
            );
            pb.PayTax(Date{.day = 2, .month = 1, .year = 2000},
                      Date{.day = 2, .month = 1, .year = 2000});
            pb.PayTax(Date{.day = 1, .month = 1, .year = 2000},
                      Date{.day = 1, .month = 1, .year = 2000});
            pb.PayTax(Date{.day = 5, .month = 1, .year = 2000},
                      Date{.day = 5, .month = 1, .year = 2000});
            pb.PayTax(Date{.day = 5, .month = 1, .year = 2000},
                      Date{.day = 5, .month = 1, .year = 2099});

            double income = pb.ComputeIncome(Date{.day = 2, .month = 1, .year = 2000},
                                             Date{.day = 2, .month = 1, .year = 2000});
            ASSERT(income > 1.73 and income < 1.75);

            income = pb.ComputeIncome(Date{.day = 3, .month = 1, .year = 2000},
                                      Date{.day = 3, .month = 1, .year = 2000});
            ASSERT(income > 1.9 and income < 2.1);

            income = pb.ComputeIncome(Date{.day = 4, .month = 1, .year = 2000},
                                      Date{.day = 4, .month = 1, .year = 2000});
            ASSERT(income > 1.9 and income < 2.1);

            income = pb.ComputeIncome(Date{.day = 5, .month = 1, .year = 2000},
                                      Date{.day = 5, .month = 1, .year = 2000});
            ASSERT(income == 0.0);
            income = pb.ComputeIncome(Date{.day = 1, .month = 1, .year = 2000},
                                      Date{.day = 1, .month = 1, .year = 2000});
            ASSERT(income == 0.0);
            income = pb.ComputeIncome(Date{.day = 2, .month = 1, .year = 2000},
                                      Date{.day = 4, .month = 1, .year = 2000});
            ASSERT(income > 5.73 and income < 5.75);
            income = pb.ComputeIncome(Date{.day = 2, .month = 1, .year = 2001},
                                      Date{.day = 4, .month = 1, .year = 2099});
            ASSERT(income == 0.0);
            pb.PayTax(Date{.day = 3, .month = 1, .year = 2000},
                      Date{.day = 1, .month = 1, .year = 2099});
            income = pb.ComputeIncome(Date{.day = 2, .month = 1, .year = 2000},
                                      Date{.day = 4, .month = 1, .year = 2000});
            ASSERT(income > 5.21 and income < 5.23);
        }
        {
            PersonalBudjet pb;

            pb.Earn(
                Date{.day = 1, .month = 1, .year = 2000},
                Date{.day = 31, .month = 12, .year = 2099},
                36525
            );
            double income = pb.ComputeIncome(Date{.day = 1, .month = 1, .year = 2000},
                                             Date{.day = 1, .month = 1, .year = 2000});
            ASSERT_EQUAL(income, 1.0);
            pb.PayTax(Date{.day = 1, .month = 1, .year = 2000},
                      Date{.day = 31, .month = 12, .year = 2099});
            income = pb.ComputeIncome(Date{.day = 1, .month = 1, .year = 2000},
                                      Date{.day = 1, .month = 1, .year = 2000});
            ASSERT_EQUAL(income, 0.87);
        }
    }

    static void TestComputeIncomeFromValueStat()
    {
        PersonalBudjet pb;

        {
            PersonalBudjet::ValueStat from{.value = 5, .partial_sum = 5};
            PersonalBudjet::ValueStat to{.value = 5, .partial_sum = 10};
            ASSERT_EQUAL(pb.ComputeIncome(from, to), 10);
        }
        {
            PersonalBudjet::ValueStat from{.value = 5, .partial_sum = 5};
            PersonalBudjet::ValueStat to{.value = 5, .partial_sum = 5};
            ASSERT_EQUAL(pb.ComputeIncome(from, to), 5);
        }
    }

    static void TestUpdatePartialSum()
    {
        PersonalBudjet pb;

        {
            Date date1{.day = 1, .month = 1, .year = 2000};
            Date date2{.day = 2, .month = 1, .year = 2000};
            Date date3{.day = 3, .month = 1, .year = 2000};

            PersonalBudjet::ValueStat value_stat1{.value = 5, .partial_sum = 0};
            PersonalBudjet::ValueStat value_stat2{.value = 10, .partial_sum = 0};
            PersonalBudjet::ValueStat value_stat3{.value = 15, .partial_sum = 0};

            pb._budjet[date1] = value_stat1;
            pb.UpdatePartialSum(pb._budjet);
            ASSERT_EQUAL(pb._budjet[date1].partial_sum, 0);

            pb._budjet[date2] = value_stat2;
            pb.UpdatePartialSum(pb._budjet);
            ASSERT_EQUAL(pb._budjet[date1].partial_sum, 0);
            ASSERT_EQUAL(pb._budjet[date2].partial_sum, 5);

            pb._budjet[date3] = value_stat3;
            pb.UpdatePartialSum(pb._budjet);
            ASSERT_EQUAL(pb._budjet[date1].partial_sum, 0);
            ASSERT_EQUAL(pb._budjet[date2].partial_sum, 5);
            ASSERT_EQUAL(pb._budjet[date3].partial_sum, 15);
        }
    }
};

void TestDateAddDay()
{
    {
        Date date{.day = 31, .month = 3, .year = 2000};
        date.AddDay();
        ASSERT_EQUAL(date.day, 1);
        ASSERT_EQUAL(date.month, 4);
        ASSERT_EQUAL(date.year, 2000);
    }
    {
        Date date{.day = 28, .month = 2, .year = 2000};
        date.AddDay();
        ASSERT_EQUAL(date.day, 29);
        ASSERT_EQUAL(date.month, 2);
        ASSERT_EQUAL(date.year, 2000);
    }
    {
        Date date{.day = 28, .month = 2, .year = 2001};
        date.AddDay();
        ASSERT_EQUAL(date.day, 1);
        ASSERT_EQUAL(date.month, 3);
        ASSERT_EQUAL(date.year, 2001);
    }
}

void TestProcessQuery()
{
    istringstream iss(R"(
        8
        Earn 2000-01-02 2000-01-06 20
        ComputeIncome 2000-01-01 2001-01-01
        PayTax 2000-01-02 2000-01-03
        ComputeIncome 2000-01-01 2001-01-01
        Earn 2000-01-03 2000-01-03 10
        ComputeIncome 2000-01-01 2001-01-01
        PayTax 2000-01-03 2000-01-03
        ComputeIncome 2000-01-01 2001-01-01
    )");

    ostringstream oss;

    ProcessQuery(iss, oss);

    string expect = "20\n18.96000000000000085265128\n28.96000000000000085265128\n27.20759999999999934061634\n";

    ASSERT_EQUAL(oss.str(), expect);
}

void TestPersonalBudjet()
{
    {
        PersonalBudjet pb{};
        double value = double(10.9) / 10.0;
        pb.Earn({2000, 1, 1}, {2000, 1, 10}, 10.9);
        AssertEqual(pb.ComputeIncome({2000, 1, 1}, {2099, 12, 31}), value * 10, "PersonalBudjet 0");
        AssertEqual(pb.ComputeIncome({2000, 1, 1}, {2000, 1, 1}), value * 1, "PersonalBudjet 1");
        AssertEqual(pb.ComputeIncome({2000, 1, 1}, {2000, 1, 2}), value * 2, "PersonalBudjet 2");
        AssertEqual(pb.ComputeIncome({2000, 1, 1}, {2000, 1, 10}), value * 10, "PersonalBudjet 3");
        AssertEqual(pb.ComputeIncome({2000, 1, 8}, {2099, 12, 28}), value * 3, "PersonalBudjet 4");
        AssertEqual(pb.ComputeIncome({2000, 1, 4}, {2000, 1, 7}), value * 4, "PersonalBudjet 5");
    }
    {
        PersonalBudjet pb{};
        pb.Earn({2000, 1, 1}, {2000, 1, 10}, 100);
        AssertEqual(pb.ComputeIncome({2000, 1, 10}, {2000, 1, 12}), 10, "PersonalBudjet 6");
    }
    {
        PersonalBudjet pb{};
        pb.Earn({2003, 2, 27}, {2003, 3, 2}, 11.2);
        AssertEqual(pb.ComputeIncome({2003, 2, 27}, {2003, 3, 2}), double{11.2}, "PersonalBudjet 7");
    }
    {
        PersonalBudjet pb{};
        pb.Earn({2000, 1, 1}, {2001, 1, 1}, 400);
        AssertEqual(pb.ComputeIncome({2000, 1, 1}, {2001, 1, 1}), 399.9999999999986357579473, "PersonalBudjet 8");
    }
    {
        PersonalBudjet pb{};
        pb.Earn({2000, 1, 1}, {2000, 1, 15}, 65.0);
        pb.Earn({2000, 2, 5}, {2000, 2, 15}, 30.0);
        pb.Earn({2002, 6, 5}, {2002, 6, 20}, 35.0);
        pb.Earn({2065, 6, 5}, {2065, 6, 20}, 40.0);
        double value = 65.0 + 30.0 + 35.0 + 40.0;
        double res = pb.ComputeIncome({2000, 1, 1}, {2099, 1, 1});
        Assert(res >= (value - 0.1) and res <= (value + 0.1), "PersonalBudjet 9");
        value = 65.0 + 30.0 + 35.0;
        res = pb.ComputeIncome({2000, 1, 1}, {2005, 1, 1});
        Assert(res >= (value - 0.1) and res <= (value + 0.1), "PersonalBudjet 10");
        pb.Earn({2065, 6, 5}, {2065, 6, 20}, 40.0);
        value = 80.0;
        res = pb.ComputeIncome({2005, 1, 1}, {2099, 1, 1});
        Assert(res >= (value - 0.1) and res <= (value + 0.1), "PersonalBudjet 11");
    }
    {
        PersonalBudjet pb{};
        pb.Earn({2000, 1, 1}, {2000, 1, 15}, 65.0);
        pb.Earn({2000, 2, 5}, {2000, 2, 15}, 30.0);
        pb.Earn({2002, 6, 5}, {2002, 6, 20}, 35.0);
        pb.Earn({2065, 6, 5}, {2065, 6, 20}, 40.0);
        double value = 65.0 + 30.0 + 35.0 + 40.0;
        double res = pb.ComputeIncome({2000, 1, 1}, {2099, 1, 1});
        Assert(res >= (value - 0.1) and res <= (value + 0.1), "PersonalBudjet 12");
        value = 65.0 + 30.0 + 35.0;
        res = pb.ComputeIncome({2000, 1, 1}, {2005, 1, 1});
        Assert(res >= (value - 0.1) and res <= (value + 0.1), "PersonalBudjet 13");
        pb.Earn({2065, 6, 5}, {2065, 6, 20}, 40.0);
        value = 80.0;
        res = pb.ComputeIncome({2005, 1, 1}, {2099, 1, 1});
        Assert(res >= (value - 0.1) and res <= (value + 0.1), "PersonalBudjet 14");
    }
    {
        PersonalBudjet pb{};
        pb.Earn({2000, 1, 1}, {2099, 12, 31}, 30000.0);
        pb.Earn({2000, 1, 1}, {2099, 12, 30}, 30000.0);
        pb.Earn({2000, 1, 1}, {2099, 12, 29}, 30000.0);
        pb.Earn({2000, 1, 1}, {2099, 12, 28}, 30000.0);
        pb.Earn({2000, 1, 1}, {2099, 12, 27}, 30000.0);
        pb.Earn({2000, 1, 1}, {2099, 12, 26}, 30000.0);
        pb.Earn({2000, 1, 1}, {2099, 12, 25}, 30000.0);
        pb.Earn({2000, 1, 1}, {2099, 12, 24}, 30000.0);
        pb.Earn({2000, 1, 1}, {2099, 12, 23}, 30000.0);
        pb.Earn({2000, 1, 1}, {2099, 12, 22}, 30000.0);
        pb.Earn({2000, 1, 1}, {2099, 12, 21}, 30000.0);
        pb.Earn({2000, 1, 1}, {2099, 12, 20}, 30000.0);
        auto res = pb.ComputeIncome({2000, 1, 1}, {2099, 12, 29});
        AssertEqual(res, 359997.5359119722270406783, "PersonalBudjet 15");
    }
    {
        PersonalBudjet pb{};
        pb.Earn({2000, 1, 2}, {2000, 1, 6}, 20);
        AssertEqual(pb.ComputeIncome({2000, 1, 2}, {2001, 1, 1}), 20, "PersonalBudjet 16");
        AssertEqual(pb.ComputeIncome({2000, 1, 1}, {2000, 1, 3}), 8, "PersonalBudjet 17");
        pb.Earn({2000, 1, 3}, {2000, 1, 3}, 10);
        AssertEqual(pb.ComputeIncome({2000, 1, 1}, {2001, 1, 1}), 30, "PersonalBudjet 18");
    }
    {
        PersonalBudjet pb{};
        pb.Earn({2000, 1, 1}, {2099, 12, 31}, 1000000.0);
        AssertEqual(pb.ComputeIncome({2000, 1, 1}, {2099, 12, 31}), 999999.9999993942910805345, "PersonalBudjet 19");
    }
    {
        PersonalBudjet pb{};
        pb.Earn({2000, 1, 1}, {2099, 12, 31}, 1000000.0);
        AssertEqual(pb.ComputeIncome({2000, 1, 1}, {2099, 12, 31}), 999999.9999993942910805345, "PersonalBudjet 20");
    }
}

void TestAll()
{
    TestRunner tr{};
    // RUN_TEST(tr, TestPersonalBudjet);
    RUN_TEST(tr, TestProcessQuery);
    RUN_TEST(tr, TestDateAddDay);
    RUN_TEST(tr, PersonalBudjetTester::TestEarn);
    RUN_TEST(tr, PersonalBudjetTester::TestPayTax);
    RUN_TEST(tr, PersonalBudjetTester::TestComputeIncome);
    RUN_TEST(tr, PersonalBudjetTester::TestComputeIncomeFromValueStat);
    RUN_TEST(tr, PersonalBudjetTester::TestUpdatePartialSum);
}

void Profile()
{
}