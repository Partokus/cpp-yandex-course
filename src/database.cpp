#include "database.h"

using namespace std;

void Database::Add(const Date &date, const string &event)
{
    _date_and_events[date].push_back(event);
}

unsigned int Database::RemoveIf(const Predicate &predicate)
{
    unsigned int count = 0;
    for (auto &[date, events] : _date_and_events)
    {
        unsigned int elem_index = 0;
        for (auto &event : events)
        {
            if (predicate(date, event))
            {
                _date_and_events[date].erase(_date_and_events[date].cbegin() + elem_index++);
                if (_date_and_events[date].empty())
                {
                    _date_and_events.erase(date);
                }
                ++count;
            }
        }
    }
    return count;
}

vector<std::string> Database::FindIf(const Predicate &predicate) const
{
    return {};
}

std::string Database::Last(const Date &date) const
{
    return {};
}

void Database::Print(ostream &os) const
{
    for (const auto &[date, events] : _date_and_events)
    {
        for (const auto &event : events)
        {
            os << setfill('0')
                << setw(4) << date.year << "-"
                << setw(2) << date.mounth << "-"
                << setw(2) << date.day << " "
                << event << endl;
        }
    }
}

// bool Database::DeleteEvent(const Date &date, const string &event)
// {
//     // если событие существует
//     if (_date_and_events.count(date) and _date_and_events.at(date).count(event))
//     {
//         _date_and_events[date].erase(event);
//         if (_date_and_events.at(date).empty())
//         {
//             _date_and_events.erase(date);
//         }
//         return true;
//     }
//     return false;
// }

// int Database::DeleteDate(const Date &date)
// {
//     int event_number = 0;
//     if (_date_and_events.count(date))
//     {
//         event_number = _date_and_events.at(date).size();
//         _date_and_events.erase(date);
//     }
//     return event_number;
// }

// void Database::Find(const Date &date) const
// {
//     try
//     {
//         for (const auto &event : _date_and_events.at(date))
//         {
//             cout << event << endl;
//         }
//     }
//     catch (const std::exception &e)
//     {
//         cout << e.what() << endl;
//     }
// }

// void Database::Print(istream &is) const
// {
//     for (const auto &[key, value] : _date_and_events)
//     {
//         for (const auto &event : value)
//         {
//             is << setfill('0') << setw(4) << key.year << "-"
//                << setw(2) << key.mounth << "-"
//                << setw(2) << key.day << " "
//                << event << endl;
//         }
//     }
// }
