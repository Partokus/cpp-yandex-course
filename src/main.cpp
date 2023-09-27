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
    double ComputeIncome(Date date_from, Date date_to)
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

        // if (not _isPartialSumUpdated)
        // {
        //     _isPartialSumUpdated = true;
        //     partial_sum(_budjet.cbegin(), _budjet.cend(), _partial_sums.begin(), [](auto &lhs, auto &rhs)
        //     {
        //         return {};
        //     });
        // }

        return 0.0;
    }

    void Earn(const Date date, const double value)
    {
        _budjet[MakeTime(date)] += value;
    }

// private:
    map<time_t, double> _budjet{};
    map<time_t, double> _budjet_partial_sums{};
    bool _isPartialSumUpdated = false;

    static constexpr time_t OneDay = 60 * 60 * 24;
    static constexpr time_t TimeRangeBegin = 946684800; // 2000-1-1
    static constexpr unsigned int DaysCount = 36525;

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

    unsigned int TimeToIndex(time_t my_time)
    {
        return (my_time - TimeRangeBegin) / OneDay;
    }
};

void TestPersonalBudjet()
{
    {
        PersonalBudjet pb{};
        AssertEqual(pb.TimeToIndex(pb.MakeTime({2000, 1, 1})), 0U, "TimeToIndex 0");
        AssertEqual(pb.TimeToIndex(pb.MakeTime({2000, 1, 2})), 1U, "TimeToIndex 1");
        AssertEqual(pb.TimeToIndex(pb.MakeTime({2099, 12, 31})), PersonalBudjet::DaysCount - 1, "TimeToIndex 3");
        cout << pb.TimeToIndex(pb.MakeTime({2099, 12, 31}));
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
    return 0;

    PersonalBudjet pb{};

    int E = 0;
    cin >> E;

    // E раз добавляем информацию о заработках, например:
    // 2000-01-01 10
    for (int i = 0; i < E; ++i)
    {
        Date date{};
        double value = 0.0;

        char separ{};

        cin >> date.year >> separ >> date.month >> separ >> date.day >> value;
        pb.Earn(date, value);
    }

    int Q = 0;
    cin >> Q;

    map<time_t, double> sums{};

    // Q раз вычисляем прибыль, например:
    // 2000-01-01 2000-01-06
    for (int i = 0; i < Q; ++i)
    {
        Date date_from{};
        Date date_to{};

        char separ{};

        cin >> date_from.year >> separ >> date_from.month >> separ >> date_from.day >>
            date_to.year >> separ >> date_to.month >> separ >> date_to.day;

        cout << pb.ComputeIncome(date_from, date_to) << endl;
    }
    return 0;
}
