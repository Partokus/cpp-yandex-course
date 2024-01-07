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
    unsigned int blocks_num = 0; // 10^5 max

    uint64_t blocks_density = 0; // gram / santimeter^3. 100 max

    uint64_t result_weight = 0;

    cin >> blocks_num >> blocks_density;

    for (unsigned int i = 0; i < blocks_num; ++i)
    {
        // 10^4 max for each
        uint64_t block_width = 0;
        uint64_t block_lenght = 0;
        uint64_t block_height = 0;

        cin >> block_width >> block_lenght >> block_height;

        uint64_t volume = block_width * block_lenght * block_height;

        result_weight += volume * blocks_density;
    }

    cout << result_weight;

    return 0;
}
