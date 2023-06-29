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

using namespace std;

struct Date
{
    int year = 0;
    int mounth = 0;
    int day = 0;
};
map<Date, set<string>> date_and_events;

void ensureSymbol(istringstream &ss, const string &date_str)
{
    if (ss.peek() != '-')
    {
        throw runtime_error("Wrong date format: " + date_str);
    }
    ss.ignore(1);
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
    ensureSymbol(ss_date, date_str);
    ss_date >> date.mounth;
    ensureSymbol(ss_date, date_str);
    ss_date >> date.day;

    // если строка в конце оказалась не пустая, то ошибка
    if (not ss_date.eof())
    {
        throw runtime_error("wrong format of date: " + date_str);
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

void testDate()
{
    Date date;
    int test_number = 0;

    {
        try
        {
            istringstream ss("1-2-3");
            ss >> date;
        }
        catch (const std::exception &e)
        {
            throw runtime_error("testDate failed: " + to_string(test_number));
        }
        ++test_number;
    }
    {
        try
        {
            istringstream ss("0-2--0");
            ss >> date;
        }
        catch (const std::exception &e)
        {
            string error = "Day value is invalid: 0";
            if (e.what() != error)
            {
                throw runtime_error("testDate failed: " + to_string(test_number));
            }
        }
        ++test_number;
    }
    {
        try
        {
            istringstream ss("-1--2--3");
            ss >> date;
        }
        catch (const std::exception &e)
        {
            string error = "Month value is invalid: -2";
            if (e.what() != error)
            {
                throw runtime_error("testDate failed: " + to_string(test_number));
            }
        }
        ++test_number;
    }
    {
        try
        {
            istringstream ss("2015-12-31");
            ss >> date;
        }
        catch (const std::exception &e)
        {
            throw runtime_error("testDate failed: " + to_string(test_number));
        }
        ++test_number;
    }
    {
        try
        {
            istringstream ss("0001-01-01");
            ss >> date;
        }
        catch (const std::exception &e)
        {
            throw runtime_error("testDate failed: " + to_string(test_number));
        }
        ++test_number;
    }
    {
        try
        {
            istringstream ss("2015-1-1");
            ss >> date;
        }
        catch (const std::exception &e)
        {
            throw runtime_error("testDate failed: " + to_string(test_number));
        }
        ++test_number;
    }
    {
        try
        {
            istringstream ss("--1-2-3");
            ss >> date;
            throw runtime_error("testDate failed: " + to_string(test_number));
        }
        catch (const std::exception &e)
        {
        }
        ++test_number;
    }
    {
        try
        {
            istringstream ss("1---2-3");
            ss >> date;
            throw runtime_error("testDate failed: " + to_string(test_number));
        }
        catch (const std::exception &e)
        {
        }
        ++test_number;
    }
    {
        try
        {
            istringstream ss("1-2---3");
            ss >> date;
            throw runtime_error("testDate failed: " + to_string(test_number));
        }
        catch (const std::exception &e)
        {
        }
        ++test_number;
    }
    {
        try
        {
            istringstream ss("1-2-3-");
            ss >> date;
            throw runtime_error("testDate failed: " + to_string(test_number));
        }
        catch (const std::exception &e)
        {
        }
        ++test_number;
    }
    {
        try
        {
            istringstream ss("1-2-3a");
            ss >> date;
            throw runtime_error("testDate failed: " + to_string(test_number));
        }
        catch (const std::exception &e)
        {
        }
        ++test_number;
    }
    {
        try
        {
            istringstream ss("1b-2-3");
            ss >> date;
            throw runtime_error("testDate failed: " + to_string(test_number));
        }
        catch (const std::exception &e)
        {
        }
        ++test_number;
    }
    {
        try
        {
            istringstream ss("a-b-c");
            ss >> date;
            throw runtime_error("testDate failed: " + to_string(test_number));
        }
        catch (const std::exception &e)
        {
        }
        ++test_number;
    }
    {
        try
        {
            istringstream ss("1-2+3");
            ss >> date;
            throw runtime_error("testDate failed: " + to_string(test_number));
        }
        catch (const std::exception &e)
        {
        }
        ++test_number;
    }
    {
        try
        {
            istringstream ss("---2--3--7");
            ss >> date;
            throw runtime_error("testDate failed: " + to_string(test_number));
        }
        catch (const std::exception &e)
        {
        }
        ++test_number;
    }
    {
        try
        {
            istringstream ss("-----2-3-7");
            ss >> date;
            throw runtime_error("testDate failed: " + to_string(test_number));
        }
        catch (const std::exception &e)
        {
        }
        ++test_number;
    }
    {
        try
        {
            istringstream ss("2015-0-1");
            ss >> date;
            throw runtime_error("testDate failed: " + to_string(test_number));
        }
        catch (const std::exception &e)
        {
        }
        ++test_number;
    }
    {
        try
        {
            istringstream ss("2015-1-0");
            ss >> date;
            throw runtime_error("testDate failed: " + to_string(test_number));
        }
        catch (const std::exception &e)
        {
        }
        ++test_number;
    }
    {
        try
        {
            istringstream ss("2015-13-1");
            ss >> date;
            throw runtime_error("testDate failed: " + to_string(test_number));
        }
        catch (const std::exception &e)
        {
        }
        ++test_number;
    }
    {
        try
        {
            istringstream ss("2015-12-32");
            ss >> date;
            throw runtime_error("testDate failed: " + to_string(test_number));
        }
        catch (const std::exception &e)
        {
        }
        ++test_number;
    }
    {
        try
        {
            istringstream ss("2015-12--31");
            ss >> date;
            throw runtime_error("testDate failed: " + to_string(test_number));
        }
        catch (const std::exception &e)
        {
        }
        ++test_number;
    }
}

void deleteEventIfExists(Date &date, const string &event)
{
    // если событие существует
    if (date_and_events.at(date).count(event))
    {
        date_and_events[date].erase(event);
        cout << "Deleted successfully" << endl;
    }
    else
    {
        cout << "Event not found" << endl;
    }
}

void deleteAllEvents(Date &date)
{
    if (date_and_events.count(date) and not date_and_events.at(date).empty())
    {
        size_t event_number = date_and_events.at(date).size();
        date_and_events[date].clear();
        cout << "Deleted " << event_number << " events" << endl;
    }
    else
    {
        cout << "Deleted 0 events" << endl;
    }
}

void printAll()
{
    for (const auto date : date_and_events)
}

void run()
{
    bool test = false;
    if (test)
    {
        testDate();
        cout << "testDate OK" << endl;
        throw runtime_error("TESTS OK");
    }

    string line;
    Date date;

    getline(cin, line);

    if (line.empty())
    {
        return;
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

        date_and_events[date].insert(event);
    }
    else if (command == "Del")
    {
        ss >> date;

        string event;
        getline(ss, event);
        if (not event.empty())
        {
            deleteEventIfExists(date, event);
        }
        else
        {
            deleteAllEvents(date);
        }
    }
    else if (command == "Find")
    {
        ss >> date;

        for (const auto &event : date_and_events.at(date))
        {
            cout << event << endl;
        }
    }
    else if (command == "Print")
    {
        printAll();
    }
    else
    {
        throw runtime_error("Unknown command: " + command);
    }
}

int main()
{
    try
    {
        while (true)
        {
            run();
        }
    }
    catch (const exception &e)
    {
        cout << e.what() << endl;
    }
    return 0;
}
