#include <algorithm>

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
SimpleVector<T>::~SimpleVector()
{
    delete[] _data;
}

template <typename T>
T *SimpleVector<T>::begin() { return _data; }
template <typename T>
T *SimpleVector<T>::end() { return _end; }
template <typename T>
const T *SimpleVector<T>::begin() const { return _data; }
template <typename T>
const T *SimpleVector<T>::end() const { return _end; }

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
            std::copy(_data, _end, new_data);
            delete[] _data;
            _data = new_data;
        }
    }
    _data[_size] = value;
    ++_size;
    _end = _data + _size;
}