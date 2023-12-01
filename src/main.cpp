#include "test_runner.h"

#include <string>
#include <vector>
#include <numeric>
#include <iterator>
#include <algorithm>

void TestAll();

using namespace std;

#define TOKENPASTE(x, y) x ## y
#define TOKENPASTE2(x, y) TOKENPASTE(x, y)
#define UNIQ_ID TOKENPASTE2(uniq_id, __LINE__)

int main()
{
    int UNIQ_ID = 0;
    string UNIQ_ID = "hello";
    vector<string> UNIQ_ID = {"hello", "world"};
    vector<int> UNIQ_ID = {1, 2, 3, 4};
}

void TestAll()
{
    // TestRunner tr;
    // RUN_TEST(tr, TestPrintValue);
}