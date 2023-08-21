#include <iostream>
#include <cmath>
using namespace std;

int main()
{
    double a = 0;
    double b = 0;
    double c = 0;

    cin >> a >> b >> c;

    double d = pow(b, 2) - 4 * a * c;

    if (a != 0)
    {
        if (d > 0)
        {
            cout << (sqrt(d) - b) / (2.0 * a) << " " << (-sqrt(d) - b) / (2.0 * a);
        }
        else if (d == 0)
        {
            cout << (-b / (2.0 * a));
        }
    }
    else if (b != 0)
    {
        cout << (-c / b);
    }
    return 0;
}