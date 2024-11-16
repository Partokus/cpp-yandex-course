#include <profile.h>
#include <test_runner.h>

#include <cassert>
#include <algorithm>
#include <iterator>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <string>
#include <forward_list>
#include <iterator>
#include <deque>
#include <tuple>
#include <random>
#include <functional>
#include <fstream>
#include <random>
#include <tuple>
#include <json.h>

using namespace std;

void TestAll();
void Profile();

using namespace std;

struct Spending
{
    string category;
    int amount;
};

bool operator==(const Spending &lhs, const Spending &rhs)
{
    return lhs.category == rhs.category && lhs.amount == rhs.amount;
}

ostream &operator<<(ostream &os, const Spending &s)
{
    return os << '(' << s.category << ": " << s.amount << ')';
}

int CalculateTotalSpendings(
    const vector<Spending> &spendings)
{
    int result = 0;
    for (const Spending &s : spendings)
    {
        result += s.amount;
    }
    return result;
}

string MostExpensiveCategory(
    const vector<Spending> &spendings)
{
    auto compare_by_amount =
        [](const Spending &lhs, const Spending &rhs)
    {
        return lhs.amount < rhs.amount;
    };
    return max_element(begin(spendings), end(spendings),
                       compare_by_amount)
        ->category;
}

vector<Spending> LoadFromJson(istream &input)
{
    Document doc = Load(input);
    const vector<Node> &nodes_arr = doc.GetRoot().AsArray();

    vector<Spending> result;
    result.reserve(nodes_arr.size());

    for (const Node &node : nodes_arr)
    {
        const map<string, Node>& nodes_map = node.AsMap();
        result.push_back({nodes_map.at("category").AsString(),
                          nodes_map.at("amount").AsInt()});
    }

    return result;
}

int main()
{
    TestAll();
    Profile();
    return 0;
}

void TestLoadFromJson()
{
    istringstream json_input(R"([
    {"amount": 2500, "category": "food"},
    {"amount": 1150, "category": "transport"},
    {"amount": 5780, "category": "restaurants"},
    {"amount": 7500, "category": "clothes"},
    {"amount": 23740, "category": "travel"},
    {"amount": 12000, "category": "sport"}
  ])");

    const vector<Spending> spendings = LoadFromJson(json_input);

    const vector<Spending> expected = {
        {"food", 2500},
        {"transport", 1150},
        {"restaurants", 5780},
        {"clothes", 7500},
        {"travel", 23740},
        {"sport", 12000}};
    ASSERT_EQUAL(spendings, expected);
}

void TestJsonLibrary()
{
    istringstream json_input(R"([
    {"amount": 2500, "category": "food"},
    {"amount": 1150, "category": "transport"},
    {"amount": 12000, "category": "sport"}
  ])");

    Document doc = Load(json_input);
    const vector<Node> &root = doc.GetRoot().AsArray();
    ASSERT_EQUAL(root.size(), 3u);

    const map<string, Node> &food = root.front().AsMap();
    ASSERT_EQUAL(food.at("category").AsString(), "food");
    ASSERT_EQUAL(food.at("amount").AsInt(), 2500);

    const map<string, Node> &sport = root.back().AsMap();
    ASSERT_EQUAL(sport.at("category").AsString(), "sport");
    ASSERT_EQUAL(sport.at("amount").AsInt(), 12000);

    Node transport(map<string, Node>{{"category", Node("transport")}, {"amount", Node(1150)}});
    Node array_node(vector<Node>{transport});
    ASSERT_EQUAL(array_node.AsArray().size(), 1u);
}

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, TestLoadFromJson);
    RUN_TEST(tr, TestJsonLibrary);
}

void Profile()
{
}
