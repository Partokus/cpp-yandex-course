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
    TestRunner tr{};
    // RUN_TEST(tr, TestParseJson);
    // RUN_TEST(tr, TestParseAddStopQuery);
    // RUN_TEST(tr, TestParseAddBusQuery);
    // RUN_TEST(tr, TestCalcGeoDistance);
    // RUN_TEST(tr, TestDataBaseCreateInfo);
    // RUN_TEST(tr, TestDataBaseCreateGraph);
    // RUN_TEST(tr, TestBuildRoute);
    // RUN_TEST(tr, TestParseRouteQuery);
    // RUN_TEST(tr, TestParse);

    // RUN_TEST(tr, TestRgbCout);
    // RUN_TEST(tr, TestColorCout);
    // RUN_TEST(tr, TestDocument);
    // RUN_TEST(tr, TestMakeRenderSettigs);
    // RUN_TEST(tr, Test15Failed);
    // RUN_TEST(tr, TestCreateMap);
}

void Profile()
{
}

int main()
{
    // TestAll();

    DataBase db;
    Parse(cin, cout, db);
    return 0;
}