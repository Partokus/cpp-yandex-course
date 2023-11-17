#include <test_runner.h>
#include <algorithm>
#include <numeric>
#include <cmath>

using namespace std;

void TestAll()
{
    TestRunner tr = {};
}

template <typename RandomIt>
void MergeSort(RandomIt range_begin, RandomIt range_end)
{
    auto elements_number = range_end - range_begin;

    if (elements_number < 2)
    {
        return;
    }

    vector<typename RandomIt::value_type> elements(range_begin, range_end);

    auto middle_it = begin(elements) + elements_number / 2;

    MergeSort(begin(elements), middle_it);
    MergeSort(middle_it, end(elements));

    merge(begin(elements), middle_it, middle_it, end(elements), range_begin);
}

// int main()
// {
//     vector<int> v = {6, 4, 7, 6, 4, 4, 0, 1};
//     MergeSort(begin(v), end(v));
//     for (int x : v)
//     {
//         cout << x << " ";
//     }
//     cout << endl;
//     return 0;
// }
