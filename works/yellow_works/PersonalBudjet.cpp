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

struct DateRange
{
    time_t date_from{};
    time_t date_to{};
};

bool operator<(const DateRange &lhs, const DateRange &rhs)
{
    if (lhs.date_from < rhs.date_from)
    {
        return true;
    }
    if (lhs.date_from > rhs.date_from)
    {
        return false;
    }
    if (lhs.date_to < rhs.date_to)
    {
        return true;
    }
    return false;
}

struct Date
{
    int year = 0;
    int month = 0;
    int day = 0;
};

bool operator<(const Date &lhs, const Date &rhs)
{
    return make_tuple(lhs.year, lhs.month, lhs.day) < make_tuple(rhs.year, rhs.month, rhs.day);
}

class PersonalBudjet
{
public:
    double ComputeIncome(Date date_from, Date date_to) const
    {
        if (date_to < date_from)
        {
            swap(date_from, date_to);
        }

        const time_t date_begin = MakeTime(date_from);
        const time_t date_end = MakeTime(date_to);

        auto it_date_begin = _budjet.find(date_begin);
        if (it_date_begin == end(_budjet))
        {
            it_date_begin = _budjet.upper_bound(date_begin);
        }

        const auto it_date_end = _budjet.upper_bound(date_end);

        return accumulate(it_date_begin, it_date_end, 0.0, [](const double &value, const auto &my_pair)
        {
            return my_pair.second + value;
        });
    }

    void Earn(Date date_from, Date date_to, const double value)
    {
        if (date_to < date_from)
        {
            swap(date_from, date_to);
        }

        const time_t date_begin = MakeTime(date_from);
        const time_t date_end = MakeTime(date_to) + OneDay;

        const auto days = CountDays(date_begin, date_end - OneDay);
        const double value_for_day = value / days;

        for (auto date = date_begin; date != date_end; date += OneDay)
        {
            _budjet[date] += value_for_day;
        }
    }

private:
    map<time_t, double> _budjet{};

    static constexpr time_t OneDay = 60 * 60 * 24;

    time_t MakeTime(const Date &date) const
    {
        time_t empty_time{};
        tm *ltm = localtime(&empty_time);
        ltm->tm_year = date.year - 1900;
        ltm->tm_mon = date.month - 1;
        ltm->tm_mday = date.day;
        return mktime(ltm);
    }

    unsigned int CountDays(time_t date_time_from, time_t date_time_to)
    {
        return ((date_time_to - date_time_from) / OneDay) + 1;
    }
};

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
        pb.Earn({2000, 1, 15}, {2000, 1, 1}, 65.0);
        pb.Earn({2000, 2, 5}, {2000, 2, 15}, 30.0);
        pb.Earn({2002, 6, 5}, {2002, 6, 20}, 35.0);
        pb.Earn({2065, 6, 20}, {2065, 6, 5}, 40.0);
        double value = 65.0 + 30.0 + 35.0 + 40.0;
        double res = pb.ComputeIncome({2000, 1, 1}, {2099, 1, 1});
        Assert(res >= (value - 0.1) and res <= (value + 0.1), "PersonalBudjet 12");
        value = 65.0 + 30.0 + 35.0;
        res = pb.ComputeIncome({2005, 1, 1}, {2000, 1, 1});
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
        pb.Earn({2099, 12, 31}, {2000, 1, 1}, 1000000.0);
        AssertEqual(pb.ComputeIncome({2099, 12, 31}, {2000, 1, 1}), 999999.9999993942910805345, "PersonalBudjet 19");
    }
    {
        PersonalBudjet pb{};
        pb.Earn({2099, 12, 31}, {2000, 1, 1}, 1000000.0);
        AssertEqual(pb.ComputeIncome({2099, 12, 31}, {2000, 1, 1}), 999999.9999993942910805345, "PersonalBudjet 20");
    }
}

void TestAll()
{
    TestRunner tr = {};
    tr.RunTest(TestPersonalBudjet, "TestPersonalBudjet");
}

int main()
{
    cout.precision(25);

    TestAll();

    PersonalBudjet pb{};
    int Q = 0;
    cin >> Q;

    for (int i = 0; i < Q; ++i)
    {
        string req{};
        cin >> req;

        Date date_from{};
        Date date_to{};

        char separ{};

        cin >> date_from.year >> separ >> date_from.month >> separ >> date_from.day >>
            date_to.year >> separ >> date_to.month >> separ >> date_to.day;

        if (req == "Earn")
        {
            double value = 0.0;
            cin >> value;
            pb.Earn(date_from, date_to, value);
        }
        else if (req == "ComputeIncome")
        {
            cout << pb.ComputeIncome(date_from, date_to) << endl;
        }
    }
    return 0;
}
