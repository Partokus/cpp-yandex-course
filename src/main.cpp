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
class SimpleVector
{
public:
    SimpleVector() = default;
    explicit SimpleVector(size_t size);
    ~SimpleVector();

    T &operator[](size_t index);

    T *begin();
    T *end();

    size_t Size() const;
    size_t Capacity() const;
    void PushBack(const T &value);

private:
    T *_data = nullptr;
};

template <typename T>
SimpleVector<T>::SimpleVector(size_t size)
{
    _data = new T[size];
}

template <typename T>
SimpleVector<T>::~SimpleVector()
{
    delete[] _data;
}

int main()
{
    TestAll();
    Profile();

    return 0;
}

void TestAll()
{
    TestRunner tr{};
}

void Profile()
{
}