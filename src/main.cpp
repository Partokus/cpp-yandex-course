#include <profile.h>
#include <test_runner.h>

void TestAll();
void Profile();

void TestAll()
{
    TestRunner tr{};
}

void Profile()
{
}

int main()
{
#if defined(LOCAL_BUILD)
    TestAll();
#endif

    return 0;
}