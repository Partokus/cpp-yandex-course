#include <test_runner.h>
#include <algorithm>

using namespace std;

void PrintVectorPart(const vector<int> &numbers)
{
    auto negative_it = find_if(numbers.begin(), numbers.end(), [](const int number)
                               { return number < 0; });

    for (auto it = negative_it; it != numbers.begin();)
    {
        cout << *(--it) << " ";
    }
}

void TestPrintVectorPart()
{
}

void TestAll()
{
    TestRunner tr = {};
    tr.RunTest(TestPrintVectorPart, "TestPrintVectorPart");
}

int main()
{
    PrintVectorPart({6, 1, 8, -5, 4});
    cout << endl;
    PrintVectorPart({-6, 1, 8, -5, 4}); // ничего не выведется
    cout << endl;
    PrintVectorPart({6, 1, 8, 5, 4});
    cout << endl;
    return 0;
}