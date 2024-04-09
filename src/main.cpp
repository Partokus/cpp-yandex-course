#include "simple_vector.h"
#include <profile.h>
#include <iostream>
#include <test_runner.h>
#include <vector>
#include <string>
#include <algorithm>
#include <iterator>
#include <numeric>


using namespace std;

void TestAll();
void Profile();

int main()
{
    TestAll();
    Profile();
    return 0;
}

void TestCopyAssignment()
{
    SimpleVector<int> numbers(10);
    iota(numbers.begin(), numbers.end(), 1);

    SimpleVector<int> dest;
    ASSERT_EQUAL(dest.Size(), 0u);

    dest = numbers;
    ASSERT_EQUAL(dest.Size(), numbers.Size());
    ASSERT(dest.Capacity() >= dest.Size());
    ASSERT(equal(dest.begin(), dest.end(), numbers.begin()));
}

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, TestCopyAssignment);
}

void Profile()
{
}
