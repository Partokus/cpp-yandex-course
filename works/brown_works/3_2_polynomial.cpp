#include <profile.h>
#include <test_runner.h>

#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <utility>
#include <map>
#include <optional>
#include <unordered_set>
#include <algorithm>
#include <numeric>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <cmath>
#include <random>

using namespace std;

void TestAll();
void Profile();

template <typename T>
void PrintCoeff(std::ostream &out, int i, const T &coef, bool printed)
{
    bool coeffPrinted = false;
    if (coef == 1 && i > 0)
    {
        out << (printed ? "+" : "");
    }
    else if (coef == -1 && i > 0)
    {
        out << "-";
    }
    else if (coef >= 0 && printed)
    {
        out << "+" << coef;
        coeffPrinted = true;
    }
    else
    {
        out << coef;
        coeffPrinted = true;
    }
    if (i > 0)
    {
        out << (coeffPrinted ? "*" : "") << "x";
    }
    if (i > 1)
    {
        out << "^" << i;
    }
}

template <typename T>
class Polynomial
{
private:
    vector<T> _coeffs = {0};

    void Shrink()
    {
        while (_coeffs.size() > 1 && _coeffs.back() == 0)
        {
            _coeffs.pop_back();
        }
    }

    class IndexProxy
    {
    public:
        IndexProxy(Polynomial &poly, size_t degree)
            : _poly(poly),
              _degree(degree)
        {
        }

        IndexProxy & operator=(T value)
        {
            if (_degree >= _poly._coeffs.size())
            {
                if (value != 0)
                {
                    _poly._coeffs.resize(_degree + 1U);
                    _poly._coeffs[_degree] = value;
                }
            }
            else
            {
                _poly._coeffs[_degree] = value;
            }
            _poly.Shrink();
            return *this;
        }

        operator T() const
        {
            return std::as_const(_poly)[_degree];
        }

    private:
        Polynomial &_poly;
        size_t _degree;
    };

public:
    Polynomial() = default;
    Polynomial(vector<T> coeffs) : _coeffs(std::move(coeffs))
    {
        Shrink();
    }

    template <typename Iterator>
    Polynomial(Iterator first, Iterator last) : _coeffs(first, last)
    {
        Shrink();
    }

    bool operator==(const Polynomial &other) const
    {
        return _coeffs == other._coeffs;
    }

    bool operator!=(const Polynomial &other) const
    {
        return !operator==(other);
    }

    int Degree() const
    {
        return _coeffs.size() - 1;
    }

    Polynomial &operator+=(const Polynomial &r)
    {
        if (r._coeffs.size() > _coeffs.size())
        {
            _coeffs.resize(r._coeffs.size());
        }
        for (size_t i = 0; i != r._coeffs.size(); ++i)
        {
            _coeffs[i] += r._coeffs[i];
        }
        Shrink();
        return *this;
    }

    Polynomial &operator-=(const Polynomial &r)
    {
        if (r._coeffs.size() > _coeffs.size())
        {
            _coeffs.resize(r._coeffs.size());
        }
        for (size_t i = 0; i != r._coeffs.size(); ++i)
        {
            _coeffs[i] -= r._coeffs[i];
        }
        Shrink();
        return *this;
    }

    T operator[](size_t degree) const
    {
        return degree < _coeffs.size() ? _coeffs[degree] : 0;
    }

    IndexProxy operator[](size_t degree)
    {
        return {*this, degree};
    }

    T operator()(const T &x) const
    {
        T res = 0;
        for (auto it = _coeffs.rbegin(); it != _coeffs.rend(); ++it)
        {
            res *= x;
            res += *it;
        }
        return res;
    }

    using const_iterator = typename std::vector<T>::const_iterator;

    const_iterator begin() const
    {
        return _coeffs.cbegin();
    }

    const_iterator end() const
    {
        return _coeffs.cend();
    }
};

template <typename T>
std::ostream &operator<<(std::ostream &out, const Polynomial<T> &p)
{
    bool printed = false;
    for (int i = p.Degree(); i >= 0; --i)
    {
        if (p[i] != 0)
        {
            PrintCoeff(out, i, p[i], printed);
            printed = true;
        }
    }
    return out;
}

template <typename T>
Polynomial<T> operator+(Polynomial<T> lhs, const Polynomial<T> &rhs)
{
    lhs += rhs;
    return lhs;
}

template <typename T>
Polynomial<T> operator-(Polynomial<T> lhs, const Polynomial<T> &rhs)
{
    lhs -= rhs;
    return lhs;
}

int main()
{
    TestAll();
    Profile();

    return 0;
}

void TestCreation()
{
    {
        Polynomial<int> default_constructed;
        ASSERT_EQUAL(default_constructed.Degree(), 0);
        ASSERT_EQUAL(default_constructed[0], 0);
    }
    {
        Polynomial<double> from_vector({1.0, 2.0, 3.0, 4.0});
        ASSERT_EQUAL(from_vector.Degree(), 3);
        ASSERT_EQUAL(from_vector[0], 1.0);
        ASSERT_EQUAL(from_vector[1], 2.0);
        ASSERT_EQUAL(from_vector[2], 3.0);
        ASSERT_EQUAL(from_vector[3], 4.0);
    }
    {
        const vector<int> coeffs = {4, 9, 7, 8, 12};
        Polynomial<int> from_iterators(begin(coeffs), end(coeffs));
        ASSERT_EQUAL(from_iterators.Degree(), coeffs.size() - 1);
        ASSERT(std::equal(cbegin(from_iterators), cend(from_iterators), begin(coeffs)));
    }
}

void TestEqualComparability()
{
    Polynomial<int> p1({9, 3, 2, 8});
    Polynomial<int> p2({9, 3, 2, 8});
    Polynomial<int> p3({1, 2, 4, 8});

    ASSERT_EQUAL(p1, p2);
    ASSERT(p1 != p3);
}

void TestAddition()
{
    Polynomial<int> p1({9, 3, 2, 8});
    Polynomial<int> p2({1, 2, 4});

    p1 += p2;
    ASSERT_EQUAL(p1, Polynomial<int>({10, 5, 6, 8}));

    auto p3 = p1 + p2;
    ASSERT_EQUAL(p3, Polynomial<int>({11, 7, 10, 8}));

    p3 += Polynomial<int>({-11, 1, -10, -8});
    ASSERT_EQUAL(p3.Degree(), 1);
    const Polynomial<int> expected{{0, 8}};
    ASSERT_EQUAL(p3, expected);
}

void TestSubtraction()
{
    Polynomial<int> p1({9, 3, 2, 8});
    Polynomial<int> p2({1, 2, 4});

    p1 -= p2;
    ASSERT_EQUAL(p1, Polynomial<int>({8, 1, -2, 8}));

    auto p3 = p1 - p2;
    ASSERT_EQUAL(p3, Polynomial<int>({7, -1, -6, 8}));

    p3 -= Polynomial<int>({7, -5, -6, 8});
    ASSERT_EQUAL(p3.Degree(), 1);
    const Polynomial<int> expected{{0, 4}};
    ASSERT_EQUAL(p3, expected);
}

void TestEvaluation()
{
    const Polynomial<int> default_constructed;
    ASSERT_EQUAL(default_constructed(0), 0);
    ASSERT_EQUAL(default_constructed(1), 0);
    ASSERT_EQUAL(default_constructed(-1), 0);
    ASSERT_EQUAL(default_constructed(198632), 0);

    const Polynomial<int64_t> cubic({1, 1, 1, 1});
    ASSERT_EQUAL(cubic(0), 1);
    ASSERT_EQUAL(cubic(1), 4);
    ASSERT_EQUAL(cubic(2), 15);
    ASSERT_EQUAL(cubic(21), 9724);
}

void TestConstAccess()
{
    const Polynomial<int> poly(std::initializer_list<int>{1, 2, 3, 4, 5});

    ASSERT_EQUAL(poly[0], 1);
    ASSERT_EQUAL(poly[1], 2);
    ASSERT_EQUAL(poly[2], 3);
    ASSERT_EQUAL(poly[3], 4);
    ASSERT_EQUAL(poly[4], 5);
    ASSERT_EQUAL(poly[5], 0);
    ASSERT_EQUAL(poly[6], 0);
    ASSERT_EQUAL(poly[608], 0);
}

void TestNonconstAccess()
{
    Polynomial<int> poly;

    poly[0] = 1;
    poly[3] = 12;
    poly[5] = 4;
    ASSERT_EQUAL(poly.Degree(), 5);

    ASSERT_EQUAL(poly[0], 1);
    ASSERT_EQUAL(poly[1], 0);
    ASSERT_EQUAL(poly[2], 0);
    ASSERT_EQUAL(poly[3], 12);
    ASSERT_EQUAL(poly[4], 0);
    ASSERT_EQUAL(poly[5], 4);
    ASSERT_EQUAL(poly[6], 0);
    ASSERT_EQUAL(poly[608], 0);

    ASSERT_EQUAL(poly.Degree(), 5);

    poly[608] = 0;
    ASSERT_EQUAL(poly.Degree(), 5);

    {
        LOG_DURATION("Iteration over const");
        for (size_t i = 10; i < 50000; ++i)
        {
            ASSERT_EQUAL(std::as_const(poly)[i], 0);
            ASSERT_EQUAL(poly.Degree(), 5);
        }
    }
    {
        LOG_DURATION("Iteration over non-const");
        for (size_t i = 10; i < 50000; ++i)
        {
            ASSERT_EQUAL(poly[i], 0);
            ASSERT_EQUAL(poly.Degree(), 5);
        }
    }
}

void TestDegree()
{
    Polynomial<int> poly;

    poly[0] = 1;
    poly[3] = 12;
    poly[5] = 4;
    ASSERT_EQUAL(poly.Degree(), 5);

    auto it_before_degree_change = poly.end();
    poly[5] = 0;
    auto it_after_degree_change = poly.end();
    ASSERT_EQUAL(poly.Degree(), 3);
    ASSERT(it_before_degree_change != it_after_degree_change);
    ASSERT((poly.begin() + 4) == it_after_degree_change);
}

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, TestCreation);
    RUN_TEST(tr, TestEqualComparability);
    RUN_TEST(tr, TestAddition);
    RUN_TEST(tr, TestSubtraction);
    RUN_TEST(tr, TestEvaluation);
    RUN_TEST(tr, TestConstAccess);
    RUN_TEST(tr, TestNonconstAccess);
    RUN_TEST(tr, TestDegree);
}

void Profile()
{
}
