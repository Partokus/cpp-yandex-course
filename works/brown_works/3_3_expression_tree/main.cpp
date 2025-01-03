#include <profile.h>
#include <test_runner.h>

#include "Common.h"

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
#include <memory>

using namespace std;

void TestAll();
void Profile();

namespace expr
{

class Value : public Expression
{
public:
    Value(int value)
        : _value(value)
    {
    }

    int Evaluate() const override
    {
        return _value;
    }

    std::string ToString() const override
    {
        return to_string(_value);
    }

private:
    int _value = 0;
};

class Operation : public Expression
{
public:
    Operation(ExpressionPtr left, ExpressionPtr right, char operation)
        : _left(move(left)),
          _right(move(right)),
          _operation(operation)
    {
    }

    int Evaluate() const override
    {
        return _operation == '+' ? _left->Evaluate() + _right->Evaluate() :
                                   _left->Evaluate() * _right->Evaluate();
    }

    std::string ToString() const override
    {
        return '(' + _left->ToString() + ')' + _operation + '(' + _right->ToString() + ')';
    }

private:
    ExpressionPtr _left;
    ExpressionPtr _right;
    char _operation{};
};

}

ExpressionPtr Value(int value)
{
    return make_unique<expr::Value>(value);
}

ExpressionPtr Sum(ExpressionPtr left, ExpressionPtr right)
{
    return make_unique<expr::Operation>(move(left), move(right), '+');
}

ExpressionPtr Product(ExpressionPtr left, ExpressionPtr right)
{
    return make_unique<expr::Operation>(move(left), move(right), '*');
}

int main()
{
    TestAll();
    Profile();

    return 0;
}

string Print(const Expression *e)
{
    if (!e)
    {
        return "Null expression provided";
    }
    stringstream output;
    output << e->ToString() << " = " << e->Evaluate();
    return output.str();
}

void Test()
{
    ExpressionPtr e1 = Product(Value(2), Sum(Value(3), Value(4)));
    ASSERT_EQUAL(Print(e1.get()), "(2)*((3)+(4)) = 14");

    ExpressionPtr e2 = Sum(move(e1), Value(5));
    ASSERT_EQUAL(Print(e2.get()), "((2)*((3)+(4)))+(5) = 19");

    ASSERT_EQUAL(Print(e1.get()), "Null expression provided");
}

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, Test);
}

void Profile()
{
}
