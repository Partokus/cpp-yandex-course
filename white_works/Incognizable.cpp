#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <optional>

using namespace std;

class Incognizable
{
public:
    Incognizable() {}
    Incognizable(int a) {}
    Incognizable(int a, int b) {}
};

int main()
{
    Incognizable a;
    Incognizable b = {};
    Incognizable c = {0};
    Incognizable d = {0, 1};
    return 0;
}
