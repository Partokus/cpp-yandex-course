#include <test_runner.h>
#include <algorithm>
#include <numeric>
#include <cmath>

using namespace std;

void TestAll()
{
    TestRunner tr = {};
}

int IncTestValue(int a, int b)
{
    static int test = a + b;
    return test;
}

int main()
{
    IncTestValue(5, 5);
    IncTestValue(5, 5);
    IncTestValue(5, 5);
    cout << IncTestValue(5, 5) << endl;
    return 0;
}
