#include <profile.h>
#include <test_runner.h>

#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <utility>
#include <map>
#include <optional>
#include <unordered_set>
#include <algorithm>
#include <numeric>

using namespace std;

void TestAll();
void Profile();

template <typename Iterator>
class IteratorRange
{
public:
    IteratorRange(Iterator begin, Iterator end)
        : first(begin), last(end)
    {
    }

    Iterator begin() const
    {
        return first;
    }

    Iterator end() const
    {
        return last;
    }

private:
    Iterator first, last;
};

template <typename Collection>
auto Head(Collection &v, size_t top)
{
    return IteratorRange{v.begin(), next(v.begin(), min(top, v.size()))};
}

struct Person
{
    string name;
    int age = 0;
    int income = 0;
    bool is_male = false;
};

vector<Person> ReadPeople(istream &input)
{
    int count;
    input >> count;

    vector<Person> result(count);
    for (Person &p : result)
    {
        char gender;
        input >> p.name >> p.age >> p.income >> gender;
        p.is_male = gender == 'M';
    }

    return result;
}

struct Stats
{
    vector<Person> people_by_age;
    vector<int> cumulative_wealth;
    std::optional<string> popular_man_name = std::nullopt;
    std::optional<string> popular_woman_name = std::nullopt;
};

Stats MakeStats(const vector<Person> people)
{
    Stats result{};

    result.people_by_age = [&people]()
    {
        vector<Person> result = people;

        sort(begin(result), end(result),
            [](const Person &lhs, const Person &rhs)
            { return lhs.age < rhs.age; });

        return result;
    }();

    result.cumulative_wealth = [&people]()
    {
        vector<int> result;
        result.reserve(people.size());

        vector<Person> wealthest_people = people;
        sort(wealthest_people.begin(), wealthest_people.end(),
            [](const Person &lhs, const Person &rhs)
            { return lhs.income > rhs.income; }
        );

        accumulate(
            wealthest_people.cbegin(), wealthest_people.cend(), 0,
            [&result](int cur, const Person &p)
            {
                result.push_back(cur + p.income);
                return result.back();
            }
        );

        return result;
    }();

    auto MakeMostPopularName = [&people](char gender)
    {
        map<string_view, size_t> names_count;

        for (const Person &p : people)
        {
            if ((p.is_male ? 'M' : 'W') == gender)
            {
                ++names_count[p.name];
            }
        }

        if (names_count.empty())
            return std::optional<string>{std::nullopt};

        auto it = max_element(names_count.begin(), names_count.end(),
            [](const auto &lhs, const auto &rhs){
                return lhs.second < rhs.second;
            });

        return std::optional<string>{it->first};
    };

    result.popular_man_name = MakeMostPopularName('M');
    result.popular_woman_name = MakeMostPopularName('W');

    return result;
}

int main()
{
    TestAll();
    Profile();

    const Stats stats = MakeStats(ReadPeople(cin));

    for (string command; cin >> command;)
    {
        if (command == "AGE")
        {
            int adult_age = 0;
            cin >> adult_age;

            auto adult_begin = lower_bound(
                stats.people_by_age.cbegin(), stats.people_by_age.cend(),
                adult_age, [](const Person &lhs, int age)
                { return lhs.age < age; });

            cout << "There are " << std::distance(adult_begin, stats.people_by_age.cend())
                 << " adult people for maturity age " << adult_age << '\n';
        }
        else if (command == "WEALTHY")
        {
            int count = 0;
            cin >> count;

            cout << "Top-" << count << " people have total income " <<
                stats.cumulative_wealth[count - 1] << '\n';
        }
        else if (command == "POPULAR_NAME")
        {
            char gender;
            cin >> gender;

            const std::optional<string> &popular_name = gender == 'M' ? stats.popular_man_name :
                                                                        stats.popular_woman_name;

            if (not popular_name.has_value())
            {
                cout << "No people of gender " << gender << '\n';
                continue;
            }

            cout << "Most popular name among people of gender " <<
                gender << " is " << *popular_name << '\n';
        }
    }
    return 0;
}

void TestAll()
{
    TestRunner tr{};
}

void Profile()
{
}