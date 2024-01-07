#include "test_runner.h"

#include <string>
#include <vector>
#include <numeric>
#include <iterator>
#include <algorithm>

void TestAll();

using namespace std;

template <class T>
class Table
{
public:
    Table(size_t row_count, size_t column_count)
        : _row_and_column(row_count)
    {
        for (auto &column : _row_and_column)
        {
            column.resize(column_count);
        }
    }

    vector<T> &operator[](size_t index)
    {
        return _row_and_column[index];
    }

    const vector<T> &operator[](size_t index) const
    {
        return _row_and_column.at(index);
    }

    std::pair<size_t, size_t> Size() const
    {
        if (_row_and_column.empty())
        {
            return {0U, 0U};
        }
        else if (_row_and_column.at(0).empty())
        {
            return {_row_and_column.size(), 0U};
        }
        return {_row_and_column.size(), _row_and_column.at(0).size()};
    }

    void Resize(size_t row_count, size_t column_count)
    {
        _row_and_column.resize(row_count);
        for (auto &column : _row_and_column)
        {
            column.resize(column_count);
        }
    }

private:
    vector<vector<T>> _row_and_column;
};

void TestTable()
{
    Table<int> t(1, 1);
    ASSERT_EQUAL(t.Size().first, 1u);
    ASSERT_EQUAL(t.Size().second, 1u);
    t[0][0] = 42;
    ASSERT_EQUAL(t[0][0], 42);
    t.Resize(3, 4);
    ASSERT_EQUAL(t.Size().first, 3u);
    ASSERT_EQUAL(t.Size().second, 4u);
}

int main()
{
    TestAll();

    return 0;
}

void TestAll()
{
    TestRunner tr;
    RUN_TEST(tr, TestTable);
}