#include <profile.h>
#include <test_runner.h>

#include <cassert>
#include <algorithm>
#include <iterator>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <string>
#include <forward_list>
#include <iterator>
#include <deque>
#include <tuple>
#include <random>
#include <functional>
#include <fstream>
#include <random>

using namespace std;

void TestAll();
void Profile();

using namespace std;

struct Record
{
    string id;
    string title;
    string user;
    int timestamp = 0;
    int karma = 0;

    bool operator==(const Record &other) const
    {
        return id == other.id;
    }
};

struct RecordHasher
{
    size_t operator()(const Record &v) const
    {
        return str_hasher(v.id);
    }

    hash<string> str_hasher;
};

// Реализуйте этот класс
class Database
{
public:
    bool Put(const Record &record);
    const Record *GetById(const string &id) const;
    bool Erase(const string &id);

    template <typename Callback>
    void RangeByTimestamp(int low, int high, Callback callback) const;

    template <typename Callback>
    void RangeByKarma(int low, int high, Callback callback) const;

    template <typename Callback>
    void AllByUser(const string &user, Callback callback) const;

private:
    unordered_set<Record, RecordHasher> _ids;
    multimap<int, reference_wrapper<const Record>> _timestamp_to_rec;
    multimap<int, reference_wrapper<const Record>> _karma_to_rec;
    unordered_multimap<string_view, reference_wrapper<const Record>> _user_to_rec;

    template<typename Multimap>
    void erase(Multimap &m,
        const typename Multimap::key_type &key,
        const typename Multimap::mapped_type &mapped);

    template <typename Multimap, typename Callback>
    void RangeBy(const Multimap &m, int low, int high, Callback callback) const;
};

bool Database::Put(const Record &record)
{
    auto [it, inserted] = _ids.insert(record);
    if (not inserted)
    {
        return false;
    }

    _timestamp_to_rec.emplace(it->timestamp, ref(*it));
    _karma_to_rec.emplace(it->karma, ref(*it));
    _user_to_rec.emplace(it->user, ref(*it));
    return true;
}

const Record *Database::GetById(const string &id) const
{
    if (auto it = _ids.find(Record{.id = id}); it != _ids.end())
    {
        return &(*it);
    }
    return nullptr;
}

bool Database::Erase(const string &id)
{
    auto it = _ids.find(Record{.id = id});
    if (it == _ids.end())
    {
        return false;
    }

    erase(_timestamp_to_rec, it->timestamp, ref(*it));
    erase(_karma_to_rec, it->karma, ref(*it));
    erase(_user_to_rec, it->user, ref(*it));

    _ids.erase(it);
    return true;
}

template <typename Callback>
void Database::RangeByTimestamp(int low, int high, Callback callback) const
{
    RangeBy(_timestamp_to_rec, low, high, callback);
}

template <typename Callback>
void Database::RangeByKarma(int low, int high, Callback callback) const
{
    RangeBy(_karma_to_rec, low, high, callback);
}

template <typename Callback>
void Database::AllByUser(const string &user, Callback callback) const
{
    for (auto [it, end] = _user_to_rec.equal_range(user); it != end; ++it)
    {
        const Record &record = it->second.get();
        if (const bool keep_searching = callback(record); not keep_searching)
        {
            return;
        }
    }
}

template<typename Multimap>
void Database::erase(Multimap &m,
    const typename Multimap::key_type &key,
    const typename Multimap::mapped_type &mapped)
{
    const Record &record_with_need_id = mapped.get();
    for (auto [it, end] = m.equal_range(key); it != end; ++it)
    {
        const Record &record_with_need_timestamp = it->second;
        if (record_with_need_timestamp == record_with_need_id)
        {
            it = m.erase(it);
            return;
        }
    }
}

template <typename Multimap, typename Callback>
void Database::RangeBy(const Multimap &m, int low, int high, Callback callback) const
{
    auto begin = m.lower_bound(low);
    auto end = m.upper_bound(high);

    for (auto it = begin; it != end; ++it)
    {
        const Record &record = it->second.get();
        if (const bool keep_searching = callback(record); not keep_searching)
        {
            return;
        }
    }
}

int main()
{
    TestAll();
    Profile();

    // const int good_karma = 1000;
    // const int bad_karma = -10;

    // Database db;
    // db.Put({"id1", "Hello there", "master", 1536107260, good_karma});
    // db.Put({"id2", "O>>-<", "general2", 1536107260, bad_karma});

    // db.Erase("id1");
    // db.Erase("id3");
    // db.Erase("id2");
    // multimap<int, int> m;
    // m.insert({1, 1});
    // m.insert({1, 1});
    // m.insert({1, 2});
    // m.insert({1, 0});
    // m.insert({0, 1});

    // for (const auto &[key, value] : m)
    // {
    //     cout << "[" << key << ", " << value << "]" << endl;
    // }
    return 0;
}

void TestRangeBoundaries()
{
    const int good_karma = 1000;
    const int bad_karma = -10;

    Database db;
    db.Put({"id1", "Hello there", "master", 1536107260, good_karma});
    db.Put({"id2", "O>>-<", "general2", 1536107260, bad_karma});

    int count = 0;
    db.RangeByKarma(bad_karma, good_karma, [&count](const Record &)
                    {
        ++count;
    return true; });

    ASSERT_EQUAL(2, count);
}

void TestSameUser()
{
    Database db;
    db.Put({"id1", "Don't sell", "master", 1536107260, 1000});
    db.Put({"id2", "Rethink life", "master", 1536107260, 2000});

    int count = 0;
    db.AllByUser("master", [&count](const Record &)
                 {
    ++count;
    return true; });

    ASSERT_EQUAL(2, count);
}

void TestReplacement()
{
    const string final_body = "Feeling sad";

    Database db;
    db.Put({"id", "Have a hand", "not-master", 1536107260, 10});
    db.Erase("id");
    db.Put({"id", final_body, "not-master", 1536107260, -10});

    auto record = db.GetById("id");
    ASSERT(record != nullptr);
    ASSERT_EQUAL(final_body, record->title);
}

static constexpr size_t RecordsCount = 300'000U;

void MakeInputData()
{
    static constexpr size_t IdsVariaty = 100U; // количество различных id

    Database db;
    ofstream ofile("input_data.txt");
    Record r{};

    for (size_t i = 0U; i < RecordsCount; ++i)
    {
        // делаем id с названием "123456789" и
        // в конце число от 0 до IdsVariaty
        const size_t num1 = rand() % IdsVariaty;
        const size_t num2 = rand() % IdsVariaty;
        r.id = "123456789" + to_string(num1);
        r.title = "just_random_title" + to_string(num1) + to_string(num2);
        r.user = "just_random_name" + to_string(num1) + to_string(num2);
        r.timestamp = num1;
        r.karma = num2;

        ofile << r.id << " " << r.title << " " << r.user << " " << r.timestamp << " " << r.karma << endl;
    }
}

void ProfileLong()
{
    LOG_DURATION("Total");
    Database db;
    ifstream ifile("input_data.txt");

    if (not ifile)
    {
        throw runtime_error("Didn't open file");
    }

    {
        LOG_DURATION("Put");
        for (size_t i = 0; i < RecordsCount; ++i)
        {
            Record r;
            ifile >> r.id >> r.title >> r.user >> r.timestamp >> r.karma;
            db.Put(r);
        }
    }

    ifile.seekg(ios::beg);

    {
        LOG_DURATION("GetById");
        for (size_t i = 0; i < RecordsCount; ++i)
        {
            Record r;
            ifile >> r.id >> r.title >> r.user >> r.timestamp >> r.karma;
            db.GetById(r.id);
        }
    }

    ifile.seekg(ios::beg);

    size_t counter = 0U;
    auto cb = [&counter](const Record &rec){
        return ++counter;
    };

    {
        LOG_DURATION("RangeByTimestamp");
        for (size_t i = 0; i < RecordsCount; ++i)
        {
            Record r;
            ifile >> r.id >> r.title >> r.user >> r.timestamp >> r.karma;
            const int low = r.timestamp <= r.karma ? r.timestamp : r.karma;
            const int high = r.timestamp >= r.karma ? r.timestamp : r.karma;
            db.RangeByTimestamp(low, high, cb);
        }
    }

    ifile.seekg(ios::beg);

    {
        LOG_DURATION("RangeByKarma");
        for (size_t i = 0; i < RecordsCount; ++i)
        {
            Record r;
            ifile >> r.id >> r.title >> r.user >> r.timestamp >> r.karma;
            const int low = r.timestamp <= r.karma ? r.timestamp : r.karma;
            const int high = r.timestamp >= r.karma ? r.timestamp : r.karma;
            db.RangeByKarma(low, high, cb);
        }
    }

    ifile.seekg(ios::beg);

    {
        LOG_DURATION("AllByUser");
        for (size_t i = 0; i < RecordsCount; ++i)
        {
            Record r;
            ifile >> r.id >> r.title >> r.user >> r.timestamp >> r.karma;
            db.AllByUser(r.user, cb);
        }
    }
}

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, TestRangeBoundaries);
    RUN_TEST(tr, TestSameUser);
    RUN_TEST(tr, TestReplacement);

    // MakeInputData();
}

void Profile()
{
    ProfileLong();
}
