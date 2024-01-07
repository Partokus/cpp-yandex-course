#include <test_runner.h>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <optional>
#include <tuple>
#include <chrono>

using namespace std;

class PersonalBudjet
{
public:
    double ComputeIncome(const std::chrono::year year_from, const std::chrono::month month_from, const std::chrono::day day_from,
                         const std::chrono::year year_to, const std::chrono::month month_to, const std::chrono::day day_to) const
    {
        const auto date_begin = MakeTimePoint(year_from, month_from, day_from);
        const auto date_end = MakeTimePoint(year_to, month_to, day_to) + std::chrono::days{1};
        return accumulate(_dates.lower_bound(date_begin), _dates.find(date_end), 0.0, [](double value, const auto &my_pair)
                          { return my_pair.second + value; });
    }

    void Earn(const std::chrono::year year_from, const std::chrono::month month_from, const std::chrono::day day_from,
              const std::chrono::year year_to, const std::chrono::month month_to, const std::chrono::day day_to,
              double value)
    {
        auto date = MakeTimePoint(year_from, month_from, day_from);
        const auto date_end = MakeTimePoint(year_to, month_to, day_to) + std::chrono::days{1};

        const auto days_count = std::chrono::duration_cast<std::chrono::days>(date_end - date).count();
        const double value_for_day = value / days_count;

        while (date != date_end)
        {
            _dates[date] += value_for_day;
            date += std::chrono::days{1};
        }
    }

private:
    map<std::chrono::system_clock::time_point, double> _dates{};

    std::chrono::system_clock::time_point MakeTimePoint(std::chrono::year year,
                                                        std::chrono::month month,
                                                        std::chrono::day day) const
    {
        const std::chrono::year_month_day ymd{year, month, day};
        return std::chrono::sys_days{ymd};
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

    using namespace std::chrono;

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
            pb.Earn(year{year_from}, month{month_from}, day{day_from},
                    year{year_to}, month{month_to}, day{day_to},
                    value);
        }
        else if (req == "ComputeIncome")
        {
            cout << pb.ComputeIncome(year{year_from}, month{month_from}, day{day_from},
                                     year{year_to}, month{month_to}, day{day_to}) << endl;
        }
    }
    return 0;
}
