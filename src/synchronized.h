#include <mutex>
#include <utility>

using namespace std;

template <typename T>
class Synchronized
{
public:
    explicit Synchronized(T initial = T());

    struct Access
    {
        T &ref;
        lock_guard<mutex> guard;
    };

    Access access();

private:
    T _value;
    mutex _m;
};

template <typename T>
Synchronized<T>::Synchronized(T initial)
    : _value(move(initial))
{
}

template <typename T>
typename Synchronized<T>::Access Synchronized<T>::access()
{
    return {_value, lock_guard(_m)};
}