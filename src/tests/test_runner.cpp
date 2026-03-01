#include "test_runner.h"

TestRunner::~TestRunner()
{
    if (fail_count > 0)
    {
        std::cerr << fail_count << " unit tests failed. Terminate" << std::endl;
        exit(1);
    }
}

void Assert(bool b, const string &hint)
{
    AssertEqual(b, true, hint);
}

bool AssertDouble(double lhs, double rhs)
{
    return lhs > (rhs - 0.1) and lhs < (rhs + 0.1);
}