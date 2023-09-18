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

class PersonalBudjet
{
public:
    double ComputeIncome(int year_from, int month_from, int day_from,
                         int year_to, int month_to, int day_to) const
    {
        time_t date_begin = MakeTime(year_from, month_from, day_from);
        time_t date_end = MakeTime(year_to, month_to, day_to + 1);

        double sum = 0.0;
        for (auto date = date_begin; date != date_end; date += OneDay)
        {
            sum = accumulate(_budjet.begin(), _budjet.end(), sum, [&, date](double value, const auto &budjet)
            {
                if (date >= budjet.first[0] and date <= budjet.first[1])
                {
                    return value + budjet.second;
                }
                return value;
            });
        }
        return sum;
    }

    void Earn(int year_from, int month_from, int day_from,
              int year_to, int month_to, int day_to,
              double value)
    {
        time_t date_from = MakeTime(year_from, month_from, day_from);
        time_t date_to = MakeTime(year_to, month_to, day_to);

        int days = CountDays(date_from, date_to);
        double value_for_day = value / days;

        _budjet[{date_from, date_to}] += value_for_day;
    }

// private:
    map<vector<time_t>, double> _budjet{};

    static constexpr time_t OneDay = 60 * 60 * 24;

    time_t MakeTime(int year, int month, int day) const
    {
        time_t empty_time{};
        tm *ltm = localtime(&empty_time);
        ltm->tm_year = year - 1900;
        ltm->tm_mon = month - 1;
        ltm->tm_mday = day;
        return mktime(ltm);
    }

    int CountDays(time_t date_from, time_t date_to)
    {
        return ((date_to - date_from) / OneDay) + 1;
    }
};

void TestAll()
{
    TestRunner tr = {};
    // tr.RunTest(TestOperatorDateLess, "TestOperatorDateLess");
}

int main()
{
    TestAll();

    cout << fixed << setprecision(25);

    PersonalBudjet pb{};
    int Q = 0;
    cin >> Q;

    for (int i = 0; i < Q; ++i)
    {
        string req{};
        cin >> req;

        int year_from = 0;
        unsigned int month_from = 0;
        unsigned int day_from = 0;
        int year_to = 0;
        unsigned int month_to = 0;
        unsigned int day_to = 0;

        char separ{};

        cin >> year_from >> separ >> month_from >> separ >> day_from >>
            year_to >> separ >> month_to >> separ >> day_to;

        if (req == "Earn")
        {
            unsigned int value = 0;
            cin >> value;
            pb.Earn(year_from, month_from, day_from,
                    year_to, month_to, day_to,
                    value);
        }
        else if (req == "ComputeIncome")
        {
            cout << pb.ComputeIncome(year_from, month_from, day_from,
                                     year_to, month_to, day_to) << endl;
        }
    }
    return 0;
}
