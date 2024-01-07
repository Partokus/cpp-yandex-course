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

using namespace std;

class Rational
{
public:
    Rational() = default;

    Rational(int numerator, int denominator)
    {
        if (numerator == 0)
        {
            return;
        }

        if (numerator < 0 and denominator < 0)
        {
            _numerator = abs(numerator);
            _denominator = abs(denominator);
        }
        else if (denominator < 0)
        {
            _numerator = -numerator;
            _denominator = abs(denominator);
        }
        else
        {
            _numerator = numerator;
            _denominator = denominator;
        }

        int gcd_var = gcd(_numerator, _denominator);
        _numerator /= gcd_var;
        _denominator /= gcd_var;
    }

    int Numerator() const { return _numerator; }
    int Denominator() const { return _denominator; }

private:
    int _numerator = 0;
    int _denominator = 1;
};

bool operator==(const Rational &lhs, const Rational &rhs)
{
    return lhs.Numerator() == rhs.Numerator() and lhs.Denominator() == rhs.Denominator();
}

/**
 * * a / b + c / d = (d * a) + (b * c) / (b * d)
 */
Rational operator+(const Rational &lhs, const Rational &rhs)
{
    return {rhs.Denominator() * lhs.Numerator() + lhs.Denominator() * rhs.Numerator(), lhs.Denominator() * rhs.Denominator()};
}

/**
 * * a / b - c / d = (d * a) - (b * c) / (b * d)
 */
Rational operator-(const Rational &lhs, const Rational &rhs)
{
    return {rhs.Denominator() * lhs.Numerator() - lhs.Denominator() * rhs.Numerator(), lhs.Denominator() * rhs.Denominator()};
}

/**
 * * (a / b) * (c / d) = (a * c) / (b * d)
 */
Rational operator*(const Rational &lhs, const Rational &rhs)
{
    return {lhs.Numerator() * rhs.Numerator(), lhs.Denominator() * rhs.Denominator()};
}

/**
 * * (a / b) / (c / d) = (a * d) / (b * c)
 */
Rational operator/(const Rational &lhs, const Rational &rhs)
{
    return {lhs.Numerator() * rhs.Denominator(), lhs.Denominator() * rhs.Numerator()};
}

ostream &operator<<(ostream &os, const Rational &r)
{
    os << r.Numerator() << "/" << r.Denominator();
    return os;
}

istream &operator>>(istream &is, Rational &r)
{
    int numenator = 0;
    int denominator = 0;
    char devider{};

    if (is)
    {
        is >> numenator >> devider >> denominator;
        if (devider == '/')
        {
            r = {numenator, denominator};
        }
    }
    return is;
}

/**
 * * сначала приравниваем знаменатели, затем сравниваем числители
 */
bool operator<(const Rational &lhs, const Rational &rhs)
{
    return (lhs.Numerator() * rhs.Denominator()) < (rhs.Numerator() * lhs.Denominator());
}

/**
 * * сначала приравниваем знаменатели, затем сравниваем числители
 */
bool operator>(const Rational &lhs, const Rational &rhs)
{
    return (lhs.Numerator() * rhs.Denominator()) > (rhs.Numerator() * lhs.Denominator());
}

int main()
{
    {
        const set<Rational> rs = {{1, 2}, {1, 25}, {3, 4}, {3, 4}, {1, 2}};
        if (rs.size() != 3)
        {
            cout << "Wrong amount of items in the set" << endl;
            return 1;
        }

        vector<Rational> v;
        for (auto x : rs)
        {
            v.push_back(x);
        }
        if (v != vector<Rational>{{1, 25}, {1, 2}, {3, 4}})
        {
            cout << "Rationals comparison works incorrectly" << endl;
            return 2;
        }
    }

    {
        map<Rational, int> count;
        ++count[{1, 2}];
        ++count[{1, 2}];

        ++count[{2, 3}];

        if (count.size() != 2)
        {
            cout << "Wrong amount of items in the map" << endl;
            return 3;
        }
    }

    cout << "OK" << endl;
    return 0;
}
