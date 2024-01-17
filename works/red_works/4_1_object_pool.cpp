#include <profile.h>
#include <test_runner.h>
#include <iomanip>
#include <iostream>
#include <queue>
#include <set>
#include <string>
#include <stdexcept>

using namespace std;

void TestAll();
void Profile();

template <class T>
class ObjectPool
{
public:
    T *Allocate()
    {
        T *ptr = nullptr;

        if (_deallocated.empty())
        {
            ptr = new T;
        }
        else
        {
            ptr = _deallocated.front();
            _deallocated.pop();
        }
        _allocated.insert(ptr);

        return ptr;
    }

    T *TryAllocate()
    {
        if (_deallocated.empty())
        {
            return nullptr;
        }

        T *ptr = nullptr;

        ptr = _deallocated.front();
        _deallocated.pop();
        _allocated.insert(ptr);
        return ptr;
    }

    void Deallocate(T *object)
    {
        auto it = _allocated.find(object);
        if (it != _allocated.cend())
        {
            _deallocated.push(*it);
            _allocated.erase(it);
        }
        else
        {
            throw invalid_argument("Didn't find object");
        }
    }

    ~ObjectPool()
    {
        for (auto &i : _allocated)
        {
            delete i;
        }
        while (not _deallocated.empty())
        {
            T *ptr = _deallocated.front();
            delete ptr;
            _deallocated.pop();
        }
    }

private:
    set<T *> _allocated{};
    queue<T *> _deallocated{};
};

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