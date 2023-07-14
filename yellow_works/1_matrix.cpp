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

struct Matrix
{
    Matrix() = default;

    Matrix(int num_rows, int num_cols)
    {
        if (num_rows < 0 or num_cols < 0)
        {
            throw out_of_range("out_of_range");
        }
        else if (num_rows == 0 or num_cols == 0)
        {
            return;
        }

        _matrix.resize(num_rows);
        for (auto &col : _matrix)
        {
            col.resize(num_cols);
        }
    }

    void Reset(int num_rows, int num_cols)
    {
        if (num_rows < 0 or num_cols < 0)
        {
            throw out_of_range("out_of_range");
        }
        else if (num_rows == 0 or num_cols == 0)
        {
            _matrix.resize(0);
            return;
        }

        _matrix.resize(num_rows);
        for (auto &col : _matrix)
        {
            col.assign(num_cols, 0);
        }
    }

    int At(int row_num, int col_num) const
    {
        if (GetNumRows() == 0 or GetNumColumns() == 0 or
            row_num < 0 or col_num < 0 or
            row_num >= GetNumRows() or col_num >= GetNumColumns())
        {
            throw out_of_range("out_of_range");
        }
        return _matrix.at(row_num).at(col_num);
    }

    int &At(int row_num, int col_num)
    {
        if (GetNumRows() == 0 or GetNumColumns() == 0 or
            row_num < 0 or col_num < 0 or
            row_num >= GetNumRows() or col_num >= GetNumColumns())
        {
            throw out_of_range("out_of_range");
        }
        return _matrix[row_num][col_num];
    }

    int GetNumRows() const
    {
        return _matrix.size();
    }

    int GetNumColumns() const
    {
        if (not _matrix.empty())
        {
            return _matrix.at(0).size();
        }
        return 0;
    }

    vector<vector<int>> _matrix = {};
};

istream &operator>>(istream &is, Matrix &matrix)
{
    int row_num = 0;
    int col_num = 0;

    is >> row_num;
    is.ignore(1);
    is >> col_num;

    matrix.Reset(row_num, col_num);

    for (int row = 0; row < row_num; ++row)
    {
        for (int col = 0; col < col_num; ++col)
        {
            int elem = 0;
            is >> elem;
            matrix.At(row, col) = elem;
        }
    }
    return is;
}

ostream &operator<<(ostream &os, const Matrix &matrix)
{
    os << matrix.GetNumRows() << " " << matrix.GetNumColumns() << endl;

    for (int row = 0; row < matrix.GetNumRows(); ++row)
    {
        for (int col = 0; col < matrix.GetNumColumns(); ++col)
        {
            os << matrix.At(row, col);
            if (col != (matrix.GetNumColumns() - 1))
            {
                os << " ";
            }
        }
        if (row != (matrix.GetNumRows() - 1))
        {
            os << endl;
        }
    }
    return os;
}

bool operator==(const Matrix &lhs, const Matrix &rhs)
{
    return lhs._matrix == rhs._matrix;
}

Matrix operator+(const Matrix &lhs, const Matrix &rhs)
{
    if (lhs.GetNumRows() != rhs.GetNumRows() or lhs.GetNumColumns() != rhs.GetNumColumns())
    {
        throw invalid_argument("invalid_argument");
    }

    int row_num = lhs.GetNumRows();
    int col_num = lhs.GetNumColumns();

    Matrix result(row_num, col_num);

    for (int row = 0; row < row_num; ++row)
    {
        for (int col = 0; col < col_num; ++col)
        {
            result.At(row, col) = lhs.At(row, col) + rhs.At(row, col);
        }
    }
    return result;
}

int main()
{
    try
    {
        Matrix matrix(3, 3);

        cin >> matrix;

        matrix.Reset(3, 3);

        cout << matrix;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
    return 0;
}
