#include <test_runner.h>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <optional>
#include <tuple>
#include <ctime>
#include <cstring>
#include <iomanip>
#include <span>

using namespace std;

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

template <class InputIt, class OutputKey, class OutputValue, class BinaryOperator>
void MapPartialSum(InputIt first, InputIt last,
                   map<OutputKey, OutputValue> &dest,
                   BinaryOperator op)
{
    if (first == last)
        return;

    typename InputIt::value_type::second_type sum = first->second;
    dest[first->first] = sum;

    while (++first != last)
    {
        sum = op(sum, first->second);
        dest[first->first] = sum;
    }
}

template <class InputIt, class OutputKey, class OutputValue>
void MapPartialSum(InputIt first, InputIt last,
                   map<OutputKey, OutputValue> &dest)
{
    return MapPartialSum(first, last, dest, std::plus<OutputValue>());
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
        const time_t from = MakeTime(date_from);
        const time_t to = MakeTime(date_to);

        auto it_from = _budjet.find(from);
        if (it_from == _budjet.end())
        {
            it_from = _budjet.upper_bound(from);
        }

        auto it_to = _budjet.find(to);
        if (it_to == _budjet.end())
        {
            it_to = prev(_budjet.lower_bound(to));
        }

        if (not _isPartialSumUpdated)
        {
            _isPartialSumUpdated = true;
            MapPartialSum(_budjet.cbegin(), _budjet.cend(), _budjet);
        }

        const double lhs_sum = it_from == _budjet.cbegin() ? 0.0 : prev(it_from)->second;
        const double rhs_sum = it_to->second;

        return rhs_sum - lhs_sum;
    }

    void Earn(const Date date, const double value)
    {
        _budjet[MakeTime(date)] += value;
    }

private:
    map<time_t, double> _budjet{};
    bool _isPartialSumUpdated = false;

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
