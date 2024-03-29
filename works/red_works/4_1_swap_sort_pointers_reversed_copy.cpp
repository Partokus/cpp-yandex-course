#include <profile.h>
#include <test_runner.h>
#include <iomanip>
#include <iostream>
#include <queue>
#include <set>
#include <string>
#include <stdexcept>
#include <numeric>
#include <algorithm>
#include <array>

using namespace std;

void TestAll();
void Profile();

template <typename T>
void Swap(T *first, T *second)
{
    T temp = *first;
    *first = *second;
    *second = temp;
}

template <typename T>
void SortPointers(vector<T *> &pointers)
{
    sort(pointers.begin(), pointers.end(), [](const T *a, const T *b)
         { return *a < *b; });
}

template <typename T>
void ReversedCopy(T *source, size_t count, T *destination)
{
    // первый элемент [destination; destination + count] пересекается с [source; source + count]
    bool dest_begin_intersect = destination >= source and destination < (source + count);
    // последний элемент [destination; destination + count] пересекается с [source; source + count]
    bool dest_rbegin_intersect = (destination + count - 1) >= source and (destination + count - 1) < (source + count);

    if (dest_begin_intersect or dest_rbegin_intersect)
    {
        T *intersect_begin = dest_begin_intersect ? destination : source;
        T *intersect_rbegin = dest_begin_intersect ? (source + count - 1) : (destination + count - 1);

        T *intersect_left = intersect_begin;
        T *intersect_right = intersect_rbegin;

        while (intersect_left < intersect_right)
        {
            Swap(intersect_left, intersect_right);
            ++intersect_left;
            --intersect_right;
        }

        if (dest_begin_intersect)
        {
            for (size_t i = 0; (i + source) != intersect_begin; ++i)
            {
                destination[count - 1 - i] = source[i];
            }
        }
        else
        {
            for (size_t i = 0; (i + destination) != intersect_begin; ++i)
            {
                destination[i] = source[count - 1 - i];
            }
        }
    }
    else
    {
        reverse_copy(source, source + count, destination);
    }
}

int main()
{
    TestAll();
    Profile();

    return 0;
}

void TestSwap()
{
    int a = 1;
    int b = 2;
    Swap(&a, &b);
    ASSERT_EQUAL(a, 2);
    ASSERT_EQUAL(b, 1);

    string h = "world";
    string w = "hello";
    Swap(&h, &w);
    ASSERT_EQUAL(h, "hello");
    ASSERT_EQUAL(w, "world");
}

void TestSortPointers()
{
    int one = 1;
    int two = 2;
    int three = 3;

    vector<int *> pointers;
    pointers.push_back(&two);
    pointers.push_back(&three);
    pointers.push_back(&one);

    SortPointers(pointers);

    ASSERT_EQUAL(pointers.size(), 3u);
    ASSERT_EQUAL(*pointers[0], 1);
    ASSERT_EQUAL(*pointers[1], 2);
    ASSERT_EQUAL(*pointers[2], 3);
}

void TestReverseCopy()
{
    const size_t count = 7;

    int *source = new int[count];
    int *dest = new int[count];

    for (size_t i = 0; i < count; ++i)
    {
        source[i] = i + 1;
    }
    ReversedCopy(source, count, dest);
    const vector<int> expected1 = {7, 6, 5, 4, 3, 2, 1};
    // ASSERT_EQUAL(vector<int>(dest, dest + count), expected1);
    // {1, 2, 3, 4, 5, 6, 7};
    // Области памяти могут перекрываться
    ReversedCopy(source, count - 1, source + 1);
    const vector<int> expected2 = {1, 6, 5, 4, 3, 2, 1};
    ASSERT_EQUAL(vector<int>(source, source + count), expected2);

    for (size_t i = 0; i < count; ++i)
    {
        source[i] = i + 1;
    }
    ReversedCopy(source + 1, count - 1, source);
    const vector<int> expected3 = {7, 6, 5, 4, 3, 2, 7};
    ASSERT_EQUAL(vector<int>(source, source + count), expected3);

    for (size_t i = 0; i < count; ++i)
    {
        source[i] = i + 1;
    }
    ReversedCopy(source, count, source);
    const vector<int> expected4 = {7, 6, 5, 4, 3, 2, 1};
    ASSERT_EQUAL(vector<int>(source, source + count), expected4);

    delete[] dest;
    delete[] source;
}

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, TestSwap);
    RUN_TEST(tr, TestSortPointers);
    RUN_TEST(tr, TestReverseCopy);
}

void Profile()
{
}