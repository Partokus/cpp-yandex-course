#include <iostream>
#include <stdexcept>

#include "test_runner.h"
#include "database.h"
#include "date.h"
#include "condition_parser.h"
#include "node.h"
#include "token.h"

using namespace std;

void TestAll();

string ParseEvent(istream &is)
{
    string str{};
    getline(is, str);
    const string event(str, str.find_first_not_of(' '));
    return event;
}

int main()
{
    TestAll();

    Database db;

    for (string line; getline(cin, line);)
    {
        istringstream is(line);

        string command;
        is >> command;
        if (command == "Add")
        {
            const auto date = ParseDate(is);
            const auto event = ParseEvent(is);
            db.Add(date, event);
        }
        else if (command == "Print")
        {
            db.Print(cout);
        }
        else if (command == "Del")
        {
            auto condition = ParseCondition(is);
            auto predicate = [condition](const Date &date, const string &event)
            {
                return condition->Evaluate(date, event);
            };
            int count = db.RemoveIf(predicate);
            cout << "Removed " << count << " entries" << endl;
        }
        else if (command == "Find")
        {
            auto condition = ParseCondition(is);
            auto predicate = [condition](const Date &date, const string &event)
            {
                return condition->Evaluate(date, event);
            };

            const auto entries = db.FindIf(predicate);
            for (const auto &entry : entries)
            {
                cout << entry << endl;
            }
            cout << "Found " << entries.size() << " entries" << endl;
        }
        else if (command == "Last")
        {
            try
            {
                cout << db.Last(ParseDate(is)) << endl;
            }
            catch (invalid_argument &)
            {
                cout << "No entries" << endl;
            }
        }
        else if (command.empty())
        {
            continue;
        }
        else
        {
            throw logic_error("Unknown command: " + command);
        }
    }

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

void TestParseEvent()
{
    {
        istringstream is("event");
        AssertEqual(ParseEvent(is), "event", "Parse event without leading spaces");
    }
    {
        istringstream is("   sport event ");
        AssertEqual(ParseEvent(is), "sport event ", "Parse event with leading spaces");
    }
    {
        istringstream is("  first event  \n  second event");
        vector<string> events;
        events.push_back(ParseEvent(is));
        events.push_back(ParseEvent(is));
        AssertEqual(events, vector<string>{"first event  ", "second event"}, "Parse multiple events");
    }
}

void TestDataBase()
{
    {
        Database db{};
        stringstream ss{};
        db.Add(Date{2000, 1, 1}, "wow tournament makgora 4");
        db.Add(Date{2000, 1, 2}, "wow tournament makgora 3");
        db.Add(Date{2000, 1, 3}, "wow tournament makgora 6");
        db.Print(ss);
        string data_and_event{};
        getline(ss, data_and_event);
        AssertEqual(data_and_event, "2000-01-01 wow tournament makgora 4", "DataBase Print 0");
        getline(ss, data_and_event);
        AssertEqual(data_and_event, "2000-01-02 wow tournament makgora 3", "DataBase Print 1");
        getline(ss, data_and_event);
        AssertEqual(data_and_event, "2000-01-03 wow tournament makgora 6", "DataBase Print 2");
    }
    {
        Database db{};
        db.Add(Date{2017, 6, 1}, "1st of June");
        db.Add(Date{2017, 7, 8}, "8th of July");
        db.Add(Date{2017, 7, 8}, "Someone's birthday");

        stringstream ss{"date == 2017-07-08"};
        auto condition = ParseCondition(ss);
        auto predicate1 = [condition](const Date &date, const string &event)
        {
            return condition->Evaluate(date, event);
        };
        AssertEqual(db.RemoveIf(predicate1), 2U, "DataBase RemoveIf 0");
        ss = stringstream{"event == \"1st of June\""};
        condition = ParseCondition(ss);
        auto predicate2 = [condition](const Date &date, const string &event)
        {
            return condition->Evaluate(date, event);
        };
        AssertEqual(db.RemoveIf(predicate2), 1U, "DataBase RemoveIf 1");
    }
}

void TestAll()
{
    TestRunner tr;
    tr.RunTest(TestParseEvent, "TestParseEvent");
    tr.RunTest(TestParseDate, "TestParseDate");
    tr.RunTest(TestDataBase, "TestDataBase");
}