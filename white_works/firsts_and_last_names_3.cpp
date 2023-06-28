#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <optional>

using namespace std;

class Person
{
public:
    Person(const string &first_name, const string &last_name, int bornYear)
    {
        _bornYear = bornYear;
        ChangeFirstName(bornYear, first_name);
        ChangeLastName(bornYear, last_name);
    }

    void ChangeFirstName(int year, const string &first_name)
    {
        if (!isValid(year))
        {
            return;
        }
        _first_names[year] = first_name;
    }

    void ChangeLastName(int year, const string &last_name)
    {
        if (!isValid(year))
        {
            return;
        }
        _last_names[year] = last_name;
    }

    string GetFullName(int year) const
    {
        if (!isValid(year))
        {
            return "No person";
        }

        auto first_name_year = findNearlyYear(year, _first_names);
        auto last_name_year = findNearlyYear(year, _last_names);

        return _first_names.at(first_name_year) + " " + _last_names.at(last_name_year);
    }

    string GetFullNameWithHistory(int year) const
    {
        if (!isValid(year))
        {
            return "No person";
        }

        auto first_name_year = findNearlyYear(year, _first_names);
        auto last_name_year = findNearlyYear(year, _last_names);

        auto first_name_history = getFullNameHistory(first_name_year, _first_names);
        auto last_name_history = getFullNameHistory(last_name_year, _last_names);

        if (!first_name_history.empty() && !last_name_history.empty())
        {
            return _first_names.at(first_name_year) + " " + first_name_history + " " + _last_names.at(last_name_year) + " " + last_name_history;
        }
        else if (!first_name_history.empty())
        {
            return _first_names.at(first_name_year) + " " + first_name_history + " " + _last_names.at(last_name_year);
        }
        else if (!last_name_history.empty())
        {
            return _first_names.at(first_name_year) + " " + _last_names.at(last_name_year) + " " + last_name_history;
        }
        return _first_names.at(first_name_year) + " " + _last_names.at(last_name_year);
    }

private:
    map<int, string> _first_names;
    map<int, string> _last_names;
    int _bornYear;

    string getFullNameHistory(int year, const map<int, string> &name) const
    {
        string result{};
        string current_name = name.at(year);
        bool isHistoryExists{};

        // если год явяется нулевым элементом в контейнере
        if (name.find(year) == name.cbegin())
        {
            return "";
        }

        // идёт по контейнеру от текущего года вниз
        auto i = name.find(year);
        do
        {
            --i;
            if (i->second != current_name)
            {
                if (!isHistoryExists)
                {
                    isHistoryExists = true;
                    result += "(";
                }
                result += i->second;
                current_name = i->second;

                if (i != name.begin())
                {
                    result += ", ";
                }
                else
                {
                    result += ")";
                }
            }
        } while (i != name.cbegin());

        return result;
    }

    /**
     * findNearlyYear
     * * return lower or same year if exists
     */
    int findNearlyYear(int year, const map<int, string> &names) const noexcept
    {
        int result{};

        for (const auto &[key, value] : names)
        {
            if (year < key)
            {
                break;
            }
            result = key;
        }
        return result;
    }

    bool isValid(int year) const noexcept
    {
        return year >= _bornYear;
    }
};

int main()
{
    Person person("Polina", "Sergeeva", 1960);
    for (int year : {1959, 1960})
    {
        cout << person.GetFullNameWithHistory(year) << endl;
    }

    person.ChangeFirstName(1965, "Appolinaria");
    person.ChangeLastName(1967, "Ivanova");
    for (int year : {1965, 1967})
    {
        cout << person.GetFullNameWithHistory(year) << endl;
    }

    person.ChangeFirstName(1969, "Sveta");
    person.ChangeLastName(1970, "Vafelnaya");
    for (int year : {1969, 1970})
    {
        cout << person.GetFullNameWithHistory(year) << endl;
    }
    return 0;
}
