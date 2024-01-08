#include <iomanip>
#include <iostream>
#include <queue>
#include <map>
#include <profile.h>
#include <test_runner.h>

using namespace std;

void TestAll();
void Profile();

class HotelsReservationManager
{
public:
    void Book(int64_t time, const string &hotel_name, unsigned int client_id, unsigned int room_count)
    {
        _current_time = time;

        _hotels[hotel_name].client_info.push({time, client_id, room_count});
        ++_hotels[hotel_name].clients_reservations_counts[client_id];
        _hotels[hotel_name].room_count += room_count;
    }

    unsigned int Clients(const string &hotel_name)
    {
        PopOutdatedInfo(hotel_name);
        return _hotels[hotel_name].clients_reservations_counts.size();
    }

    unsigned int Rooms(const string &hotel_name)
    {
        PopOutdatedInfo(hotel_name);
        return _hotels[hotel_name].room_count;
    }

private:
    struct ClientInfo
    {
        int64_t time = 0;
        unsigned int client_id = 0;
        unsigned int room_count = 0;
    };

    struct HotelInfo
    {
        queue<ClientInfo> client_info{};
        map<unsigned int, unsigned int> clients_reservations_counts{};
        unsigned int room_count = 0;
    };

    map<string, HotelInfo> _hotels{};

    static constexpr int64_t SecondsInDay = 86400;
    int64_t _current_time = 0;

    void PopOutdatedInfo(const string &hotel_name)
    {
        HotelInfo &hotel = _hotels[hotel_name];

        int64_t dayback_time = _current_time - SecondsInDay;

        while (not hotel.client_info.empty() and hotel.client_info.front().time <= dayback_time)
        {
            hotel.room_count -= hotel.client_info.front().room_count;

            unsigned int client_id = hotel.client_info.front().client_id;
            --hotel.clients_reservations_counts[client_id];

            if (hotel.clients_reservations_counts[client_id] == 0U)
            {
                hotel.clients_reservations_counts.erase(client_id);
            }

            hotel.client_info.pop();
        }
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

    HotelsReservationManager manager{};

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

void TestHotelsReservationManager()
{
    {
        HotelsReservationManager mng{};

        ASSERT_EQUAL(mng.Clients("Marriott"), 0U);
        ASSERT_EQUAL(mng.Rooms("Marriott"), 0U);
        mng.Book(10, "FourSeasons", 1U, 2U);
        mng.Book(10, "Marriott", 1U, 1U);
        mng.Book(86409, "FourSeasons", 2U, 1U);
        ASSERT_EQUAL(mng.Clients("FourSeasons"), 2U);
        ASSERT_EQUAL(mng.Rooms("FourSeasons"), 3U);
        ASSERT_EQUAL(mng.Clients("Marriott"), 1U);
        mng.Book(86410, "Marriott", 2U, 10U);
        ASSERT_EQUAL(mng.Rooms("FourSeasons"), 1U);
        ASSERT_EQUAL(mng.Rooms("Marriott"), 10U);
        mng.Book(86410, "Marriott", 3U, 10U);
        ASSERT_EQUAL(mng.Clients("Marriott"), 2U);
        ASSERT_EQUAL(mng.Rooms("Marriott"), 20U);
        mng.Book(86411, "Marriott", 2U, 10U);
        ASSERT_EQUAL(mng.Clients("Marriott"), 2U);
        ASSERT_EQUAL(mng.Rooms("Marriott"), 30U);
        mng.Book(172810, "Marriott", 2U, 10U);
        ASSERT_EQUAL(mng.Clients("Marriott"), 1U);
        ASSERT_EQUAL(mng.Rooms("Marriott"), 20U);
    }
}

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, TestHotelsReservationManager);
}

void Profile()
{
}