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

class Person
{
public:
    void ChangeFirstName(int year, const string &first_name)
    {
        _first_names[year] = first_name;
    }

    void ChangeLastName(int year, const string &last_name)
    {
        _last_names[year] = last_name;
    }

    string GetFullName(int year) const
    {
        auto first_name_year = findNearlyYear(year, _first_names);
        auto last_name_year = findNearlyYear(year, _last_names);

        if (not first_name_year and not last_name_year)
        {
            return "Incognito";
        }
        if (first_name_year and not last_name_year)
        {
            return _first_names.at(first_name_year.value()) + " with unknown last name";
        }
        else if (last_name_year and not first_name_year)
        {
            return _last_names.at(last_name_year.value()) + " with unknown first name";
        }

        return _first_names.at(first_name_year.value()) + " " + _last_names.at(last_name_year.value());
    }

private:
    map<int, string> _first_names;
    map<int, string> _last_names;

    /**
     * findNearlyYear
     * * return bigger or same year if exists
     */
    std::optional<int> findNearlyYear(int year, const map<int, string> &names) const noexcept
    {
        auto iter_after = names.upper_bound(year);
        if (iter_after != names.begin())
        {
            return prev(iter_after)->first;
        }
        return std::nullopt;
    }
};

void TestAll()
{
    TestRunner tr = {};
}

int main()
{
    TestAll();

    Person person;

    for (int year : {1900, 1965, 1990})
    {
        cout << person.GetFullName(year) << endl;
    }

    person.ChangeFirstName(1965, "Polina");
    person.ChangeLastName(1967, "Sergeeva");
    for (int year : {1900, 1965, 1990})
    {
        cout << person.GetFullName(year) << endl;
    }

    person.ChangeFirstName(1970, "Appolinaria");
    for (int year : {1969, 1970})
    {
        cout << person.GetFullName(year) << endl;
    }

    person.ChangeLastName(1968, "Volkova");
    for (int year : {1969, 1970})
    {
        cout << person.GetFullName(year) << endl;
    }
    return 0;
}
