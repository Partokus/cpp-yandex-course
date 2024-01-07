#include <iomanip>
#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <stack>
#include <utility>
#include <deque>
#include <profile.h>
#include <numeric>
#include <test_runner.h>

using namespace std;

void TestAll();
void Profile();

class HotelReservationsManager
{
public:
    void Book(int64_t time, const string &hotel_name, unsigned int client_id, unsigned int rooms_count)
    {
        _current_time = time;

        HotelInfo last_hotel_info{};

        if (not _hotels_books[hotel_name].empty())
        {
            last_hotel_info = _hotels_books[hotel_name].rbegin()->second;
        }

        HotelInfo &hotel_info = _hotels_books[hotel_name][time];

        hotel_info.reservation_counts = last_hotel_info.reservation_counts + 1;
        hotel_info.room_count = last_hotel_info.room_count + rooms_count;
        hotel_info.client_count = last_hotel_info.client_count;

        if (not DidClientReserveDuringTheDay(time, hotel_name, client_id))
        {
            ++hotel_info.client_count;
        }

        _hotels_clients_timings[hotel_name][client_id].push_back(time);
    }

    unsigned int Clients(const string &hotel_name)
    {
        if (not _hotels_books.count(hotel_name))
        {
            return 0U;
        }

        auto &hotel_book = _hotels_books[hotel_name];

        // находим нижний диапазон времени
        int64_t time_day_back = _current_time - CorrectionForDay;
        auto it_bottom_time_range = hotel_book.lower_bound(time_day_back);

        // если за последние сутки не найдено записей
        if (it_bottom_time_range == hotel_book.cend())
        {
            return 0U;
        }

        // находим верхний диапазон времени
        auto it_top_time_range = hotel_book.rbegin();
        HotelInfo &top_hotel_info = it_top_time_range->second;

        if (it_bottom_time_range == hotel_book.cbegin())
        {
            return top_hotel_info.client_count;
        }

        --it_bottom_time_range;
        HotelInfo &bottom_hotel_info = it_bottom_time_range->second;

        return top_hotel_info.client_count - bottom_hotel_info.client_count + 1;
        unsigned int day_reservation_count = top_hotel_info.reservation_counts - bottom_hotel_info.reservation_counts;
        unsigned int day_repeated_reservation_count = day_reservation_count - (top_hotel_info.client_count - bottom_hotel_info.client_count);
        return day_reservation_count - day_repeated_reservation_count;
    }

    unsigned int Rooms(const string &hotel_name)
    {
        if (not _hotels_books.count(hotel_name))
        {
            return 0U;
        }

        auto &hotel_book = _hotels_books[hotel_name];

        // находим нижний диапазон времени
        int64_t time_day_back = _current_time - CorrectionForDay;
        auto it_bottom_time_range = hotel_book.lower_bound(time_day_back);

        // если за последние сутки не найдено записей
        if (it_bottom_time_range == hotel_book.cend())
        {
            return 0U;
        }

        // находим верхний диапазон времени
        auto it_top_time_range = hotel_book.rbegin();

        if (it_bottom_time_range == hotel_book.cbegin())
        {
            return it_top_time_range->second.room_count;
        }

        --it_bottom_time_range;
        return it_top_time_range->second.room_count - it_bottom_time_range->second.room_count;
    }

private:
    struct HotelInfo
    {
        unsigned int reservation_counts = 0; // количество резерваций комнат для накопления
        unsigned int client_count = 0;       // количество клиентов для накопления
        unsigned int room_count = 0;         // количество комнат для накопления
    };

    map<string, map<int64_t, HotelInfo>> _hotels_books{}; // <hotel_name, <time, <clients_count, rooms_count>>>
    map<string, map<unsigned int, vector<int64_t>>> _hotels_clients_timings{};

    static constexpr int64_t CorrectionForDay = 86400 - 1;
    int64_t _current_time = 0;

    bool DidClientReserveDuringTheDay(int64_t time, const string &hotel_name, unsigned int client_id) const
    {
        if (not _hotels_clients_timings.count(hotel_name) or not _hotels_clients_timings.at(hotel_name).count(client_id))
        {
            return false;
        }

        const auto &client_timing = _hotels_clients_timings.at(hotel_name).at(client_id);

        // находим нижний диапазон времени
        const int64_t time_day_back = time - CorrectionForDay;
        const auto it_bottom_time_range = lower_bound(client_timing.cbegin(), client_timing.cend(), time_day_back);

        // если за последние сутки не найдено записей
        if (it_bottom_time_range == client_timing.cend())
        {
            return false;
        }
        return true;
    }
};

int main()
{
    TestAll();
    Profile();

    // для ускорения чтения данных отключается синхронизация
    // cin и cout с stdio,
    // а также выполняется отвязка cin от cout
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    HotelReservationsManager manager{};

    int query_count = 0;
    cin >> query_count;

    for (int query_id = 0; query_id < query_count; ++query_id)
    {
        string query_type{};
        cin >> query_type;

        string hotel_name{};

        try
        {
            if (query_type == "BOOK")
            {
                int64_t time = 0;
                unsigned int client_id = 0;
                unsigned int rooms_count = 0;

                cin >> time >> hotel_name >> client_id >> rooms_count;

                manager.Book(time, hotel_name, client_id, rooms_count);
            }
            else if (query_type == "CLIENTS")
            {
                cin >> hotel_name;

                cout << manager.Clients(hotel_name) << endl;
            }
            else if (query_type == "ROOMS")
            {
                cin >> hotel_name;

                cout << manager.Rooms(hotel_name) << endl;
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
        }
    }
    return 0;
}

void TestAll()
{
    TestRunner tr{};
    // RUN_TEST(tr, TestReadManager);
}

void Profile()
{
}


// new_client_count = top_client - bottom_client
// client_count = reservation_count - new_client_count
