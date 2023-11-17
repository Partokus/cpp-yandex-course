#include <test_runner.h>
#include <algorithm>

using namespace std;

template <typename T>
vector<T> FindGreaterElements(const set<T> &elements, const T &border)
{
    auto it_border_greater = find_if(elements.begin(), elements.end(), [&border](const T &element) {
        return element > border;
    });
    return {it_border_greater, elements.end()};
}

void TestFindGreaterElements()
{
    AssertEqual(FindGreaterElements(set<int>{1, 2, 5, 3, 6, 15, 21}, 3), vector<int>{5, 6, 15, 21}, "FindGreaterElements 0");
    AssertEqual(FindGreaterElements(set<int>{1, 2, 5, 3, 6, 15, 21}, 4), vector<int>{5, 6, 15, 21}, "FindGreaterElements 1");
    AssertEqual(FindGreaterElements(set<int>{1, 2, 5, 3, 6, 15, 21, -8, -100, -2, 0, -4, -5}, -4), vector<int>{-2, 0, 1, 2, 3, 5, 6, 15, 21}, "FindGreaterElements 2");
    AssertEqual(FindGreaterElements(set<string>{"a", "b", "c", "f", "d"}, string{"b"}), vector<string>{"c", "d", "f"}, "FindGreaterElements 3");
}

void TestAll()
{
    TestRunner tr = {};
    tr.RunTest(TestFindGreaterElements, "TestFindGreaterElements");
}

int main()
{
    TestAll();

    for (int x : FindGreaterElements(set<int>{1, 5, 7, 8}, 5))
    {
        cout << x << " ";
    }
    cout << endl;

    string to_find = "Python";
    cout << FindGreaterElements(set<string>{"C", "C++"}, to_find).size() << endl;
    return 0;
}