#include <iomanip>
#include <iostream>
#include <set>
#include <map>
#include <utility>
#include <profile.h>
#include <numeric>
#include <tuple>
#include <test_runner.h>
#include <algorithm>

using namespace std;

void TestAll();
void Profile();

struct ReservationInfo
{
    int64_t time = 0;
    unsigned int client_id = 0;
};

bool operator<(const ReservationInfo &lhs, const ReservationInfo &rhs)
{
    return tie(lhs.time, lhs.client_id) < tie(rhs.time, rhs.client_id);
}

bool operator==(const ReservationInfo &lhs, const ReservationInfo &rhs)
{
    return tie(lhs.time, lhs.client_id) == tie(rhs.time, rhs.client_id);
}

class HotelReservationsManager
{
public:
    void Book(int64_t time, const string &hotel_name, unsigned int client_id, unsigned int room_count)
    {
        _current_time = time;

        set<ReservationInfo> &reservations_info = _hotel_client_count[hotel_name];
        map<unsigned int, int64_t> &clients_timings = _hotel_clients_timings[hotel_name];

        if (clients_timings.count(client_id))
        {
            const int64_t client_time = clients_timings.at(client_id);
            reservations_info.erase(ReservationInfo{client_time, client_id});
        }

        clients_timings[client_id] = time;

        reservations_info.insert(ReservationInfo{time, client_id});

        map<int64_t, unsigned int> &hotel_room_count = _hotel_room_count[hotel_name];

        unsigned int last_room_count = 0U;
        if (not hotel_room_count.empty())
        {
            last_room_count = hotel_room_count.rbegin()->second;
        }
        hotel_room_count[time] = room_count + last_room_count;
    }

    unsigned int Clients(const string &hotel_name)
    {
        if (not _hotel_client_count.count(hotel_name))
        {
            return 0U;
        }

        set<ReservationInfo> &reservations_info = _hotel_client_count[hotel_name];

        int64_t day_back_time = _current_time - CorrectionForDay;

        auto it = reservations_info.upper_bound(ReservationInfo{day_back_time, 0U});

        reservations_info.erase(reservations_info.begin(), it);
        return reservations_info.size();
    }

    unsigned int Rooms(const string &hotel_name)
    {
        if (not _hotel_room_count.count(hotel_name))
        {
            return 0U;
        }

        map<int64_t, unsigned int> &hotel_room_count = _hotel_room_count[hotel_name];

        // находим нижний диапазон времени
        int64_t time_day_back = _current_time - CorrectionForDay;
        auto it_bottom_time_range = hotel_room_count.lower_bound(time_day_back);

        // если за последние сутки не найдено записей
        if (it_bottom_time_range == hotel_room_count.cend())
        {
            return 0U;
        }

        // находим верхний диапазон времени
        auto it_top_time_range = hotel_room_count.rbegin();

        if (it_bottom_time_range == hotel_room_count.cbegin())
        {
            return it_top_time_range->second;
        }

        --it_bottom_time_range;
        return it_top_time_range->second - it_bottom_time_range->second;
    }

private:
    map<string, set<ReservationInfo>> _hotel_client_count{};
    map<string, map<unsigned int, int64_t>> _hotel_clients_timings{};
    map<string, map<int64_t, unsigned int>> _hotel_room_count{};

    static constexpr int64_t CorrectionForDay = 86400 - 1;
    int64_t _current_time = 0;
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

            cout << manager.Clients(hotel_name) << '\n';
        }
        else if (query_type == "ROOMS")
        {
            cin >> hotel_name;

            cout << manager.Rooms(hotel_name) << '\n';
        }
    }
    return 0;
}

void TestReservationInfoOperators()
{
    {
        ReservationInfo r1 = {0, 0};
        ReservationInfo r2 = {0, 0};
        ASSERT(r1 == r2);
        r2 = {1, 0};
        ASSERT(not(r1 == r2));
    }
    {
        ReservationInfo r1 = {1, 0};
        ReservationInfo r2 = {0, 0};
        ASSERT(r2 < r1);
        r1 = {1, 1};
        ASSERT(r2 < r1);
        r1 = {-1, 1};
        ASSERT(r1 < r2);
        r1 = {-1, 0};
        ASSERT(r1 < r2);
        r1 = {0, 0};
        ASSERT(not(r1 < r2));
        r1 = {10, 1};
        r2 = {10, 2};
        ASSERT(r1 < r2);
        r1 = {11, 1};
        ASSERT(r2 < r1);
    }
}

void TestHotelReservationsManager()
{
    {
        HotelReservationsManager mng{};

        ASSERT_EQUAL(mng.Clients("Marriott"), 0U);
        ASSERT_EQUAL(mng.Rooms("Marriott"), 0U);
        mng.Book(10, "FourSeasons", 1, 2);
        mng.Book(10, "Marriott", 1, 1);
        mng.Book(86409, "FourSeasons", 2, 1);
        ASSERT_EQUAL(mng.Clients("FourSeasons"), 2U);
        ASSERT_EQUAL(mng.Rooms("FourSeasons"), 3U);
        ASSERT_EQUAL(mng.Clients("Marriott"), 1U);
        mng.Book(86410, "Marriott", 2, 10);
        ASSERT_EQUAL(mng.Rooms("FourSeasons"), 1U);
        ASSERT_EQUAL(mng.Rooms("Marriott"), 10U);
        mng.Book(86410, "Marriott", 3, 10);
        ASSERT_EQUAL(mng.Clients("Marriott"), 2U);
        ASSERT_EQUAL(mng.Rooms("Marriott"), 20U);
        mng.Book(86411, "Marriott", 2, 10);
        ASSERT_EQUAL(mng.Clients("Marriott"), 2U);
        ASSERT_EQUAL(mng.Rooms("Marriott"), 30U);
        mng.Book(172810, "Marriott", 2, 10);
        ASSERT_EQUAL(mng.Clients("Marriott"), 1U);
        ASSERT_EQUAL(mng.Rooms("Marriott"), 20U);
    }
}

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, TestReservationInfoOperators);
    RUN_TEST(tr, TestHotelReservationsManager);
}

void Profile()
{
}
