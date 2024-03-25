#include <profile.h>
#include <test_runner.h>
#include <iomanip>
#include <iostream>
#include <set>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <string_view>
#include <random>
#include <list>
#include <iterator>
#include <numeric>

using namespace std;

void TestAll();
void Profile();

// Объявляем Group<String> для произвольного типа String
// синонимом vector<String>.
// Благодаря этому в качестве возвращаемого значения
// функции можно указать не малопонятный вектор векторов,
// а вектор групп — vector<Group<String>>.
template <typename String>
using Group = vector<String>;

// Ещё один шаблонный синоним типа
// позволяет вместо громоздкого typename String::value_type
// использовать Char<String>
template <typename String>
using Char = typename String::value_type;

template <typename String>
vector<Group<String>> GroupHeavyStrings(vector<String> strings)
{
    if (strings.empty())
    {
        return {};
    }
    if (strings.size() == 1U)
    {
        return {Group<String>{strings.front()}};
    }

    vector<Group<String>> result;
    result.reserve(strings.size());

    list<set<Char<String>>> sorted_chars(strings.size());

    auto it = sorted_chars.begin();
    for (const auto &word : strings)
    {
        *it++ = set<Char<String>>{word.cbegin(), word.cend()};
    }

    list<size_t> strings_indexes(strings.size());
    iota(strings_indexes.begin(), strings_indexes.end(), 0U);

    while (not sorted_chars.empty())
    {
        auto &leader_word = sorted_chars.front();
        result.resize(result.size() + 1U);
        result.back().push_back(move(strings[strings_indexes.front()]));
        auto it_strings_index = strings_indexes.erase(strings_indexes.begin());

        for (auto it_sorted_word = next(sorted_chars.begin()); it_sorted_word != sorted_chars.cend();)
        {
            if (leader_word == *it_sorted_word)
            {
                result.back().push_back(move(strings[*it_strings_index]));
                it_strings_index = strings_indexes.erase(it_strings_index);
                it_sorted_word = sorted_chars.erase(it_sorted_word);
            }
            else
            {
                ++it_sorted_word;
                ++it_strings_index;
            }
        }
        sorted_chars.erase(sorted_chars.begin());
    }

    result.shrink_to_fit();
    return result;
}

int main()
{
    TestAll();
    Profile();
    return 0;
}

void TestGroupingABC()
{
    vector<string> strings = {"caab", "abc", "cccc", "bacc", "c"};
    auto groups = GroupHeavyStrings(strings);
    ASSERT_EQUAL(groups.size(), 2);
    sort(begin(groups), end(groups)); // Порядок групп не имеет значения
    ASSERT_EQUAL(groups[0], vector<string>({"caab", "abc", "bacc"}));
    ASSERT_EQUAL(groups[1], vector<string>({"cccc", "c"}));
}

void TestGroupingReal()
{
    vector<string> strings = {"law", "port", "top", "laptop", "pot", "paloalto", "wall", "awl"};
    auto groups = GroupHeavyStrings(strings);
    ASSERT_EQUAL(groups.size(), 4);
    sort(begin(groups), end(groups)); // Порядок групп не имеет значения
    ASSERT_EQUAL(groups[0], vector<string>({"laptop", "paloalto"}));
    ASSERT_EQUAL(groups[1], vector<string>({"law", "wall", "awl"}));
    ASSERT_EQUAL(groups[2], vector<string>({"port"}));
    ASSERT_EQUAL(groups[3], vector<string>({"top", "pot"}));
}

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, TestGroupingABC);
    RUN_TEST(tr, TestGroupingReal);
}

void Profile()
{
}
