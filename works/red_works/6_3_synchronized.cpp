#include <profile.h>
#include <test_runner.h>
#include <vector>
#include <algorithm>
#include <numeric>
#include <iterator>
#include <cmath>
#include <future>
#include <string_view>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

using namespace std;

void TestAll();
void Profile();

template <typename T>
class Synchronized
{
public:
    explicit Synchronized(T initial = T());

    struct Access
    {
        T &ref_to_value;
        lock_guard<mutex> guard;
    };

    Access GetAccess();

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
typename Synchronized<T>::Access Synchronized<T>::GetAccess()
{
    return {_value, lock_guard(_m)};
}

int main()
{
    TestAll();
    Profile();
    return 0;
}

void TestConcurrentUpdate()
{
    Synchronized<string> common_string;

    const size_t add_count = 50000;
    auto updater = [&common_string, add_count]
    {
        for (size_t i = 0; i < add_count; ++i)
        {
            auto access = common_string.GetAccess();
            access.ref_to_value += 'a';
        }
    };

    auto f1 = async(updater);
    auto f2 = async(updater);

    f1.get();
    f2.get();

    ASSERT_EQUAL(common_string.GetAccess().ref_to_value.size(), 2 * add_count);
}

vector<int> Consume(Synchronized<deque<int>> &common_queue)
{
    vector<int> got;

    for (;;)
    {
        deque<int> q;

        {
            // Мы специально заключили эти две строчки в операторные скобки, чтобы
            // уменьшить размер критической секции. Поток-потребитель захватывает
            // мьютекс, перемещает всё содержимое общей очереди в свою
            // локальную переменную и отпускает мьютекс. После этого он обрабатывает
            // объекты в очереди за пределами критической секции, позволяя
            // потоку-производителю параллельно помещать в очередь новые объекты.
            //
            // Размер критической секции существенно влияет на быстродействие
            // многопоточных программ.
            auto access = common_queue.GetAccess();
            q = move(access.ref_to_value);
        }

        for (int item : q)
        {
            if (item > 0)
            {
                got.push_back(item);
            }
            else
            {
                return got;
            }
        }
    }
}

void TestProducerConsumer()
{
    Synchronized<deque<int>> common_queue;

    auto consumer = async(Consume, ref(common_queue));

    const size_t item_count = 100000;
    for (size_t i = 1; i <= item_count; ++i)
    {
        common_queue.GetAccess().ref_to_value.push_back(i);
    }
    common_queue.GetAccess().ref_to_value.push_back(-1);

    vector<int> expected(item_count);
    iota(begin(expected), end(expected), 1);
    ASSERT_EQUAL(consumer.get(), expected);
}

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, TestConcurrentUpdate);
    RUN_TEST(tr, TestProducerConsumer);
}

void Profile()
{
}