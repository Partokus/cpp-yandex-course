#include <profile.h>
#include <test_runner.h>

using namespace std;

void TestAll();
void Profile();

template <typename T>
class UniquePtr
{
public:
    UniquePtr() = default;

    UniquePtr(T *ptr)
        : _ptr(ptr)
    {
    }

    UniquePtr(const UniquePtr &) = delete;

    UniquePtr(UniquePtr &&other)
    {
        _ptr = other._ptr;
        other._ptr = nullptr;
    }

    UniquePtr &operator=(const UniquePtr &) = delete;

    UniquePtr &operator=(nullptr_t)
    {
        Reset(nullptr);
        return *this;
    }

    UniquePtr &operator=(UniquePtr &&other)
    {
        if (this != &other)
        {
            Reset(other._ptr);
            other._ptr = nullptr;
        }
        return *this;
    }

    ~UniquePtr()
    {
        delete _ptr;
    }

    T &operator*() const
    {
        return *_ptr;
    }

    T *operator->() const
    {
        return _ptr;
    }

    T *Release()
    {
        T *temp = _ptr;
        _ptr = nullptr;
        return temp;
    }

    void Reset(T *ptr)
    {
        delete _ptr;
        _ptr = ptr;
    }

    void Swap(UniquePtr &other)
    {
        swap(_ptr, other._ptr);
    }

    T *Get() const
    {
        return _ptr;
    }

private:
    T *_ptr = nullptr;
};

int main()
{
    TestAll();
    Profile();

    return 0;
}

struct Item
{
    static int counter;
    int value;
    Item(int v = 0) : value(v)
    {
        ++counter;
    }
    Item(const Item &other) : value(other.value)
    {
        ++counter;
    }
    ~Item()
    {
        --counter;
    }
};

int Item::counter = 0;

void TestLifetime()
{
    Item::counter = 0;
    {
        UniquePtr<Item> ptr(new Item);
        ASSERT_EQUAL(Item::counter, 1);

        ptr.Reset(new Item);
        ASSERT_EQUAL(Item::counter, 1);
    }
    ASSERT_EQUAL(Item::counter, 0);

    {
        UniquePtr<Item> ptr(new Item);
        ASSERT_EQUAL(Item::counter, 1);

        auto rawPtr = ptr.Release();
        ASSERT_EQUAL(Item::counter, 1);

        delete rawPtr;
        ASSERT_EQUAL(Item::counter, 0);
    }
    ASSERT_EQUAL(Item::counter, 0);
}

void TestGetters()
{
    UniquePtr<Item> ptr(new Item(42));
    ASSERT_EQUAL(ptr.Get()->value, 42);
    ASSERT_EQUAL((*ptr).value, 42);
    ASSERT_EQUAL(ptr->value, 42);
}

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, TestLifetime);
    RUN_TEST(tr, TestGetters);
}

void Profile()
{
}
