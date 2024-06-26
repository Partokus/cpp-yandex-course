#pragma once
#include "iterator_range.h"
#include <string_view>
#include <sstream>
#include <vector>

using namespace std;

template <typename Container>
string Join(char c, const Container &cont)
{
    ostringstream os;
    for (const auto &item : Head(cont, cont.size() - 1U))
    {
        os << item << c;
    }
    os << *rbegin(cont);
    return os.str();
}

string_view Strip(string_view sv);
vector<string_view> SplitBy(string_view sv, char sep);