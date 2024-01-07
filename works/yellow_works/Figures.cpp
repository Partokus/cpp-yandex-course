#include <test_runner.h>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <optional>
#include <tuple>
#include <ctime>
#include <cstring>
#include <iomanip>
#include <string_view>
#include <memory>

using namespace std;

class Figure
{
public:
    virtual ~Figure() = default;

    virtual string Name() const = 0;
    virtual double Perimeter() const = 0;
    virtual double Area() const = 0;
};

class Triangle : public Figure
{
public:
    Triangle(unsigned int a, unsigned int b, unsigned int c)
    {
        _perimeter = a + b + c;
        double half_p = _perimeter / 2.0;
        _area = sqrt(half_p * (half_p - a) * (half_p - b) * (half_p - c));
    }
    ~Triangle() override = default;

    string Name() const override { return NameVar; }
    double Perimeter() const override { return _perimeter; }
    double Area() const override { return _area; }

private:
    const string NameVar = "TRIANGLE";
    double _perimeter = 0.0;
    double _area = 0.0;
};

class Rect : public Figure
{
public:
    Rect(unsigned int a, unsigned int b)
    {
        _perimeter = 2 * (a + b);
        _area = a * b;
    }
    ~Rect() override = default;

    string Name() const override { return NameVar; }
    double Perimeter() const override { return _perimeter; }
    double Area() const override { return _area; }

private:
    const string NameVar = "RECT";
    double _perimeter = 0.0;
    double _area = 0.0;
};

class Circle : public Figure
{
public:
    Circle(double radius)
    {
        static constexpr double Pi = 3.14;
        _perimeter = 2.0 * Pi * radius;
        _area = Pi * radius * radius;
    }
    ~Circle() override = default;

    string Name() const override { return NameVar; }
    double Perimeter() const override { return _perimeter; }
    double Area() const override { return _area; }

private:
    const string NameVar = "CIRCLE";
    double _perimeter = 0.0;
    double _area = 0.0;
};

shared_ptr<Figure> CreateFigure(istringstream &is)
{
    shared_ptr<Figure> result{};

    string figure{};
    is >> figure;

    if (figure == "TRIANGLE")
    {
        unsigned int a = 0;
        unsigned int b = 0;
        unsigned int c = 0;

        is >> a >> b >> c;

        result = make_shared<Triangle>(a, b, c);
    }
    else if (figure == "RECT")
    {
        unsigned int a = 0;
        unsigned int b = 0;

        is >> a >> b;

        result = make_shared<Rect>(a, b);
    }
    else if (figure == "CIRCLE")
    {
        double radius = 0.0;

        is >> radius;

        result = make_shared<Circle>(radius);
    }
    return result;
}

void TestTriangle()
{
    {
        Triangle t(5, 4, 7);
        AssertEqual(t.Perimeter(), 16U, "Triangle 0");
        Assert(9.79 <= t.Area() and t.Area() <= 9.81, "Triangle 1");
    }
}

void TestRect()
{
    {
        Rect r(5, 4);
        AssertEqual(r.Perimeter(), 18U, "Rect 0");
        Assert(19.9 <= r.Area() and r.Area() <= 20.1, "Rect 1");
    }
}

void TestCircle()
{
    {
        Circle c(5.0);
        Assert(31.3 <= c.Perimeter() and c.Perimeter() <= 31.5, "Circle 0");
        Assert(78.4 <= c.Area() and c.Area() <= 78.6, "Circle 1");
    }
}

void TestAll()
{
    TestRunner tr{};
    tr.RunTest(TestTriangle, "TestTriangle");
    tr.RunTest(TestRect, "TestRect");
    tr.RunTest(TestCircle, "TestCircle");
}

int main()
{
    TestAll();

    vector<shared_ptr<Figure>> figures;
    for (string line; getline(cin, line);)
    {
        istringstream is(line);

        string command;
        is >> command;
        if (command == "ADD")
        {
            figures.push_back(CreateFigure(is));
        }
        else if (command == "PRINT")
        {
            for (const auto &current_figure : figures)
            {
                cout << fixed << setprecision(3)
                     << current_figure->Name() << " "
                     << current_figure->Perimeter() << " "
                     << current_figure->Area() << endl;
            }
        }
    }
    return 0;
}
