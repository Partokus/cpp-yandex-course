#include <profile.h>
#include <test_runner.h>
#include <vector>
#include <algorithm>
#include <numeric>
#include <iterator>
#include <cmath>
#include <future>

using namespace std;

void TestAll();
void Profile();

template <class Iterator>
class IteratorRange
{
public:
    IteratorRange(Iterator begin, Iterator end)
        : _begin(begin),
          _end(end)
    {
    }

    Iterator begin()
    {
        return _begin;
    }

    Iterator end()
    {
        return _end;
    }

    Iterator begin() const
    {
        return _begin;
    }

    Iterator end() const
    {
        return _end;
    }

    size_t size() const
    {
        return _end - _begin;
    }

private:
    Iterator _begin{};
    Iterator _end{};
};

template <class Iterator>
class Paginator
{
public:
    using Page = IteratorRange<Iterator>;

    Paginator(Iterator begin, Iterator end, size_t page_size)
    {
        _pages.reserve(std::ceil(static_cast<double>(end - begin) / page_size));

        Iterator it = begin;
        const size_t last_page_num = _pages.capacity() - 1;

        for (size_t page_num = 0; page_num < _pages.capacity(); ++page_num)
        {
            if (page_num != last_page_num)
            {
                _pages.push_back({it, it + page_size});
                it += page_size;
            }
            else
            {
                _pages.push_back({it, end});
            }
        }
    }

    typename vector<Page>::iterator begin()
    {
        return _pages.begin();
    }

    typename vector<Page>::iterator end()
    {
        return _pages.end();
    }

    typename vector<Page>::iterator begin() const
    {
        return _pages.begin();
    }

    typename vector<Page>::iterator end() const
    {
        return _pages.end();
    }

    size_t size() const
    {
        return _pages.size();
    }

private:
    vector<Page> _pages{};
};

int64_t CalculateMatrixSum(const vector<vector<int>> &matrix)
{
    static constexpr size_t ThreadsCount = 4U;

    int64_t result = 0;
    vector<future<int64_t>> futures;

    const size_t page_size = matrix.size() > ThreadsCount ? (matrix.size() / ThreadsCount) : matrix.size();

    for (const auto page : Paginator(matrix.begin(), matrix.end(), page_size))
    {
        futures.push_back(
            // page нужно опрокидывать по значению,
            // т.к. после выхода из цикла for ссылка инвалидируется
            async([page]{
                int64_t sum = 0;
                for (const auto &row : page)
                {
                    sum = accumulate(row.begin(), row.end(), sum);
                }
                return sum;
            })
        );
    }

    for (auto &f : futures)
    {
        result += f.get();
    }

    return result;
}

int main()
{
    TestAll();
    Profile();
    return 0;
}

void TestCalculateMatrixSum()
{
    {
        const vector<vector<int>> matrix = {
            {1, 2, 3, 4}};
        ASSERT_EQUAL(CalculateMatrixSum(matrix), 10);
    }
    {
        const vector<vector<int>> matrix = {
            {1, 2, 3, 4},
            {5, 6, 7, 8},
            {9, 10, 11, 12},
            {13, 14, 15, 16}};
        ASSERT_EQUAL(CalculateMatrixSum(matrix), 136);
    }
}

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, TestCalculateMatrixSum);
}

void Profile()
{
}