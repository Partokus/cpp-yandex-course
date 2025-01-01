#include <profile.h>
#include <test_runner.h>

#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <utility>
#include <map>
#include <optional>
#include <unordered_set>
#include <algorithm>
#include <numeric>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <cmath>
#include <random>
#include <memory>

using namespace std;

void TestAll();
void Profile();

template <class T>
class ObjectPool
{
public:
    T *Allocate();
    T *TryAllocate();

    void Deallocate(T *object);

private:
    // Определили свой компаратор
    struct Compare
    {
        using is_transparent = void; // Сделали компаратор прозрачным

        // Использовали стандартное сравнение для ключей
        bool operator()(const unique_ptr<T> &lhs, const unique_ptr<T> &rhs) const
        {
            return lhs < rhs;
        }
        // Определили функции сравнения ключа и обычного указателя
        bool operator()(const unique_ptr<T> &lhs, const T *rhs) const
        {
            return less<const T *>()(lhs.get(), rhs);
        }
        bool operator()(const T *lhs, const unique_ptr<T> &rhs) const
        {
            return less<const T *>()(lhs, rhs.get());
        }
    };

    queue<unique_ptr<T>> _free;
    set<unique_ptr<T>, Compare> _allocated;
};

template <typename T>
T *ObjectPool<T>::Allocate()
{
    if (_free.empty())
    {
        _free.push(make_unique<T>());
    }
    auto ptr = move(_free.front());
    _free.pop();
    T *ret = ptr.get();
    _allocated.insert(move(ptr));
    return ret;
}

template <typename T>
T *ObjectPool<T>::TryAllocate()
{
    if (_free.empty())
    {
        return nullptr;
    }
    return Allocate();
}

template <typename T>
void ObjectPool<T>::Deallocate(T *object)
{
    auto it = _allocated.find(object);
    if (it == _allocated.end())
    {
        throw invalid_argument("");
    }
    _free.push(move(_allocated.extract(it).value()));
}

int main()
{
    TestAll();
    Profile();

    return 0;
}

void TestObjectPool()
{
    {
        ObjectPool<string> pool;

        auto p1 = pool.Allocate();
        auto p2 = pool.Allocate();
        auto p3 = pool.Allocate();

        *p1 = "first";
        *p2 = "second";
        *p3 = "third";

        pool.Deallocate(p2);
        ASSERT_EQUAL(*pool.Allocate(), "second");

        pool.Deallocate(p3);
        pool.Deallocate(p1);
        ASSERT_EQUAL(*pool.Allocate(), "third");
        ASSERT_EQUAL(*pool.Allocate(), "first");

        pool.Deallocate(p1);
    }
    {
        ObjectPool<string> pool;
        for (int i = 0; i < 100'000 * 100; ++i)
        {
            pool.Allocate();
        }
    }
}

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, TestObjectPool);
}

void Profile()
{
}