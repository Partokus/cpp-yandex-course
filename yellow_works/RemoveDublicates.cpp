#include <test_runner.h>
#include <algorithm>

using namespace std;

template <typename T>
void RemoveDuplicates(vector<T> &elements)
{
    sort(begin(elements), end(elements));
    auto it = unique(begin(elements), end(elements));
    elements.erase(it, end(elements));
}

void TestRemoveDuplicates()
{
}

void TestAll()
{
    TestRunner tr = {};
    tr.RunTest(TestRemoveDuplicates, "TestRemoveDuplicates");
}
