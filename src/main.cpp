#include "test_runner.h"

#include <string>
#include <vector>
#include <list>
#include <forward_list>
#include <numeric>
#include <iterator>
#include <algorithm>

void TestAll();

using namespace std;

int main()
{
    TestAll();
    return 0;
}

void TestAll()
{
    TestRunner tr;
    // RUN_TEST(tr, TestUniqueMax);
}