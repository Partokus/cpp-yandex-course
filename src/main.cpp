#include <iomanip>
#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <utility>
#include <profile.h>

using namespace std;

class ReadingManager
{
public:
    ReadingManager()
        : user_page_counts_(MAX_USER_COUNT_ + 1, 0),
          sorted_users_(),
          user_positions_(MAX_USER_COUNT_ + 1, -1)
    {
    }

    void Read(int user_id, int page_count)
    {
        if (not user_id_to_page_count.count(user_id))
        {
            user_id_to_page_count[user_id] = page_count;
            page_count_to_user_ids[page_count].insert(user_id);
        }
        else
        {
            const int user_page_count = user_id_to_page_count[user_id];

            page_count_to_user_ids[user_page_count].erase(user_id);
            if (page_count_to_user_ids[user_page_count].empty())
            {
                page_count_to_user_ids.erase(user_page_count);
            }

            user_id_to_page_count[user_id] = page_count;
            page_count_to_user_ids[page_count].insert(user_id);
        }
    }

    double Cheer(int user_id) const
    {
        if (not user_id_to_page_count.count(user_id))
        {
            return 0;
        }
        const int user_count = user_id_to_page_count.size();
        if (user_count == 1)
        {
            return 1;
        }

        const int page_count = user_id_to_page_count.at(user_id);
        int position = user_positions_[user_id];

        while (position < user_count &&
               user_page_counts_[sorted_users_[position]] == page_count)
        {
            ++position;
        }
        if (position == user_count)
        {
            return 0.;
        }
        // По умолчанию деление целочисленное, поэтому
        // нужно привести числитель к типу double.
        // Простой способ сделать это — умножить его на 1.0.
        return static_cast<double>(user_count - position) / (user_count - 1);
    }

private:
    // Статическое поле не принадлежит какому-то конкретному
    // объекту класса. По сути это глобальная переменная,
    // в данном случае константная.
    // Будь она публичной, к ней можно было бы обратиться снаружи
    // следующим образом: ReadingManager::MAX_USER_COUNT.
    static const int MAX_USER_COUNT_ = 100'000;

    map<int, set<int>> page_count_to_user_ids{};
    map<int, int> user_id_to_page_count{};
    vector<int> user_page_counts_;
    vector<int> sorted_users_;   // отсортированы по убыванию количества страниц
    vector<int> user_positions_; // позиции в векторе sorted_users_
};

int main()
{
    // Для ускорения чтения данных отключается синхронизация
    // cin и cout с stdio,
    // а также выполняется отвязка cin от cout
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    ReadingManager manager;

    int page_count = 0;
    int page_increment_period = 100;
    int page_increment_counter = 0;

    for (int i = 99'500; i > 0; --i)
    {
        LOG_DURATION("Read");
        manager.Read(i, page_count);
        if (page_increment_counter++ == page_increment_period)
        {
            ++page_count;
            page_increment_counter = 0;
        }
    }

    return 0;

    int query_count;
    cin >> query_count;

    for (int query_id = 0; query_id < query_count; ++query_id)
    {
        string query_type;
        cin >> query_type;
        int user_id;
        cin >> user_id;

        if (query_type == "READ")
        {
            int page_count;
            cin >> page_count;
            manager.Read(user_id, page_count);
        }
        else if (query_type == "CHEER")
        {
            cout << setprecision(6) << manager.Cheer(user_id) << "\n";
        }
    }
    return 0;
}