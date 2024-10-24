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
#include <xml.h>

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

vector<Spending> LoadFromXml(istream &input)
{
    Document doc = Load(input);
    const vector<Node> &childrens = doc.GetRoot().Children();

    vector<Spending> result;
    result.reserve(childrens.size());

    for (const Node &children : childrens)
    {
        result.push_back({children.AttributeValue<string>("category"),
                          children.AttributeValue<int>("amount")});
    }

    return result;
}

int main()
{
    TestAll();
    Profile();
    return 0;
}

void TestLoadFromXml()
{
    istringstream xml_input(R"(<july>
    <spend amount="2500" category="food"></spend>
    <spend amount="1150" category="transport"></spend>
    <spend amount="5780" category="restaurants"></spend>
    <spend amount="7500" category="clothes"></spend>
    <spend amount="23740" category="travel"></spend>
    <spend amount="12000" category="sport"></spend>
  </july>)");

    const vector<Spending> spendings = LoadFromXml(xml_input);

    const vector<Spending> expected = {
        {"food", 2500},
        {"transport", 1150},
        {"restaurants", 5780},
        {"clothes", 7500},
        {"travel", 23740},
        {"sport", 12000}};
    ASSERT_EQUAL(spendings, expected);
}

void TestXmlLibrary()
{
    // Тест демонстрирует, как пользоваться библиотекой из файла xml.h

    istringstream xml_input(R"(<july>
    <spend amount="2500" category="food"></spend>
    <spend amount="23740" category="travel"></spend>
    <spend amount="12000" category="sport"></spend>
  </july>)");

    Document doc = Load(xml_input);
    const Node &root = doc.GetRoot();
    ASSERT_EQUAL(root.Name(), "july");
    ASSERT_EQUAL(root.Children().size(), 3u);

    const Node &food = root.Children().front();
    ASSERT_EQUAL(food.AttributeValue<string>("category"), "food");
    ASSERT_EQUAL(food.AttributeValue<int>("amount"), 2500);

    const Node &sport = root.Children().back();
    ASSERT_EQUAL(sport.AttributeValue<string>("category"), "sport");
    ASSERT_EQUAL(sport.AttributeValue<int>("amount"), 12000);

    Node july("july", {});
    Node transport("spend", {{"category", "transport"}, {"amount", "1150"}});
    july.AddChild(transport);
    ASSERT_EQUAL(july.Children().size(), 1u);
}

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, TestXmlLibrary);
    RUN_TEST(tr, TestLoadFromXml);
}

void Profile()
{
}
