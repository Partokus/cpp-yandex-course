#include <vector>
#include <stdexcept>
#include <iostream>

using namespace std;

template <class T>
class Deque
{
public:
    void PushFront(const T &value)
    {
        _front.push_back(value);
    }

    void PushBack(const T &value)
    {
        _back.push_back(value);
    }

    bool Empty() const
    {
        return _front.empty() and _back.empty();
    }

    size_t Size() const
    {
        return _front.size() + _back.size();
    }

    T &operator[](size_t index)
    {
        return Elem(index);
    }

    const T &operator[](size_t index) const
    {
        return Elem(index);
    }

    T &At(size_t index)
    {
        check(index);
        return Elem(index);
    }

    const T &At(size_t index) const
    {
        check(index);
        return Elem(index);
    }

    T &Front()
    {
        return _front.empty() ? _back.front() : _front.back();
    }

    const T &Front() const
    {
        return _front.empty() ? _back.front() : _front.back();
    }

    T &Back()
    {
        return _back.empty() ? _front.front() : _back.back();
    }

    const T &Back() const
    {
        return _back.empty() ? _front.front() : _back.back();
    }

private:
    vector<T> _front{};
    vector<T> _back{};

    T &Elem(size_t index)
    {
        if (index < _front.size())
        {
            return _front[_front.size() - 1 - index];
        }
        return _back[index - _front.size()];
    }

    void check(size_t index)
    {
        if (index >= Size())
        {
            throw out_of_range("Out of range");
        }
    }
};

int main()
{
    return 0;
}