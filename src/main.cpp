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

#define PRINT_VALUES(out, x, y) ((out) << (x) << endl << (y) << endl)

int main()
{
    TestAll();

    return 0;
}

void TestPrintValue()
{
    ostringstream output;
    PRINT_VALUES(output, 5, "red belt");
    ASSERT_EQUAL(output.str(), "5\nred belt\n");
}

void TestAll()
{
    TestRunner tr;
    RUN_TEST(tr, TestPrintValue);
}