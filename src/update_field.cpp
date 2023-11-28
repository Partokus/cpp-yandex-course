#include "airline_ticket.h"
#include "test_runner.h"

#include <algorithm>
#include <numeric>
#include <tuple>
#include <iostream>
#include <iomanip>

using namespace std;

inline static constexpr string WordTime = "time";

#define UPDATE_FIELD(ticket, field, values) \
{ \
    auto it = values.find( #field )

    if (it != values.cend())
    {
        auto [field_name, field_value] = *it;

        if (field_name == "from" or  field_name == "to" or  field_name == "airline")
        {
            ticket.field = field_value;
        }
        else if (field_name == "price")
        {
            ticket.price = stoi(field_value);
        }
        else if (search(field_name.crbegin(), field_name.crend(), WordTime.crbegin(), WordTime.crend()))
        {
            istringstream is(field_value);
            is >> ticket.field.hours >> ticket.field.minutes;
        }
        else
        {
            istringstream is(field_value);
            is >> ticket.field.year >> ticket.field.month >> ticket.field.day;
        }
    }
} \

void UpdateTicket(AirlineTicket &ticket, const map<string, string> &updates)
{
    UPDATE_FIELD(ticket, to, updates);
    UPDATE_FIELD(ticket, from, updates);
    UPDATE_FIELD(ticket, price, updates);
    UPDATE_FIELD(ticket, airline, updates);
}

void TestUpdate()
{
    AirlineTicket t;
    t.price = 0;

    const map<string, string> updates1 = {
        {"departure_date", "2018-2-28"},
        {"departure_time", "17:40"},
    };
    UPDATE_FIELD(t, departure_date, updates1);
    UPDATE_FIELD(t, departure_time, updates1);
    UPDATE_FIELD(t, price, updates1);

    ASSERT_EQUAL(t.departure_date, (Date{2018, 2, 28}));
    ASSERT_EQUAL(t.departure_time, (Time{17, 40}));
    ASSERT_EQUAL(t.price, 0);

    const map<string, string> updates2 = {
        {"price", "12550"},
        {"arrival_time", "20:33"},
    };
    UPDATE_FIELD(t, departure_date, updates2);
    UPDATE_FIELD(t, departure_time, updates2);
    UPDATE_FIELD(t, arrival_time, updates2);
    UPDATE_FIELD(t, price, updates2);

    // updates2 не содержит ключей "departure_date" и "departure_time", поэтому
    // значения этих полей не должны измениться
    ASSERT_EQUAL(t.departure_date, (Date{2018, 2, 28}));
    ASSERT_EQUAL(t.departure_time, (Time{17, 40}));
    ASSERT_EQUAL(t.price, 12550);
    ASSERT_EQUAL(t.arrival_time, (Time{20, 33}));
}

int main()
{
    TestRunner tr;
    RUN_TEST(tr, TestUpdate);
}

bool operator<(const Date &lhs, const Date &rhs)
{
    return std::tie(lhs.year, lhs.month, lhs.day) < std::tie(rhs.year, rhs.month, rhs.day);
}

bool operator<(const Time &lhs, const Time &rhs)
{
    return std::tie(lhs.hours, lhs.minutes) < std::tie(rhs.hours, rhs.minutes);
}

bool operator==(const Date &lhs, const Date &rhs)
{
    return std::tie(lhs.year, lhs.month, lhs.day) == std::tie(rhs.year, rhs.month, rhs.day);
}

bool operator==(const Time &lhs, const Time &rhs)
{
    return std::tie(lhs.hours, lhs.minutes) == std::tie(rhs.hours, rhs.minutes);
}

ostream &operator<<(ostream &os, const Date &date)
{
    os << setfill('0')
       << setw(4) << date.year << "-"
       << setw(2) << date.month << "-"
       << setw(2) << date.day;
    return os;
}

ostream &operator<<(ostream &os, const Time &time)
{
    os << setfill('0') << setw(2)
       << time.hours << ":" << time.minutes;
    return os;
}