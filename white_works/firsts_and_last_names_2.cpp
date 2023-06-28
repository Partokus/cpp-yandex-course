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
    void ChangeFirstName(int year, const string &first_name)
    {
        auto lowerYear = findLowerYear(year);

        _yearToName[year].first_name = first_name;
        _yearToName[year].is_first_name_changed = true;

        if ((lowerYear.has_value() and lowerYear.value() != year) and !_yearToName.empty())
        {
            _yearToName[year].last_name = _yearToName[lowerYear.value()].last_name;
            for (auto i = ++_yearToName.find(year); i != _yearToName.end(); ++i)
            {
                if (!i->second.is_first_name_changed)
                {
                    i->second.first_name = first_name;
                }
                else
                {
                    break;
                }
            }
        }
    }

    void ChangeLastName(int year, const string &last_name)
    {
        auto lowerYear = findLowerYear(year);

        _yearToName[year].is_last_name_changed = true;
        _yearToName[year].last_name = last_name;

        if ((lowerYear.has_value() and lowerYear.value() != year) and !_yearToName.empty())
        {
            _yearToName[year].first_name = _yearToName[lowerYear.value()].first_name;
            _yearToName[year].last_name = last_name;
            for (auto i = ++_yearToName.find(year); i != _yearToName.end(); ++i)
            {
                if (!i->second.is_last_name_changed)
                {
                    i->second.last_name = last_name;
                }
                else
                {
                    break;
                }
            }
        }
    }

    string GetFullName(int year)
    {
        auto lowerYear = findLowerYear(year);

        if (lowerYear.has_value())
        {
            if (_yearToName[lowerYear.value()].first_name != "" && _yearToName[lowerYear.value()].last_name != "")
            {
                return _yearToName[lowerYear.value()].first_name + " " + _yearToName[lowerYear.value()].last_name;
            }
            else if (_yearToName[lowerYear.value()].first_name != "")
            {
                return _yearToName[lowerYear.value()].first_name + " with unknown last name";
            }
            else
            {
                return _yearToName[lowerYear.value()].last_name + " with unknown first name";
            }
        }
        else
        {
            return "Incognito";
        }
    }

    string GetFullNameWithHistory(int year)
    {
        auto lowerYear = findLowerYear(year);

        if (lowerYear.has_value())
        {
            auto full_name_history = getFullNameHistory(_yearToName.find(lowerYear.value()));

            if (_yearToName[lowerYear.value()].first_name != "" && _yearToName[lowerYear.value()].last_name != "")
            {
                Name result = {_yearToName[lowerYear.value()].first_name + " ", _yearToName[lowerYear.value()].last_name};
                if (!full_name_history.first_name.empty())
                {
                    result.first_name += full_name_history.first_name + " ";
                }
                if (!full_name_history.last_name.empty())
                {
                    result.last_name += " " + full_name_history.last_name;
                }
                return result.first_name + result.last_name;
            }
            else if (_yearToName[lowerYear.value()].first_name != "")
            {
                if (full_name_history.first_name.empty())
                {
                    return _yearToName[lowerYear.value()].first_name + " with unknown last name";
                }
                else
                {
                    return _yearToName[lowerYear.value()].first_name + " " + full_name_history.first_name + " with unknown last name";
                }
            }
            else
            {
                if (full_name_history.last_name.empty())
                {
                    return _yearToName[lowerYear.value()].last_name + " with unknown first name";
                }
                else
                {
                    return _yearToName[lowerYear.value()].last_name + " " + full_name_history.last_name + " with unknown first name";
                }
            }
        }
        else
        {
            return "Incognito";
        }
    }

private:
    struct Name
    {
        string first_name{};
        string last_name{};
        bool is_first_name_changed{};
        bool is_last_name_changed{};
    };
    map<int, Name> _yearToName{};

    string getFullNameHistory(int year, const map<int, string> &name)
    {
        string result{"("};
        string current_name{name.at(year)};

        name.
        for (auto i = prev(year); i != prev(_yearToName.begin()); --i)
        {
            if (i->second.first_name == "" && i->second.last_name == "")
            {
                break;
            }
            if (i->second.first_name != current_name.first_name && i->second.first_name != "")
            {
                result.first_name += i->second.first_name + ", ";
                current_name.first_name = i->second.first_name;
            }
            if (i->second.last_name != current_name.last_name && i->second.last_name != "")
            {
                result.last_name += i->second.last_name + ", ";
                current_name.last_name = i->second.last_name;
            }
        }

        if (result.first_name == "(")
        {
            result.first_name.erase();
        }
        else
        {
            // clear ", " and add ")"
            result.first_name.erase(prev(result.first_name.end()));
            result.first_name.erase(prev(result.first_name.end()));
            result.first_name += ")";
        }

        if (result.last_name == "(")
        {
            result.last_name.erase();
        }
        else
        {
            // clear ", " and add ")"
            result.last_name.erase(prev(result.last_name.end()));
            result.last_name.erase(prev(result.last_name.end()));
            result.last_name += ")";
        }
        return result;
    }

    /**
     * findLowerYear
     * * return lower or same year if exists
    */
    std::optional<const int> findLowerYear(int year) noexcept
    {
        if (_yearToName.empty())
        {
            return std::nullopt;
        }
        if (_yearToName.count(year))
        {
            // return same year or lower
            return _yearToName.lower_bound(year)->first;
        }
        else
        {
            // return lower year or nullopt
            auto lowerYear = prev(_yearToName.lower_bound(year))->first;
            if (lowerYear < year)
            {
                return lowerYear;
            }
        }
        return std::nullopt;
    }

};

int main()
{
    Person person;

    person.ChangeFirstName(1965, "Polina");
    person.ChangeLastName(1967, "Sergeeva");
    for (int year : {1900, 1965, 1990})
    {
        cout << person.GetFullNameWithHistory(year) << endl;
    }

    person.ChangeFirstName(1970, "Appolinaria");
    for (int year : {1969, 1970})
    {
        cout << person.GetFullNameWithHistory(year) << endl;
    }

    person.ChangeLastName(1968, "Volkova");
    for (int year : {1969, 1970})
    {
        cout << person.GetFullNameWithHistory(year) << endl;
    }

    person.ChangeFirstName(1990, "Polina");
    person.ChangeLastName(1990, "Volkova-Sergeeva");
    cout << person.GetFullNameWithHistory(1990) << endl;

    person.ChangeFirstName(1966, "Pauline");
    cout << person.GetFullNameWithHistory(1966) << endl;

    person.ChangeLastName(1960, "Sergeeva");
    for (int year : {1960, 1967})
    {
        cout << person.GetFullNameWithHistory(year) << endl;
    }

    person.ChangeLastName(1961, "Ivanova");
    cout << person.GetFullNameWithHistory(1967) << endl;

    return 0;
}
