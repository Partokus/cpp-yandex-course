#pragma once
#include <iostream>
#include <map>
#include <set>
#include <string>
#include "date.h"

class Database
{
public:
    void Add(const Date &date, const std::string &event);
    RemoveIf
    bool DeleteEvent(const Date &date, const std::string &event);
    int DeleteDate(const Date &date);
    void Find(const Date &date) const;
    void Print(std::istream &is) const;
    int FindIf();
    int RemoveIf();
    std::string Last(const Date &date);
private:
    std::map<Date, std::set<std::string>> _date_and_events{};
    
};