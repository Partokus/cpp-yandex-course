#include <profile.h>
#include <test_runner.h>
#include <iomanip>
#include <iostream>
#include <set>
#include <map>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <string_view>
#include <random>
#include <list>
#include <iterator>
#include <numeric>

using namespace std;

void TestAll();
void Profile();

template <typename RandomIt>
void MakeJosephusPermutation(RandomIt first, RandomIt last, uint32_t step_size)
{
    list<typename RandomIt::value_type> pool(make_move_iterator(first), make_move_iterator(last));

    auto cur_pos_it = pool.begin();
    size_t cur_pos = 0;
    while (!pool.empty())
    {
        *(first++) = move(*cur_pos_it);
        cur_pos_it = pool.erase(cur_pos_it);
        if (pool.empty())
        {
            break;
        }

        size_t last_pos = cur_pos;
        cur_pos = (cur_pos + step_size - 1) % pool.size();

        if (last_pos > cur_pos)
        {
            cur_pos_it = pool.begin();
            advance(cur_pos_it, cur_pos);
        }
        else
        {
            advance(cur_pos_it, cur_pos - last_pos);
        }
    }
}

int main()
{
    TestAll();
    Profile();
    return 0;
}

// Это специальный тип, который поможет вам убедиться, что ваша реализация
// функции MakeJosephusPermutation не выполняет копирование объектов.
// Сейчас вы, возможно, не понимаете как он устроен, однако мы расскажем,
// почему он устроен именно так, далее в блоке про move-семантику —
// в видео «Некопируемые типы»
struct NoncopyableInt
{
    int value;

    NoncopyableInt(const NoncopyableInt &) = delete;
    NoncopyableInt &operator=(const NoncopyableInt &) = delete;

    NoncopyableInt(NoncopyableInt &&) = default;
    NoncopyableInt &operator=(NoncopyableInt &&) = default;
};

vector<int> MakeTestVector()
{
    vector<int> numbers(10);
    iota(begin(numbers), end(numbers), 0);
    return numbers;
}

void TestIntVector()
{
    const vector<int> numbers = MakeTestVector();
    {
        vector<int> numbers_copy = numbers;
        MakeJosephusPermutation(begin(numbers_copy), end(numbers_copy), 1);
        ASSERT_EQUAL(numbers_copy, numbers);
    }
    {
        vector<int> numbers_copy = numbers;
        MakeJosephusPermutation(begin(numbers_copy), end(numbers_copy), 3);
        ASSERT_EQUAL(numbers_copy, vector<int>({0, 3, 6, 9, 4, 8, 5, 2, 7, 1}));
    }
}

bool operator==(const NoncopyableInt &lhs, const NoncopyableInt &rhs)
{
    return lhs.value == rhs.value;
}

ostream &operator<<(ostream &os, const NoncopyableInt &v)
{
    return os << v.value;
}

void TestAvoidsCopying()
{
    vector<NoncopyableInt> numbers;
    numbers.push_back({1});
    numbers.push_back({2});
    numbers.push_back({3});
    numbers.push_back({4});
    numbers.push_back({5});

    MakeJosephusPermutation(begin(numbers), end(numbers), 2);

    vector<NoncopyableInt> expected;
    expected.push_back({1});
    expected.push_back({3});
    expected.push_back({5});
    expected.push_back({4});
    expected.push_back({2});

    ASSERT_EQUAL(numbers, expected);
}

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, TestIntVector);
    RUN_TEST(tr, TestAvoidsCopying);
}

void Profile()
{
    // const size_t Size = 1000'000;
    // const string s = "hello";
    // {
    //     deque<string> strings;

    //     {
    //         strings.assign(Size, s);

    //         LOG_DURATION("deque pop_front");

    //         while (not strings.empty())
    //         {
    //             strings.pop_front();
    //         }
    //     }
    //     {
    //         strings.assign(Size, s);

    //         LOG_DURATION("deque erase");

    //         while (not strings.empty())
    //         {
    //             strings.erase(strings.begin());
    //         }
    //     }
    // }
}
