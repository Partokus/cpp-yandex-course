#include <profile.h>
#include <test_runner.h>
#include <vector>
#include <string>
#include <algorithm>
#include <iterator>

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
    using Letters = set<Char<String>>;

    map<Letters, Group<String>> groups;

    for (String &str : strings)
    {
        groups[Letters{str.begin(), str.end()}].emplace_back(move(str));
    }

    vector<Group<String>> result;
    result.reserve(strings.size());
    for (auto &[letters, group] : groups)
    {
        result.emplace_back(move(group));
    }
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

void TestGroupingHeavy()
{
    vector<vector<string>> strings = {{"law", "wal", "wal", "law"}, {"lwa", "wal"}, {"wal", "law", "wal"}, {"top", "pot"}, {"pot", "top"}};
    auto groups = GroupHeavyStrings(strings);
    ASSERT_EQUAL(groups.size(), 3);
    sort(begin(groups), end(groups)); // Порядок групп не имеет значения
    ASSERT_EQUAL(groups[0], vector<vector<string>>({{"law", "wal", "wal", "law"}, {"wal", "law", "wal"}}));
    ASSERT_EQUAL(groups[1], vector<vector<string>>({{"lwa", "wal"}}));
    ASSERT_EQUAL(groups[2], vector<vector<string>>({{"top", "pot"}, {"pot", "top"}}));
}

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, TestGroupingABC);
    RUN_TEST(tr, TestGroupingReal);
    RUN_TEST(tr, TestGroupingHeavy);
}

void Profile()
{
    string symbol1;
    string symbol2;
    string symbol3;
    for (int i = 0; i < 10; ++i)
    {
        symbol1 += "ten symbo1";
    }
    for (int i = 0; i < 10; ++i)
    {
        symbol2 += "ten symbo3";
    }
    for (int i = 0; i < 10; ++i)
    {
        symbol3 += "ten symbo2";
    }
    using String = vector<string>;
    vector<String> strings;

    String str1;
    for (int k = 0; k < 33; ++k)
    {
        str1.push_back(symbol1);
    }
    for (int k = 0; k < 33; ++k)
    {
        str1.push_back(symbol2);
    }
    for (int k = 0; k < 33; ++k)
    {
        str1.push_back(symbol3);
    }

    for (int i = 0; i < 30'000; ++i)
    {
        strings.push_back(str1);
    }

    {
        LOG_DURATION("heavy");
        auto groups = GroupHeavyStrings(strings);
        size_t size = groups.size();
        ++size;
    }
}
