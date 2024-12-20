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

using namespace std;

void TestAll();
void Profile();

template <typename T>
class LazyValue
{
public:
    explicit LazyValue(std::function<T()> init);

    bool HasValue() const;
    const T &Get() const;

private:
    std::function<T()> _init = nullptr;
    mutable std::optional<T> _value = std::nullopt;
};

template <typename T>
LazyValue<T>::LazyValue(std::function<T()> init)
    : _init(init)
{
}

template <typename T>
bool LazyValue<T>::HasValue() const
{
    return _value.has_value();
}

template <typename T>
const T & LazyValue<T>::Get() const
{
    if (not _value.has_value())
        _value = _init();

    return _value.value();
}

int main()
{
    TestAll();
    Profile();

    return 0;
}

void UseExample()
{
    const string big_string = "Giant amounts of memory";

    LazyValue<string> lazy_string([&big_string]
                                  { return big_string; });

    ASSERT(!lazy_string.HasValue());
    ASSERT_EQUAL(lazy_string.Get(), big_string);
    ASSERT_EQUAL(lazy_string.Get(), big_string);
    ASSERT(lazy_string.HasValue());
}

void TestInitializerIsntCalled()
{
    bool called = false;

    {
        LazyValue<int> lazy_int([&called]
        {
            called = true;
            return 0;
        });
    }
    ASSERT(!called);
}

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, UseExample);
    RUN_TEST(tr, TestInitializerIsntCalled);
}

void Profile()
{
}
