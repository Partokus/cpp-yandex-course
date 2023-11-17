#include "database.h"

#include <sstream>

using namespace std;

void Database::Add(const Date &date, const string &event)
{
    if (not _for_check_event_exist[date].count(event))
    {
        _date_and_events[date].push_back(event);
        _for_check_event_exist[date].insert(event);
    }
}

unsigned int Database::RemoveIf(const Predicate &predicate)
{
    unsigned int removed_count = 0;

    for (auto it = _date_and_events.begin(); it != _date_and_events.cend();)
    {
        auto &[date, events] = *it;

        for (auto it_event = events.begin(); it_event != events.end();)
        {
            const auto &event = *it_event;

            if (predicate(date, event))
            {
                _for_check_event_exist[date].erase(event);
                events.erase(it_event);
                ++removed_count;
            }
            else
            {
                ++it_event;
            }
        }

        if (events.empty())
        {
            _date_and_events.erase(it++);
        }
        else
        {
            ++it;
        }
    }
    return removed_count;
}

vector<string> Database::FindIf(const Predicate &predicate) const
{
    vector<string> date_and_event{};
    for (const auto &[date, events] : _date_and_events)
    {
        for (const auto &event : events)
        {
            if (predicate(date, event))
            {
                date_and_event.push_back(MakeStr(date, event));
            }
        }
    }
    return date_and_event;
}

string Database::Last(const Date &date) const
{
    if (_date_and_events.empty() or date < _date_and_events.cbegin()->first)
    {
        throw std::invalid_argument("No entries");
    }
    const auto it = prev(_date_and_events.upper_bound(date));

    const auto &[date_result, events] = *it;
    const auto &event_result = *events.crbegin();

    return MakeStr(date_result, event_result);
}

void Database::Print(ostream &os) const
{
    for (const auto &[date, events] : _date_and_events)
    {
        for (const auto &event : events)
        {
            os << date << " " << event << endl;
        }
    }
}

string Database::MakeStr(const Date &date, const string &event) const
{
    stringstream ss{};
    ss << date << " " << event;
    return ss.str();
}
