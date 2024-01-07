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

using namespace std;

class Figure
{
public:
    Figure(const string &name)
        : Name(name)
    {
    }
    virtual ~Figure() = default;

    string Name() const { return Name; }
    double Perimeter() const { return _perimeter; }
    double Area() const { return _area; }

protected:
    const string Name;
    double _perimeter = 0;
    double _area = 0;
};

class Triangle : public Figure
{
public:
    Triangle()
        : Figure("TRIANGLE")
    {
    }
    ~Triangle() override = default;
};

class Rect : public Figure
{
public:
    Rect()
        : Figure("RECT")
    {
    }
    ~Rect() override = default;
};

class Circle : public Figure
{
public:
    Circle()
        : Figure("CIRCLE")
    {
    }
    ~Circle() override = default;
};

int main()
{
    // vector<shared_ptr<Figure>> figures;
    // for (string line; getline(cin, line);)
    // {
    //     istringstream is(line);

    //     string command;
    //     is >> command;
    //     if (command == "ADD")
    //     {
    //         figures.push_back(CreateFigure(is));
    //     }
    //     else if (command == "PRINT")
    //     {
    //         for (const auto &current_figure : figures)
    //         {
    //             cout << fixed << setprecision(3)
    //                  << current_figure->Name() << " "
    //                  << current_figure->Perimeter() << " "
    //                  << current_figure->Area() << endl;
    //         }
    //     }
    // }
    return 0;
}
