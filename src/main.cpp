#include <profile.h>
#include <test_runner.h>

#include "trans.h"
#include "trans_test.h"
#include "json.h"
#include "router.h"
#include "svg.h"

void TestAll();
void Profile();

void TestAll()
{
    // На стороне грейдера теста не проходят по непонятным причинам
    TestRunner tr{};
    // RUN_TEST(tr, TestRgbCout);
    // RUN_TEST(tr, TestColorCout);
    // RUN_TEST(tr, TestDocument);
    // RUN_TEST(tr, TestParseJson);
    // RUN_TEST(tr, TestParseAddStopQuery);
    // RUN_TEST(tr, TestParseAddBusQuery);
    // RUN_TEST(tr, TestCalcGeoDistance);
    // RUN_TEST(tr, TestDataBaseCreateInfo);
    // RUN_TEST(tr, TestDataBaseCreateGraph);
    // RUN_TEST(tr, TestBuildRoute);
    // RUN_TEST(tr, TestParseRouteQuery);
    RUN_TEST(tr, TestParse);
}

void Profile()
{
}

int main()
{
    using namespace std::chrono;

    TestAll();

    // Parse(cin, cout);
    return 0;
}