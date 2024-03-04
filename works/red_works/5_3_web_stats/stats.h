#pragma once
#include "http_request.h"
#include <string_view>
#include <string>
#include <map>
#include <set>

using namespace std;

class Stats
{
public:
    Stats();

    void AddMethod(string_view method);
    void AddUri(string_view uri);
    const map<string_view, int> &GetMethodStats() const;
    const map<string_view, int> &GetUriStats() const;

private:
    map<string_view, int> _method_stats;
    map<string_view, int> _uri_stats;
    const set<string> Methods = {"GET", "POST", "PUT", "DELETE", "UNKNOWN"};
    const set<string> Uries = {"/", "/order", "/product", "/basket", "/help", "unknown"};
};

HttpRequest ParseRequest(string_view line);
