#include <iomanip>
#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <utility>
#include <profile.h>
#include <numeric>
#include <test_runner.h>

using namespace std;

class ReadingManager
{
public:
    ReadingManager()
        : _page_count_users_count(MaxPagesCount + 1),
          _users_behind_count(MaxPagesCount + 1)
    {
    }

    void Read(int user_id, int page_count)
    {
        if (not _users_page_count.count(user_id))
        {
            _users_page_count[user_id] = page_count;
            ++_page_count_users_count[page_count];
        }
        else
        {
            const int user_page_count = _users_page_count.at(user_id);
            --_page_count_users_count[user_page_count];
            ++_page_count_users_count[page_count];
            _users_page_count[user_id] = page_count;
        }

        _is_partial_sum_done = false;
    }

    double Cheer(int user_id)
    {
        if (not _users_page_count.count(user_id))
        {
          return 0.0;
        }

        const int users_count = _users_page_count.size();

        if (users_count == 1)
        {
          return 1.0;
        }

        const int user_page_count = _users_page_count.at(user_id);

        if (not _is_partial_sum_done)
        {
            partial_sum(_page_count_users_count.begin(), _page_count_users_count.end(), _users_behind_count.begin());
            _is_partial_sum_done = true;
        }

        const int users_behind_count = _users_behind_count[user_page_count] - _page_count_users_count[user_page_count];

        return static_cast<double>(users_behind_count) / (users_count - 1);
    }

private:
    static constexpr int MaxPagesCount = 1'000;

    map<int, int> _users_page_count{};     // v[user] = page_count
    vector<int> _page_count_users_count{}; // сколько пользователей у конкретного количества страниц v[page_count] = users_count. Вектор для аккумуляции
    vector<int> _users_behind_count{};     // количество пользователей позади
    bool _is_partial_sum_done = false;
};

void TestReadManager()
{
    {
        ReadingManager manager;
        manager.Read(1, 1);
        manager.Read(2, 2);
        manager.Read(3, 3);
        ASSERT_EQUAL(manager.Cheer(3), 1.0);
        ASSERT_EQUAL(manager.Cheer(2), 0.5);
        ASSERT_EQUAL(manager.Cheer(1), 0.0);
        manager.Read(4, 4);
        ASSERT(manager.Cheer(3) <= 0.666668 and manager.Cheer(3) >= 0.666666);
        ASSERT(manager.Cheer(2) <= 0.333334 and manager.Cheer(2) >= 0.333332);
        ASSERT_EQUAL(manager.Cheer(1), 0.0);
        manager.Read(5, 5);
        ASSERT_EQUAL(manager.Cheer(1), 0.0);
        ASSERT_EQUAL(manager.Cheer(2), 0.25);
        ASSERT_EQUAL(manager.Cheer(3), 0.5);
        ASSERT_EQUAL(manager.Cheer(4), 0.75);
        ASSERT_EQUAL(manager.Cheer(5), 1.0);
    }
    {
        ReadingManager manager;
        manager.Read(1, 1);
        manager.Read(2, 2);
        manager.Read(3, 3);
        ASSERT_EQUAL(manager.Cheer(3), 1.0);
        ASSERT_EQUAL(manager.Cheer(2), 0.5);
        ASSERT_EQUAL(manager.Cheer(1), 0.0);
        manager.Read(4, 1);
        manager.Read(1, 4);
        ASSERT(manager.Cheer(3) <= 0.666668 and manager.Cheer(3) >= 0.666666);
        ASSERT(manager.Cheer(2) <= 0.333334 and manager.Cheer(2) >= 0.333332);
        ASSERT_EQUAL(manager.Cheer(1), 1.0);
        manager.Read(5, 5);
        ASSERT_EQUAL(manager.Cheer(1), 0.75);
        ASSERT_EQUAL(manager.Cheer(2), 0.25);
        ASSERT_EQUAL(manager.Cheer(3), 0.5);
        ASSERT_EQUAL(manager.Cheer(4), 0.0);
        ASSERT_EQUAL(manager.Cheer(5), 1.0);
    }
}

void TestAll();

int main()
{
    TestAll();
    // Profile();

    // Для ускорения чтения данных отключается синхронизация
    // cin и cout с stdio,
    // а также выполняется отвязка cin от cout
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    ReadingManager manager{};

    int query_count = 0;
    cin >> query_count;

    for (int query_id = 0; query_id < query_count; ++query_id)
    {
        string query_type{};
        cin >> query_type;
        int user_id = 0;
        cin >> user_id;

        if (query_type == "READ")
        {
            int page_count = 0;
            cin >> page_count;
            manager.Read(user_id, page_count);
        }
        else if (query_type == "CHEER")
        {
            cout << setprecision(6) << manager.Cheer(user_id) << '\n';
        }
    }
    return 0;
}

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, TestReadManager);
}

void Profile()
{
    ReadingManager manager;

    int page_count = 0;
    int page_increment_period = 100;
    int page_increment_counter = 0;

    LOG_DURATION("Total");
    {
        LOG_DURATION("Read");
        for (int i = 0; i < 20'000; ++i)
        {

            manager.Read(i, page_count);
            if (page_increment_counter++ == page_increment_period)
            {
                ++page_count;
                page_increment_counter = 0;
            }
        }
    }
    {
      LOG_DURATION("Cheer");
      for (int i = 0; i < 20'000; ++i)
      {
        manager.Cheer(i);
      }
    }
}