#include <profile.h>
#include <test_runner.h>

#include "json_printer.h"

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

    // PrintJsonObject(std::cout).Key("Hello \"world\"").Key("ss");


    return 0;
}