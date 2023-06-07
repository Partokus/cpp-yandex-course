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
        ofstream output("output.txt");
        string line;

        while (getline(input, line))
        {
            output << line << endl;
        }
    }
    return 0;
}
