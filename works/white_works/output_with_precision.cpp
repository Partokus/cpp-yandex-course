#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <optional>

using namespace std;

int main()
{
    ifstream input("input.txt");

    if (input)
    {
        cout << fixed << setprecision(3);

        double number = 0;
        while (input >> number)
        {
            cout << number << endl;
        }
    }
    return 0;
}
