#pragma once
#include <iostream>
#include <exception>
#include <string>
#include <iomanip>

struct Date
{
    int year = 0;
    int mounth = 0;
    int day = 0;
};

Date ParseDate(std::istream &is);
bool operator<(const Date &lhs, const Date &rhs);
bool operator==(const Date &lhs, const Date &rhs);
std::ostream &operator<<(std::ostream &os, const Date &date);