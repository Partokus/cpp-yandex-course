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

// class Rational
// {
// public:
//     Rational() = default;

//     Rational(int numerator, int denominator)
//     {
//         if (numerator == 0)
//         {
//             return;
//         }

//         if (numerator < 0 and denominator < 0)
//         {
//             _numerator = abs(numerator);
//             _denominator = abs(denominator);
//         }
//         else if (denominator < 0)
//         {
//             _numerator = -numerator;
//             _denominator = abs(denominator);
//         }
//         else
//         {
//             _numerator = numerator;
//             _denominator = denominator;
//         }

//         int gcd_var = gcd(_numerator, _denominator);
//         _numerator /= gcd_var;
//         _denominator /= gcd_var;
//     }

//     int Numerator() const { return _numerator; }
//     int Denominator() const { return _denominator; }

// private:
//     int _numerator = 0;
//     int _denominator = 1;
// };

void testRational()
{
    {
        Rational r;

        AssertEqual(r.Numerator(), 0, "default 0");
        AssertEqual(r.Denominator(), 1, "default 1");
    }

    {
        Rational r(2, 3);

        AssertEqual(r.Numerator(), 2, "simple 0");
        AssertEqual(r.Denominator(), 3, "simple 1");
    }

    {
        Rational r(3, 6);

        AssertEqual(r.Numerator(), 1, "not simple 0");
        AssertEqual(r.Denominator(), 2, "not simple 1");
    }

    {
        Rational r(25, 30);

        AssertEqual(r.Numerator(), 5, "not simple 2");
        AssertEqual(r.Denominator(), 6, "not simple 3");
    }

    {
        Rational r(-2, -3);

        AssertEqual(r.Numerator(), 2, "negative simple 0");
        AssertEqual(r.Denominator(), 3, "negative simple 1");
    }

    {
        Rational r(36, -44);

        AssertEqual(r.Numerator(), -9, "negative not simple 0");
        AssertEqual(r.Denominator(), 11, "negative not simple 1");
    }

    {
        Rational r(-36, 44);

        AssertEqual(r.Numerator(), -9, "negative not simple 2");
        AssertEqual(r.Denominator(), 11, "negative not simple 3");
    }

    {
        Rational r(0, 44);

        AssertEqual(r.Numerator(), 0, "num 0 0");
        AssertEqual(r.Denominator(), 1, "num 0 1");
    }
}

int main()
{
    TestRunner runner;

    runner.RunTest(testRational, "testRational");
    return 0;
}
