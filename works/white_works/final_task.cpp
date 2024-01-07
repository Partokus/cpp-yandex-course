#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <optional>
#include <variant>
#include <cmath>
#include <numeric>
#include <limits>

using namespace std;

struct Date
{
    int year = 0;
    int mounth = 0;
    int day = 0;
};

void ensureSymbol(istringstream &ss, const string &date_str)
{
    if (ss.peek() != '-')
    {
        throw runtime_error("Wrong date format: " + date_str);
    }
    ss.ignore(1);
}

void checkOverflow(istringstream &ss)
{
    if (ss.fail())
    {
        // если появилась ошибка, то игнорим её и идём дальше.
        // если это была ошибка из-за переполнения, то следующим символом должен быть '-', если формат правильный
        ss.clear();
    }
}

void isDateValid(const Date &date)
{
    static constexpr int MinMounth = 1;
    static constexpr int MaxMounth = 12;

    static constexpr int MinDay = 1;
    static constexpr int MaxDay = 31;

    if (date.mounth < MinMounth or date.mounth > MaxMounth)
    {
        throw runtime_error("Month value is invalid: " + to_string(date.mounth));
    }

    if (date.day < MinDay or date.day > MaxDay)
    {
        throw runtime_error("Day value is invalid: " + to_string(date.day));
    }
}

istringstream &operator>>(istringstream &ss, Date &date)
{
    string date_str;

    // получаем полную строку даты, чтобы вывести в случае ошибки
    getline(ss, date_str, ' ');

    // парсим дату
    istringstream ss_date(date_str);
    ss_date >> date.year;
    checkOverflow(ss_date);
    ensureSymbol(ss_date, date_str);
    ss_date >> date.mounth;
    checkOverflow(ss_date);
    ensureSymbol(ss_date, date_str);
    ss_date >> date.day;

    // если считали не int или строка в конце оказалась не пустая, то ошибка
    if ((ss_date.fail() and date.day != numeric_limits<int>::max()) or not ss_date.eof())
    {
        throw runtime_error("Wrong date format: " + date_str);
    }

    isDateValid(date);

    return ss;
}

bool operator<(const Date &lhs, const Date &rhs)
{
    if (lhs.year < rhs.year)
    {
        return true;
    }
    if (lhs.year != rhs.year)
    {
        return false;
    }
    if (lhs.mounth < rhs.mounth)
    {
        return true;
    }
    if (lhs.mounth != rhs.mounth)
    {
        return false;
    }
    if (lhs.day < rhs.day)
    {
        return true;
    }
    return false;
}

struct DataBase
{
    void addEvent(const Date &date, const string &event)
    {
        _date_and_events[date].insert(event);
    }

    bool deleteEvent(const Date &date, const string &event)
    {
        // если событие существует
        if (_date_and_events.count(date) and _date_and_events.at(date).count(event))
        {
            _date_and_events[date].erase(event);
            if (_date_and_events.at(date).empty())
            {
                _date_and_events.erase(date);
            }
            return true;
        }
        return false;
    }

    int deleteDate(const Date &date)
    {
        int event_number = 0;
        if (_date_and_events.count(date))
        {
            event_number = _date_and_events.at(date).size();
            _date_and_events.erase(date);
        }
        return event_number;
    }

    void find(const Date &date) const
    {
        try
        {
            for (const auto &event : _date_and_events.at(date))
            {
                cout << event << endl;
            }
        }
        catch (const std::exception &e)
        {
            cout << e.what() << endl;
        }
    }

    void print() const
    {
        for (const auto &[key, value] : _date_and_events)
        {
            for (const auto &event : value)
            {
                cout << setfill('0') << setw(4) << key.year << "-"
                     << setw(2) << key.mounth << "-"
                     << setw(2) << key.day << " "
                     << event << endl;
            }
        }
    }

private:
    map<Date, set<string>> _date_and_events;
};

int main()
{
    string line;
    DataBase db;
    Date date;

    try
    {
        while (getline(cin, line))
        {
            if (line.empty())
            {
                continue;
            }

            istringstream ss(line);
            string command;
            ss >> command;

            // пропускаем пробел
            ss.ignore(1);

            if (command == "Add")
            {
                ss >> date;

                string event;
                ss >> event;

                db.addEvent(date, event);
            }
            else if (command == "Del")
            {
                ss >> date;

                string event;
                getline(ss, event);

                if (not event.empty())
                {
                    if (db.deleteEvent(date, event))
                    {
                        cout << "Deleted successfully" << endl;
                    }
                    else
                    {
                        cout << "Event not found" << endl;
                    }
                }
                else
                {
                    cout << "Deleted " << db.deleteDate(date) << " events" << endl;
                }
            }
            else if (command == "Find")
            {
                ss >> date;
                db.find(date);
            }
            else if (command == "Print")
            {
                db.print();
            }
            else
            {
                throw runtime_error("Unknown command: " + command);
            }
        }
    }
    catch (const exception &e)
    {
        cout << e.what() << endl;
    }
    return 0;
}
