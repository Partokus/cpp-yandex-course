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
#include <limits>

using namespace std;

int main()
{
    unsigned int N = 0;
    int64_t sum = 0;

    cin >> N;

    vector<int64_t> temperatures(N);

    for (auto &temp : temperatures)
    {
        cin >> temp;
        sum += temp;
    }

    int64_t average_temp = sum / static_cast<int>(N);

    vector<unsigned int> above_average_indices = {};
    for (size_t i = 0; i < temperatures.size(); ++i)
    {
        if (temperatures.at(i) > average_temp)
        {
            above_average_indices.push_back(i);
        }
    }

    cout << above_average_indices.size() << endl;
    for (const auto &i : above_average_indices)
    {
        cout << i << " ";
    }
    return 0;
}
