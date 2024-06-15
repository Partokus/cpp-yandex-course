#include "parse.h"

string_view Strip(string_view sv) {
  while (!sv.empty() && isspace(sv.front())) {
    sv.remove_prefix(1);
  }
  while (!sv.empty() && isspace(sv.back())) {
    sv.remove_suffix(1);
  }
  return sv;
}

vector<string_view> SplitBy(string_view sv, char sep) {
    vector<string_view> result;

    if (sv.empty())
    {
        return {};
    }
    else if (sv[0] == sep)
    {
        size_t not_space = sv.find_first_not_of(sep);
        sv.remove_prefix(not_space);
    }

    while (true)
    {
        size_t space = sv.find(sep);
        result.push_back(sv.substr(0, space));
        if (space == sv.npos)
        {
            break;
        }
        else
        {
            sv.remove_prefix(space + 1);
            // ищем лишние пробелы, так как их может быть больше одного
            size_t not_space = sv.find_first_not_of(sep);
            if (not_space == sv.npos)
            {
                break;
            }
            sv.remove_prefix(not_space);
        }
    }
    return result;
}

