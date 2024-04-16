#include <algorithm>

using namespace std;

template <typename T>
class SimpleVector
{
public:
    SimpleVector() = default;
    explicit SimpleVector(size_t size);               // default ctor
    explicit SimpleVector(const SimpleVector &other); // copy ctor
    explicit SimpleVector(SimpleVector &&other);      // move ctor
    void operator=(const SimpleVector &other);        // copy assignment ctor
    void operator=(SimpleVector &&other);        // move assignment ctor
    ~SimpleVector();

    T &operator[](size_t index);

    T *begin();
    T *end();
    const T *begin() const;
    const T *end() const;

    size_t Size() const;
    size_t Capacity() const;
    void PushBack(const T &value);
    void PushBack(T &&value);

private:
    T *_data = nullptr;
    size_t _size = 0U;
    size_t _capacity = 0U;
};

template <typename T>
SimpleVector<T>::SimpleVector(size_t size)
    : _capacity(size)
{
    _data = new T[size];
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
SimpleVector<T>::SimpleVector(SimpleVector<T> &&other)
    : _data(other._data),
      _size(other._size),
      _capacity(other._capacity)
{
    other._data = nullptr;
    other._size = 0U;
    other._capacity = 0U;
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
void SimpleVector<T>::operator=(SimpleVector<T> &&other)
{
    _data = other._data;
    _size = other._size;
    _capacity = other._capacity;

    other._data = nullptr;
    other._size = 0U;
    other._capacity = 0U;
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
    PushBack(value);
}

template <typename T>
void SimpleVector<T>::PushBack(T &&value)
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
            move(_data, _data + _size, new_data);
            delete[] _data;
            _data = new_data;
        }
    }
    _data[_size] = move(value);
    ++_size;
}

// void TestConstruction()
// {
//     SimpleVector<int> empty;
//     ASSERT_EQUAL(empty.Size(), 0u);
//     ASSERT_EQUAL(empty.Capacity(), 0u);
//     ASSERT(empty.begin() == empty.end());

//     SimpleVector<string> five_strings(5);
//     ASSERT_EQUAL(five_strings.Size(), 5u);
//     ASSERT(five_strings.Size() <= five_strings.Capacity());
//     for (auto &item : five_strings)
//     {
//         ASSERT(item.empty());
//     }
//     five_strings[2] = "Hello";
//     ASSERT_EQUAL(five_strings[2], "Hello");
// }

// void TestPushBack()
// {
//     SimpleVector<int> v;
//     for (int i = 10; i >= 1; --i)
//     {
//         v.PushBack(int{i});
//         ASSERT(v.Size() <= v.Capacity());
//     }
//     sort(begin(v), end(v));

//     const vector<int> expected = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
//     ASSERT(equal(begin(v), end(v), begin(expected)));
// }

// class StringNonCopyable : public string
// {
// public:
//     using string::string;
//     StringNonCopyable(string &&other) : string(move(other)) {}
//     StringNonCopyable(const StringNonCopyable &) = delete;
//     StringNonCopyable(StringNonCopyable &&) = default;
//     StringNonCopyable &operator=(const StringNonCopyable &) = delete;
//     StringNonCopyable &operator=(StringNonCopyable &&) = default;
// };

// void TestNoCopy()
// {
//     SimpleVector<StringNonCopyable> strings;
//     static const int SIZE = 10;
//     for (int i = 0; i < SIZE; ++i)
//     {
//         strings.PushBack(StringNonCopyable(to_string(i)));
//     }
//     for (int i = 0; i < SIZE; ++i)
//     {
//         ASSERT_EQUAL(strings[i], to_string(i));
//     }
// }