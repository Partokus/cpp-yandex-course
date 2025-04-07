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

    bool operator>=(const Date &other) const
    {
        return tie(year, month, day) >= tie(other.year, other.month, other.day);
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
        if (day < 28)
        {
            ++day;
            return *this;
        }

        int max_day = 0;
        switch (month)
        {
        case 4:
        case 6:
        case 9:
        case 11:
            max_day = 30;
            break;
        case 2:
            max_day = year % 4 == 0 ? 29 : 28;
            break;
        default:
            max_day = 31;
            break;
        }

        if (day == max_day)
        {
            day = 1;
            if (month == 12)
            {
                month = 1;
                ++year;
            }
            else
                ++month;
        }
        else
        {
            ++day;
        }

        return *this;
    }
};

class PersonalBudjet
{
public:
    void Earn(const Date &from, const Date &to, double value)
    {
        EarnOrSpend(from, to, value, Action::Earn);
    }

    void Spend(const Date &from, const Date &to, double value)
    {
        EarnOrSpend(from, to, value, Action::Spend);
    }

    void PayTax(const Date &from, const Date &to, int percentage)
    {
        const double tax_pay = 1.0 - (percentage / 100.0);

        auto begin = _budjet.lower_bound(from);

        for (auto it = begin;
             it != _budjet.cend() and it->first <= to;
             ++it)
        {
            it->second.earned *= tax_pay;
        }

        if (_it_update_partial_sum_start_with != _budjet.end() and
            from >= _it_update_partial_sum_start_with->first)
        {
            return;
        }

        _it_update_partial_sum_start_with = begin;
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

        if (from != to and _it_update_partial_sum_start_with != _budjet.end())
        {
            UpdatePartialSum(_budjet);
            _it_update_partial_sum_start_with = _budjet.end();
        }

        return ComputeIncome(it_from->second, it_to->second);
    }

private:
    friend class PersonalBudjetTester;

    struct ValueStat
    {
        double earned = 0.0;
        double spent = 0.0;
        double partial_sum = 0.0;
    };

    using Budjet = map<Date, ValueStat>;
    Budjet _budjet;

    Budjet::iterator _it_update_partial_sum_start_with = _budjet.end();

    int ComputeDaysDiff(const Date &from, const Date &to) const
    {
        const time_t timestamp_to = to.AsTimestamp();
        const time_t timestamp_from = from.AsTimestamp();
        static constexpr int SecondsInDay = 60 * 60 * 24;
        return (timestamp_to - timestamp_from) / SecondsInDay;
    }

    void UpdatePartialSum(map<Date, ValueStat> &budjet)
    {
        double init_sum = 0.0;

        if (_it_update_partial_sum_start_with != _budjet.begin())
        {
            auto prev_it = prev(_it_update_partial_sum_start_with);
            init_sum = prev_it->second.partial_sum + prev_it->second.earned - prev_it->second.spent;
        }

        accumulate(_it_update_partial_sum_start_with, _budjet.end(), init_sum,
            [](double sum, auto &p)
            {
                p.second.partial_sum = sum;
                return sum + p.second.earned - p.second.spent;
            }
        );
    }

    double ComputeIncome(const ValueStat &from, const ValueStat &to) const
    {
        return to.partial_sum - from.partial_sum + to.earned - to.spent;
    }

    enum class Action
    {
        Earn,
        Spend
    };

    void EarnOrSpend(const Date &from, const Date &to, double value, Action action)
    {
        if (value == 0.0)
        {
            return;
        }

        const double value_for_day = value / static_cast<double>((ComputeDaysDiff(from, to) + 1));

        if (action == Action::Earn)
            for (Date date = from; date <= to; date.AddDay())
                _budjet[date].earned += value_for_day;
        else
            for (Date date = from; date <= to; date.AddDay())
                _budjet[date].spent += value_for_day;

        if (_it_update_partial_sum_start_with != _budjet.end() and
            from >= _it_update_partial_sum_start_with->first)
        {
            return;
        }

        _it_update_partial_sum_start_with = _budjet.find(from);
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
        else if (query == "Spend")
        {
            double value = 0.0;
            is >> value;
            pb.Spend(from, to, value);
        }
        else if (query == "ComputeIncome")
        {
            os << setprecision(25) << pb.ComputeIncome(from, to) << '\n';
        }
        else if (query == "PayTax")
        {
            int percentage = 0;
            is >> percentage;
            pb.PayTax(from, to, percentage);
        }
    }
}

int main()
{
    // для ускорения чтения данных отключается синхронизация
    // cin и cout с stdio,
    // а также выполняется отвязка cin от cout
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

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
            ASSERT_EQUAL(pb._budjet.at(Date{.day = 1, .month = 1, .year = 2000}).earned, 9.0);
        }
        {
            PersonalBudjet pb;
            pb.Earn(
                Date{.day = 1, .month = 1, .year = 2000},
                Date{.day = 3, .month = 1, .year = 2000},
                9.0
            );
            ASSERT_EQUAL(pb._budjet.at(Date{.day = 1, .month = 1, .year = 2000}).earned, 3.0);
            ASSERT_EQUAL(pb._budjet.at(Date{.day = 2, .month = 1, .year = 2000}).earned, 3.0);
            ASSERT_EQUAL(pb._budjet.at(Date{.day = 3, .month = 1, .year = 2000}).earned, 3.0);
            pb.Earn(
                Date{.day = 1, .month = 1, .year = 2000},
                Date{.day = 3, .month = 1, .year = 2000},
                9.0
            );
            ASSERT_EQUAL(pb._budjet.at(Date{.day = 1, .month = 1, .year = 2000}).earned, 6.0);
            ASSERT_EQUAL(pb._budjet.at(Date{.day = 2, .month = 1, .year = 2000}).earned, 6.0);
            ASSERT_EQUAL(pb._budjet.at(Date{.day = 3, .month = 1, .year = 2000}).earned, 6.0);
        }
        {
            PersonalBudjet pb;
            pb.Earn(
                Date{.day = 1, .month = 1, .year = 2000},
                Date{.day = 3, .month = 1, .year = 2000},
                8.4
            );
            double value = pb._budjet.at(Date{.day = 1, .month = 1, .year = 2000}).earned;
            ASSERT(value > 2.7 and value < 2.9);
            value = pb._budjet.at(Date{.day = 2, .month = 1, .year = 2000}).earned;
            ASSERT(value > 2.7 and value < 2.9);
            value = pb._budjet.at(Date{.day = 3, .month = 1, .year = 2000}).earned;
            ASSERT(value > 2.7 and value < 2.9);
        }
    }

    static void TestSpend()
    {
        {
            PersonalBudjet pb;
            Date date{.day = 1, .month = 1, .year = 2000};
            pb.Earn(date, date, 10.0);
            pb.Spend(date, date, 10.0);
            int income = pb.ComputeIncome(date, date);
            ASSERT_EQUAL(income, 0.0);
        }
        {
            PersonalBudjet pb;
            Date date{.day = 1, .month = 1, .year = 2000};
            pb.Earn(date, date, 10.0);
            pb.Spend(date, date, 4.0);
            int income = pb.ComputeIncome(date, date);
            ASSERT_EQUAL(income, 6.0);
            pb.Earn(date, date, 5.0);
            income = pb.ComputeIncome(date, date);
            ASSERT_EQUAL(income, 11.0);
            pb.Spend(date, date, 2.0);
            income = pb.ComputeIncome(date, date);
            ASSERT_EQUAL(income, 9.0);
        }
        {
            PersonalBudjet pb;
            Date date{.day = 1, .month = 1, .year = 2000};
            pb.Spend(date,date, 4.0);
            int income = pb.ComputeIncome(date, date);
            ASSERT_EQUAL(income, -4.0);
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
                      Date{.day = 1, .month = 1, .year = 2000}, 13);

            double value = pb._budjet.at(Date{.day = 1, .month = 1, .year = 2000}).earned;
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
                      Date{.day = 3, .month = 1, .year = 2000}, 13);

            double value = pb._budjet.at(Date{.day = 1, .month = 1, .year = 2000}).earned;
            ASSERT(value > 2.6 and value < 2.62);

            pb.PayTax(Date{.day = 1, .month = 1, .year = 2000},
                      Date{.day = 3, .month = 1, .year = 2000}, 13);

            value = pb._budjet.at(Date{.day = 1, .month = 1, .year = 2000}).earned;
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
                      Date{.day = 2, .month = 1, .year = 2001}, 13);

            income = pb.ComputeIncome(Date{.day = 1, .month = 1, .year = 2000},
                                      Date{.day = 1, .month = 1, .year = 2000});
            ASSERT(income > 7.82 and income < 7.84);

            income = pb.ComputeIncome(Date{.day = 1, .month = 1, .year = 2000},
                                      Date{.day = 2, .month = 1, .year = 2000});
            ASSERT(income > 15.65 and income < 15.67);

            pb.PayTax(Date{.day = 1, .month = 1, .year = 2000},
                      Date{.day = 2, .month = 1, .year = 2001}, 13);

            income = pb.ComputeIncome(Date{.day = 1, .month = 1, .year = 2000},
                                      Date{.day = 2, .month = 1, .year = 2000});
            ASSERT(income > 13.61 and income < 13.63);

            pb.PayTax(Date{.day = 1, .month = 1, .year = 1999},
                      Date{.day = 31, .month = 12, .year = 1999}, 13);

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
                      Date{.day = 1, .month = 1, .year = 2099}, 13);

            income = pb.ComputeIncome(Date{.day = 1, .month = 1, .year = 2000},
                                      Date{.day = 1, .month = 1, .year = 2099});
            ASSERT(income > 434.0 and income < 436.0);

            pb.PayTax(Date{.day = 1, .month = 1, .year = 2000},
                      Date{.day = 1, .month = 1, .year = 2099}, 13);

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
                      Date{.day = 2, .month = 1, .year = 2000}, 13);
            pb.PayTax(Date{.day = 1, .month = 1, .year = 2000},
                      Date{.day = 1, .month = 1, .year = 2000}, 13);
            pb.PayTax(Date{.day = 5, .month = 1, .year = 2000},
                      Date{.day = 5, .month = 1, .year = 2000}, 13);
            pb.PayTax(Date{.day = 5, .month = 1, .year = 2000},
                      Date{.day = 5, .month = 1, .year = 2099}, 13);

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
                      Date{.day = 1, .month = 1, .year = 2099}, 13);
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
                      Date{.day = 31, .month = 12, .year = 2099}, 13);
            income = pb.ComputeIncome(Date{.day = 1, .month = 1, .year = 2000},
                                      Date{.day = 1, .month = 1, .year = 2000});
            ASSERT_EQUAL(income, 0.87);
        }
    }

    static void TestComputeIncomeFromValueStat()
    {
        PersonalBudjet pb;

        {
            PersonalBudjet::ValueStat from{.earned = 5, .partial_sum = 5};
            PersonalBudjet::ValueStat to{.earned = 5, .partial_sum = 10};
            ASSERT_EQUAL(pb.ComputeIncome(from, to), 10);
        }
        {
            PersonalBudjet::ValueStat from{.earned = 5, .partial_sum = 5};
            PersonalBudjet::ValueStat to{.earned = 5, .partial_sum = 5};
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

            pb.Earn(date1, date1, 5);
            pb.UpdatePartialSum(pb._budjet);
            ASSERT_EQUAL(pb._budjet[date1].partial_sum, 0);

            pb.Earn(date2, date2, 10);
            pb.UpdatePartialSum(pb._budjet);
            ASSERT_EQUAL(pb._budjet[date1].partial_sum, 0);
            ASSERT_EQUAL(pb._budjet[date2].partial_sum, 5);

            pb.Earn(date3, date3, 15);
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
    {
        istringstream iss(R"(
            8
            Earn 2000-01-02 2000-01-06 20
            ComputeIncome 2000-01-01 2001-01-01
            PayTax 2000-01-02 2000-01-03 13
            ComputeIncome 2000-01-01 2001-01-01
            Earn 2000-01-03 2000-01-03 10
            ComputeIncome 2000-01-01 2001-01-01
            PayTax 2000-01-03 2000-01-03 13
            ComputeIncome 2000-01-01 2001-01-01
        )");

        ostringstream oss;

        ProcessQuery(iss, oss);

        string expect = "20\n18.96000000000000085265128\n28.96000000000000085265128\n27.20759999999999934061634\n";

        ASSERT_EQUAL(oss.str(), expect);
    }
    {
        istringstream iss(R"(
            8
            Earn 2000-01-02 2000-01-06 20
            ComputeIncome 2000-01-01 2001-01-01
            PayTax 2000-01-02 2000-01-03 13
            ComputeIncome 2000-01-01 2001-01-01
            Spend 2000-12-30 2001-01-02 14
            ComputeIncome 2000-01-01 2001-01-01
            PayTax 2000-12-30 2000-12-30 13
            ComputeIncome 2000-01-01 2001-01-01
        )");

        ostringstream oss;

        ProcessQuery(iss, oss);

        string expect = "20\n18.96000000000000085265128\n8.460000000000000852651283\n8.460000000000000852651283\n";

        ASSERT_EQUAL(oss.str(), expect);
    }
}

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, TestProcessQuery);
    RUN_TEST(tr, TestDateAddDay);
    RUN_TEST(tr, PersonalBudjetTester::TestEarn);
    RUN_TEST(tr, PersonalBudjetTester::TestSpend);
    RUN_TEST(tr, PersonalBudjetTester::TestPayTax);
    RUN_TEST(tr, PersonalBudjetTester::TestComputeIncome);
    RUN_TEST(tr, PersonalBudjetTester::TestComputeIncomeFromValueStat);
    RUN_TEST(tr, PersonalBudjetTester::TestUpdatePartialSum);
}

void Profile()
{
}