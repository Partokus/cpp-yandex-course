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

void EnsureEqual(const string &left, const string &right)
{
    if (left != right)
    {
        throw runtime_error(left + " != " + right);
    }
}

int main()
{
    try
    {
        EnsureEqual("C++ White", "C++ White");
        EnsureEqual("C++ White", "C++ Yellow");
    }
    catch (const runtime_error &e)
    {
        cout << e.what() << endl;
    }
    return 0;
}
