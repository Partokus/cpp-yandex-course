// #include "database.h"
#include "date.h"
// #include "condition_parser.h"
#include "node.h"
#include "test_runner.h"

#include <iostream>
#include <stdexcept>

using namespace std;

void TestAll();

string ParseEvent(istream &is)
{
    // Реализуйте эту функцию
    return {};
}

int main()
{
    TestAll();



    // Database db;

    // for (string line; getline(cin, line);)
    // {
    //     istringstream is(line);

    //     string command;
    //     is >> command;
    //     if (command == "Add")
    //     {
    //         const auto date = ParseDate(is);
    //         const auto event = ParseEvent(is);
    //         db.Add(date, event);
    //     }
    //     else if (command == "Print")
    //     {
    //         db.Print(cout);
    //     }
    //     else if (command == "Del")
    //     {
    //         auto condition = ParseCondition(is);
    //         auto predicate = [condition](const Date &date, const string &event)
    //         {
    //             return condition->Evaluate(date, event);
    //         };
    //         int count = db.RemoveIf(predicate);
    //         cout << "Removed " << count << " entries" << endl;
    //     }
    //     else if (command == "Find")
    //     {
    //         auto condition = ParseCondition(is);
    //         auto predicate = [condition](const Date &date, const string &event)
    //         {
    //             return condition->Evaluate(date, event);
    //         };

    //         const auto entries = db.FindIf(predicate);
    //         for (const auto &entry : entries)
    //         {
    //             cout << entry << endl;
    //         }
    //         cout << "Found " << entries.size() << " entries" << endl;
    //     }
    //     else if (command == "Last")
    //     {
    //         try
    //         {
    //             cout << db.Last(ParseDate(is)) << endl;
    //         }
    //         catch (invalid_argument &)
    //         {
    //             cout << "No entries" << endl;
    //         }
    //     }
    //     else if (command.empty())
    //     {
    //         continue;
    //     }
    //     else
    //     {
    //         throw logic_error("Unknown command: " + command);
    //     }
    // }

    return 0;
}

void TestParseDate()
{
    {
        istringstream ss("2000-02-05");
        const auto date = ParseDate(ss);
        const Date date_ref = {2000, 2, 5};
        AssertEqual(vector<int>{date.year, date.mounth, date.day}, vector<int>{date_ref.year, date_ref.mounth, date_ref.day}, "ParseDate 0");
    }
    {
        istringstream ss("0-1-2");
        const auto date = ParseDate(ss);
        const Date date_ref = {0, 1, 2};
        AssertEqual(vector<int>{date.year, date.mounth, date.day}, vector<int>{date_ref.year, date_ref.mounth, date_ref.day}, "ParseDate 1");
    }
}

void TestDateComparisonNode()
{
    {
        DateComparisonNode dcn(Comparison::Less, Date{2005, 01, 15});
        Assert(dcn.Evaluate(Date{2005, 01, 14}, {}), "DateComparisonNode Less 0");
        Assert(not dcn.Evaluate(Date{2005, 01, 15}, {}), "DateComparisonNode Less 1");
        Assert(not dcn.Evaluate(Date{2005, 01, 16}, {}), "DateComparisonNode Less 2");
    }
    {
        DateComparisonNode dcn(Comparison::LessOrEqual, Date{2005, 01, 15});
        Assert(dcn.Evaluate(Date{2005, 01, 14}, {}), "DateComparisonNode LessOrEqual 0");
        Assert(dcn.Evaluate(Date{2005, 01, 15}, {}), "DateComparisonNode LessOrEqual 1");
        Assert(not dcn.Evaluate(Date{2005, 01, 16}, {}), "DateComparisonNode LessOrEqual 2");
    }
    {
        DateComparisonNode dcn(Comparison::Greater, Date{2005, 01, 15});
        Assert(not dcn.Evaluate(Date{2005, 01, 14}, {}), "DateComparisonNode Greater 0");
        Assert(not dcn.Evaluate(Date{2005, 01, 15}, {}), "DateComparisonNode Greater 1");
        Assert(dcn.Evaluate(Date{2005, 01, 16}, {}), "DateComparisonNode Greater 2");
    }
    {
        DateComparisonNode dcn(Comparison::GreaterOrEqual, Date{2005, 01, 15});
        Assert(not dcn.Evaluate(Date{2005, 01, 14}, {}), "DateComparisonNode GreaterOrEqual 0");
        Assert(dcn.Evaluate(Date{2005, 01, 15}, {}), "DateComparisonNode GreaterOrEqual 1");
        Assert(dcn.Evaluate(Date{2005, 01, 16}, {}), "DateComparisonNode GreaterOrEqual 2");
    }
    {
        DateComparisonNode dcn(Comparison::Equal, Date{2005, 01, 15});
        Assert(not dcn.Evaluate(Date{2005, 01, 14}, {}), "DateComparisonNode Equal 0");
        Assert(dcn.Evaluate(Date{2005, 01, 15}, {}), "DateComparisonNode Equal 1");
        Assert(not dcn.Evaluate(Date{2005, 01, 16}, {}), "DateComparisonNode Equal 2");
    }
    {
        DateComparisonNode dcn(Comparison::NotEqual, Date{2005, 01, 15});
        Assert(dcn.Evaluate(Date{2005, 01, 14}, {}), "DateComparisonNode NotEqual 0");
        Assert(not dcn.Evaluate(Date{2005, 01, 15}, {}), "DateComparisonNode NotEqual 1");
        Assert(dcn.Evaluate(Date{2005, 01, 16}, {}), "DateComparisonNode NotEqual 2");
    }
}

void TestEventComparisonNode()
{
    {
        EventComparisonNode ecn(Comparison::Less, "bac");
        Assert(ecn.Evaluate({}, "abc"), "EventComparisonNode Less 0");
        Assert(not ecn.Evaluate({}, "bac"), "EventComparisonNode Less 1");
        Assert(not ecn.Evaluate({}, "cba"), "EventComparisonNode Less 2");
    }
    {
        EventComparisonNode ecn(Comparison::LessOrEqual, "bac");
        Assert(ecn.Evaluate({}, "abc"), "EventComparisonNode LessOrEqual 0");
        Assert(ecn.Evaluate({}, "bac"), "EventComparisonNode LessOrEqual 1");
        Assert(not ecn.Evaluate({}, "cba"), "EventComparisonNode LessOrEqual 2");
    }
    {
        EventComparisonNode ecn(Comparison::Greater, "bac");
        Assert(not ecn.Evaluate({}, "abc"), "EventComparisonNode Greater 0");
        Assert(not ecn.Evaluate({}, "bac"), "EventComparisonNode Greater 1");
        Assert(ecn.Evaluate({}, "cba"), "EventComparisonNode Greater 2");
    }
    {
        EventComparisonNode ecn(Comparison::GreaterOrEqual, "bac");
        Assert(not ecn.Evaluate({}, "abc"), "EventComparisonNode GreaterOrEqual 0");
        Assert(ecn.Evaluate({}, "bac"), "EventComparisonNode GreaterOrEqual 1");
        Assert(ecn.Evaluate({}, "cba"), "EventComparisonNode GreaterOrEqual 2");
    }
    {
        EventComparisonNode ecn(Comparison::Equal, "bac");
        Assert(not ecn.Evaluate({}, "abc"), "EventComparisonNode Equal 0");
        Assert(ecn.Evaluate({}, "bac"), "EventComparisonNode Equal 1");
        Assert(not ecn.Evaluate({}, "cba"), "EventComparisonNode Equal 2");
    }
    {
        EventComparisonNode ecn(Comparison::NotEqual, "bac");
        Assert(ecn.Evaluate({}, "abc"), "EventComparisonNode NotEqual 0");
        Assert(not ecn.Evaluate({}, "bac"), "EventComparisonNode NotEqual 1");
        Assert(ecn.Evaluate({}, "cba"), "EventComparisonNode NotEqual 2");
    }
}

void TestParseEvent()
{
    // {
    //     istringstream is("event");
    //     AssertEqual(ParseEvent(is), "event", "Parse event without leading spaces");
    // }
    // {
    //     istringstream is("   sport event ");
    //     AssertEqual(ParseEvent(is), "sport event ", "Parse event with leading spaces");
    // }
    // {
    //     istringstream is("  first event  \n  second event");
    //     vector<string> events;
    //     events.push_back(ParseEvent(is));
    //     events.push_back(ParseEvent(is));
    //     AssertEqual(events, vector<string>{"first event  ", "second event"}, "Parse multiple events");
    // }
}

void TestAll()
{
    TestRunner tr;
    tr.RunTest(TestParseEvent, "TestParseEvent");
    tr.RunTest(TestParseDate, "TestParseDate");
    tr.RunTest(TestDateComparisonNode, "TestDateComparisonNode");
    tr.RunTest(TestEventComparisonNode, "TestEventComparisonNode");
    // tr.RunTest(TestParseCondition, "TestParseCondition");
}