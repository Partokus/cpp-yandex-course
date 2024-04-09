#include <algorithm>

using namespace std;

template <typename T>
class SimpleVector
{
public:
    SimpleVector() = default;
    explicit SimpleVector(size_t size);               // default ctor
    explicit SimpleVector(const SimpleVector &other); // copy ctor
    void operator=(const SimpleVector &other);        // copy assignment ctor
    ~SimpleVector();

    T &operator[](size_t index);

    T *begin();
    T *end();
    const T *begin() const;
    const T *end() const;

    size_t Size() const;
    size_t Capacity() const;
    void PushBack(const T &value);

private:
    T *_data = nullptr;
    T *_end = nullptr;
    size_t _size = 0U;
    size_t _capacity = 0U;
};

template <typename T>
SimpleVector<T>::SimpleVector(size_t size)
    : _capacity(size)
{
    _data = new T[size];
    _end = _data + size;
    _size = size;
    _capacity = size;
}

template <typename T>
SimpleVector<T>::SimpleVector(const SimpleVector<T> &other)
    : _data(new T[other._capacity]),
      _size(other._size),
      _capacity(other._capacity)
{
    copy(other.begin(), other.end(), begin());
}

template <typename T>
void SimpleVector<T>::operator=(const SimpleVector<T> &other)
{
    if (_capacity >= other._capacity)
    {
        // У нас достаточно памяти - просто копируем элементы
        copy(other.begin(), other.end(), begin());
        _size = other._size;
    }
    else
    {
        // Это так называемая идиома copy-and-swap. Мы создаём временный вектор с помощью
        // конструктора копирования, а затем обмениваем его поля со своими. Так мы достигаем
        // двух целей:
        //  - избегаем дублирования кода в конструкторе копирования и операторе присваивания
        //  - обеспечиваем согласованное поведение конструктора копирования и оператора присваивания
        SimpleVector<T> tmp(other);
        swap(tmp._data, _data);
        swap(tmp._size, _size);
        swap(tmp._capacity, _capacity);
    }
}

template <typename T>
SimpleVector<T>::~SimpleVector()
{
    delete[] _data;
}

template <typename T>
T *SimpleVector<T>::begin() { return _data; }
template <typename T>
T *SimpleVector<T>::end() { return _data + _size; }
template <typename T>
const T *SimpleVector<T>::begin() const { return _data; }
template <typename T>
const T *SimpleVector<T>::end() const { return _data + _size; }

template <typename T>
T &SimpleVector<T>::operator[](size_t index)
{
    return _data[index];
}

template <typename T>
size_t SimpleVector<T>::Size() const { return _size; }
template <typename T>
size_t SimpleVector<T>::Capacity() const { return _capacity; }

template <typename T>
void SimpleVector<T>::PushBack(const T &value)
{
    if (_size == _capacity)
    {
        if (_capacity == 0U)
        {
            _capacity = 1U;
            _data = new T[1U];
        }
        else
        {
            _capacity *= 2U;
            T *new_data = new T[_capacity];
            copy(_data, _data + _size, new_data);
            delete[] _data;
            _data = new_data;
        }
    }
    _data[_size] = value;
    ++_size;
}

// void TestCopyAssignment()
// {
//     SimpleVector<int> numbers(10);
//     iota(numbers.begin(), numbers.end(), 1);

//     SimpleVector<int> dest;
//     ASSERT_EQUAL(dest.Size(), 0u);

//     dest = numbers;
//     ASSERT_EQUAL(dest.Size(), numbers.Size());
//     ASSERT(dest.Capacity() >= dest.Size());
//     ASSERT(equal(dest.begin(), dest.end(), numbers.begin()));
// }