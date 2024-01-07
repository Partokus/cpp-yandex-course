#pragma once
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <functional>
#include <vector>
#include <set>

#include "date.h"

using Predicate = std::function<bool(const Date &date, const std::string &event)>;

class Database
{
public:
    void Add(const Date &date, const std::string &event);
    unsigned int RemoveIf(const Predicate &predicate);
    std::vector<std::string> FindIf(const Predicate &predicate) const;
    void Print(std::ostream &os) const;
    std::string Last(const Date &date) const;
private:
    std::map<Date, std::vector<std::string>> _date_and_events{};
    std::map<Date, std::set<std::string>> _for_check_event_exist{};

    std::string MakeStr(const Date &date, const std::string &event) const;
};