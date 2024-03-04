#include "stats.h"
#include <vector>

Stats::Stats()
{
    for (const string &method : Methods)
    {
        _method_stats[method] = 0U;
    }
    for (const string &uri : Uries)
    {
        _uri_stats[uri] = 0U;
    }
}

void Stats::AddMethod(string_view method)
{
    string s = {method.cbegin(), method.cend()};
    auto it = Methods.find(s);
    if (it == Methods.end())
    {
        it = Methods.find("UNKNOWN");
    }
    ++_method_stats[*it];
}

void Stats::AddUri(string_view uri)
{
    string s = {uri.cbegin(), uri.cend()};
    auto it = Uries.find(s);
    if (it == Uries.end())
    {
        it = Uries.find("unknown");
    }
    ++_uri_stats[*it];
}

const map<string_view, int> &Stats::GetMethodStats() const
{
    return _method_stats;
}

const map<string_view, int> &Stats::GetUriStats() const
{
    return _uri_stats;
}

vector<string_view> SplitIntoWords(string_view line)
{
    vector<string_view> result;

    while (true)
    {
        size_t space = line.find(' ');
        result.push_back(line.substr(0, space));
        if (space == line.npos)
        {
            break;
        }
        else
        {
            line.remove_prefix(space + 1);
        }
    }
    return result;
}

HttpRequest ParseRequest(string_view line)
{
    static constexpr size_t MethodPos = 0U;
    static constexpr size_t UriPos = 1U;
    static constexpr size_t ProtocolPos = 2U;

    size_t not_space_pos = line.find_first_not_of(' ');
    line = line.substr(not_space_pos, line.npos);

    vector<string_view> words = SplitIntoWords(line);

    return {words[MethodPos], words[UriPos], words[ProtocolPos]};
}