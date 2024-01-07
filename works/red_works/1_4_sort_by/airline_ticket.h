#pragma once
#include <string>

struct Date
{
    int year = 0;
    int month = 1;
    int day = 1;
};

struct Time
{
    int hours = 0;
    int minutes = 1;
};

struct AirlineTicket
{
    std::string from{};
    std::string to{};
    std::string airline{};
    Date departure_date{};
    Time departure_time{};
    Date arrival_date{};
    Time arrival_time{};
    int price = 0;
};

bool operator<(const Date &lhs, const Date &rhs);
bool operator<(const Time &lhs, const Time &rhs);
bool operator==(const Date &lhs, const Date &rhs);
bool operator==(const Time &lhs, const Time &rhs);

std::ostream &operator<<(std::ostream &os, const Date &date);
std::ostream &operator<<(std::ostream &os, const Time &time);
