#include "database.h"

#include <sstream>

using namespace std;

void Database::Add(const Date &date, const string &event)
{
    auto &events = _date_and_events[date];
    if (find(events.cbegin(), events.cend(), event) == events.cend())
    {
        events.push_back(event);
    }
}

unsigned int Database::RemoveIf(const Predicate &predicate)
{
    unsigned int removed_count = 0;

    for (auto it = _date_and_events.begin(); it != _date_and_events.cend();)
    {
        auto it_begin_remove = remove_if(it->second.begin(), it->second.end(), [&predicate, &it](const string &event)
        {
            return predicate(it->first, event);
        });
        removed_count += it->second.cend() - it_begin_remove;
        if (removed_count != it->second.size())
        {
            it->second.erase(it_begin_remove, it->second.end());
            ++it;
        }
        else
        {
            _date_and_events.erase(it++);
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
    return MakeStr(date_result, *events.crbegin());
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
