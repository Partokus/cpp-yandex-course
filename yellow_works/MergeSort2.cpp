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

    const size_t one_part_lenght = elements_number / 3;
    auto begin_second_part = begin(elements) + one_part_lenght;
    auto begin_third_part = begin_second_part + one_part_lenght;

    MergeSort(begin(elements), begin_second_part);
    MergeSort(begin_second_part, begin_third_part);
    MergeSort(begin_third_part, end(elements));

    vector<typename RandomIt::value_type> first_two_parts;
    merge(begin(elements), begin_second_part, begin_second_part, begin_third_part, back_inserter(first_two_parts));

    merge(begin(first_two_parts), end(first_two_parts), begin_third_part, end(elements), range_begin);
}

// int main()
// {
//     vector<int> v = {6, 4, 7, 6, 4, 4, 0, 1, 5};
//     MergeSort(begin(v), end(v));
//     for (int x : v)
//     {
//         cout << x << " ";
//     }
//     cout << endl;
//     return 0;
// }
