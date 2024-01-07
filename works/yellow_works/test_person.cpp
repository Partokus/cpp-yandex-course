#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <optional>
#include <variant>
#include <cmath>
#include <numeric>
#include <limits>
#include <tuple>

using namespace std;

template <class T>
ostream &operator<<(ostream &os, const vector<T> &s)
{
    os << "{";
    bool first = true;
    for (const auto &x : s)
    {
        if (!first)
        {
            os << ", ";
        }
        first = false;
        os << x;
    }
    return os << "}";
}

template <class T>
ostream &operator<<(ostream &os, const set<T> &s)
{
    os << "{";
    bool first = true;
    for (const auto &x : s)
    {
        if (!first)
        {
            os << ", ";
        }
        first = false;
        os << x;
    }
    return os << "}";
}

template <class K, class V>
ostream &operator<<(ostream &os, const map<K, V> &m)
{
    os << "{";
    bool first = true;
    for (const auto &kv : m)
    {
        if (!first)
        {
            os << ", ";
        }
        first = false;
        os << kv.first << ": " << kv.second;
    }
    return os << "}";
}

template <class T, class U>
void AssertEqual(const T &t, const U &u, const string &hint = {})
{
    if (t != u)
    {
        ostringstream os;
        os << "Assertion failed: " << t << " != " << u;
        if (!hint.empty())
        {
            os << " hint: " << hint;
        }
        throw runtime_error(os.str());
    }
}

void Assert(bool b, const string &hint)
{
    AssertEqual(b, true, hint);
}

class TestRunner
{
public:
    template <class TestFunc>
    void RunTest(TestFunc func, const string &test_name)
    {
        try
        {
            func();
            cerr << test_name << " OK" << endl;
        }
        catch (exception &e)
        {
            ++fail_count;
            cerr << test_name << " fail: " << e.what() << endl;
        }
        catch (...)
        {
            ++fail_count;
            cerr << "Unknown exception caught" << endl;
        }
    }

    ~TestRunner()
    {
        if (fail_count > 0)
        {
            cerr << fail_count << " unit tests failed. Terminate" << endl;
            exit(1);
        }
    }

private:
    int fail_count = 0;
};

// // если имя неизвестно, возвращает пустую строку
// string FindNameByYear(const map<int, string> &names, int year)
// {
//     string name; // изначально имя неизвестно

//     // перебираем всю историю по возрастанию ключа словаря, то есть в хронологическом порядке
//     for (const auto &item : names)
//     {
//         // если очередной год не больше данного, обновляем имя
//         if (item.first <= year)
//         {
//             name = item.second;
//         }
//         else
//         {
//             // иначе пора остановиться, так как эта запись и все последующие относятся к будущему
//             break;
//         }
//     }

//     return name;
// }

// class Person
// {
// public:
//     void ChangeFirstName(int year, const string &first_name)
//     {
//         first_names[year] = first_name;
//     }
//     void ChangeLastName(int year, const string &last_name)
//     {
//         last_names[year] = last_name;
//     }
//     string GetFullName(int year)
//     {
//         // получаем имя и фамилию по состоянию на год year
//         const string first_name = FindNameByYear(first_names, year);
//         const string last_name = FindNameByYear(last_names, year);

//         // если и имя, и фамилия неизвестны
//         if (first_name.empty() && last_name.empty())
//         {
//             return "Incognito";

//             // если неизвестно только имя
//         }
//         else if (first_name.empty())
//         {
//             return last_name + " with unknown first name";

//             // если неизвестна только фамилия
//         }
//         else if (last_name.empty())
//         {
//             return first_name + " with unknown last name";

//             // если известны и имя, и фамилия
//         }
//         else
//         {
//             return first_name + " " + last_name;
//         }
//     }

// private:
//     map<int, string> first_names;
//     map<int, string> last_names;
// };

void testPerson()
{
    {
        Person person;

        person.ChangeFirstName(1960, "Joe");
        AssertEqual(person.GetFullName(1960), "Joe with unknown last name", "ChangeFirstName 0");
        AssertEqual(person.GetFullName(1961), "Joe with unknown last name", "ChangeFirstName 1");
        AssertEqual(person.GetFullName(2050), "Joe with unknown last name", "ChangeFirstName 2");

        person.ChangeFirstName(1965, "Elise");
        AssertEqual(person.GetFullName(1961), "Joe with unknown last name", "ChangeFirstName 3");
        AssertEqual(person.GetFullName(1965), "Elise with unknown last name", "ChangeFirstName 4");
        AssertEqual(person.GetFullName(1975), "Elise with unknown last name", "ChangeFirstName 5");

        person.ChangeFirstName(1955, "Emma");
        AssertEqual(person.GetFullName(1956), "Emma with unknown last name", "ChangeFirstName 6");
        AssertEqual(person.GetFullName(1960), "Joe with unknown last name", "ChangeFirstName 7");
        AssertEqual(person.GetFullName(1959), "Emma with unknown last name", "ChangeFirstName 8");
        AssertEqual(person.GetFullName(1975), "Elise with unknown last name", "ChangeFirstName 9");

        AssertEqual(person.GetFullName(1954), "Incognito", "ChangeFirstName 10");

        person.ChangeFirstName(-3, "Jesus");
        AssertEqual(person.GetFullName(-2), "Jesus with unknown last name", "ChangeFirstName 11");
        AssertEqual(person.GetFullName(0), "Jesus with unknown last name", "ChangeFirstName 12");
        AssertEqual(person.GetFullName(5), "Jesus with unknown last name", "ChangeFirstName 13");

        person.ChangeFirstName(1962, "John");
        AssertEqual(person.GetFullName(1961), "Joe with unknown last name", "ChangeFirstName 14");
        AssertEqual(person.GetFullName(1962), "John with unknown last name", "ChangeFirstName 15");
        AssertEqual(person.GetFullName(1963), "John with unknown last name", "ChangeFirstName 16");
        AssertEqual(person.GetFullName(1965), "Elise with unknown last name", "ChangeFirstName 17");
    }

    {
        Person person;

        person.ChangeLastName(1960, "Brooke");
        AssertEqual(person.GetFullName(1960), "Brooke with unknown first name", "ChangeLastName 0");
        AssertEqual(person.GetFullName(1961), "Brooke with unknown first name", "ChangeLastName 1");
        AssertEqual(person.GetFullName(2050), "Brooke with unknown first name", "ChangeLastName 2");

        person.ChangeLastName(1965, "Johanson");
        AssertEqual(person.GetFullName(1961), "Brooke with unknown first name", "ChangeLastName 3");
        AssertEqual(person.GetFullName(1965), "Johanson with unknown first name", "ChangeLastName 4");
        AssertEqual(person.GetFullName(1975), "Johanson with unknown first name", "ChangeLastName 5");

        person.ChangeLastName(1955, "Duval");
        AssertEqual(person.GetFullName(1956), "Duval with unknown first name", "ChangeLastName 6");
        AssertEqual(person.GetFullName(1960), "Brooke with unknown first name", "ChangeLastName 7");
        AssertEqual(person.GetFullName(1959), "Duval with unknown first name", "ChangeLastName 8");
        AssertEqual(person.GetFullName(1975), "Johanson with unknown first name", "ChangeLastName 9");

        AssertEqual(person.GetFullName(1954), "Incognito", "ChangeLastName 10");

        person.ChangeLastName(-3, "Christ");
        AssertEqual(person.GetFullName(-2), "Christ with unknown first name", "ChangeLastName 12");
        AssertEqual(person.GetFullName(0), "Christ with unknown first name", "ChangeLastName 13");
        AssertEqual(person.GetFullName(5), "Christ with unknown first name", "ChangeLastName 14");

        person.ChangeLastName(1962, "Wick");
        AssertEqual(person.GetFullName(1961), "Brooke with unknown first name", "ChangeLastName 15");
        AssertEqual(person.GetFullName(1962), "Wick with unknown first name", "ChangeLastName 16");
        AssertEqual(person.GetFullName(1963), "Wick with unknown first name", "ChangeLastName 17");
        AssertEqual(person.GetFullName(1965), "Johanson with unknown first name", "ChangeLastName 18");
    }

    {
        Person person;

        person.ChangeFirstName(1960, "Joe");
        person.ChangeLastName(1961, "Brooke");

        AssertEqual(person.GetFullName(1960), "Joe with unknown last name", "Both 0");
        AssertEqual(person.GetFullName(1961), "Joe Brooke", "Both 1");
        AssertEqual(person.GetFullName(1959), "Incognito", "Both 2");

        person.ChangeLastName(1955, "Duval");
        AssertEqual(person.GetFullName(1955), "Duval with unknown first name", "Both 3");
        AssertEqual(person.GetFullName(1957), "Duval with unknown first name", "Both 4");
        AssertEqual(person.GetFullName(1960), "Joe Duval", "Both 5");
        AssertEqual(person.GetFullName(1961), "Joe Brooke", "Both 6");

        person.ChangeFirstName(1955, "Emma");
        AssertEqual(person.GetFullName(1955), "Emma Duval", "Both 7");
        AssertEqual(person.GetFullName(1957), "Emma Duval", "Both 8");
        AssertEqual(person.GetFullName(1960), "Joe Duval", "Both 9");

        person.ChangeLastName(1957, "Wick");
        AssertEqual(person.GetFullName(1956), "Emma Duval", "Both 10");
        AssertEqual(person.GetFullName(1957), "Emma Wick", "Both 11");
        AssertEqual(person.GetFullName(1960), "Joe Wick", "Both 12");
    }

    {
        Person person;
        AssertEqual(person.GetFullName(1960), "Incognito", "Incognito");
    }
}

int main()
{
    TestRunner runner;

    runner.RunTest(testPerson, "testPerson");
    return 0;
}
