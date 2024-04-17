#include <profile.h>
#include <iostream>
#include <test_runner.h>
#include <vector>
#include <string>
#include <algorithm>
#include <iterator>
#include <numeric>

using namespace std;

void TestAll();
void Profile();

template <typename RandomIt>
void MergeSort(RandomIt range_begin, RandomIt range_end)
{
    auto elements_number = range_end - range_begin;

    if (elements_number < 2)
    {
        return;
    }

    vector<typename RandomIt::value_type> elements(make_move_iterator(range_begin), make_move_iterator(range_end));

    const size_t one_part_lenght = elements_number / 3;
    auto begin_second_part = elements.begin() + one_part_lenght;
    auto begin_third_part = begin_second_part + one_part_lenght;

    MergeSort(elements.begin(), begin_second_part);
    MergeSort(begin_second_part, begin_third_part);
    MergeSort(begin_third_part, elements.end());

    vector<typename RandomIt::value_type> first_two_parts;
    merge(make_move_iterator(elements.begin()), make_move_iterator(begin_second_part),
          make_move_iterator(begin_second_part), make_move_iterator(begin_third_part),
          back_inserter(first_two_parts));

    merge(make_move_iterator(first_two_parts.begin()), make_move_iterator(first_two_parts.end()),
          make_move_iterator(begin_third_part), make_move_iterator(elements.end()),
          range_begin);
}

int main()
{
    TestAll();
    Profile();
    return 0;
}

void TestIntVector()
{
    vector<int> numbers = {6, 1, 3, 9, 1, 9, 8, 12, 1};
    MergeSort(begin(numbers), end(numbers));
    ASSERT(is_sorted(begin(numbers), end(numbers)));
}

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, TestIntVector);
}

void Profile()
{
}
