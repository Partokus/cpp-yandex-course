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

using namespace std;

void TestAll();
void Profile();

using namespace std;

struct Record
{
    string id;
    string title;
    string user;
    int timestamp;
    int karma;

    bool operator<(const Record &other) const
    {
        return id < other.id;
    }
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
    set<Record> _ids;
    multimap<int, string_view> _timestamp_to_id;
    multimap<int, string_view> _karma_to_id;
    multimap<string_view, string_view> _user_to_id;

    template<typename Multimap>
    void erase(Multimap &m,
               const typename Multimap::key_type &key,
               const typename Multimap::mapped_type &value);

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

    _timestamp_to_id.emplace(it->timestamp, it->id);
    _karma_to_id.emplace(it->karma, it->id);
    _user_to_id.emplace(it->user, it->id);
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

    erase(_timestamp_to_id, it->timestamp, id);
    erase(_karma_to_id, it->karma, id);
    erase(_user_to_id, it->user, id);

    _ids.erase(it);
    return true;
}

template <typename Callback>
void Database::RangeByTimestamp(int low, int high, Callback callback) const
{
    RangeBy(_timestamp_to_id, low, high, callback);
}

template <typename Callback>
void Database::RangeByKarma(int low, int high, Callback callback) const
{
    RangeBy(_karma_to_id, low, high, callback);
}

template <typename Callback>
void Database::AllByUser(const string &user, Callback callback) const
{
    for (auto [it, end] = _user_to_id.equal_range(user); it != end; ++it)
    {
        const Record &record = *GetById(string{it->second});
        if (const bool keep_searching = callback(record); not keep_searching)
        {
            return;
        }
    }
}

template<typename Multimap>
void Database::erase(Multimap &m,
            const typename Multimap::key_type &key,
            const typename Multimap::mapped_type &value)
{
    for (auto [it, end] = m.equal_range(key); it != end; ++it)
    {
        if (auto value_range = it->second; value_range == value)
        {
            m.erase(it);
            break;
        }
    }
}

template <typename Multimap, typename Callback>
void Database::RangeBy(const Multimap &m, int low, int high, Callback callback) const
{
    auto range_begin = m.lower_bound(low);
    auto range_end = m.upper_bound(high);

    for (auto it = range_begin; it != range_end; ++it)
    {
        const Record &record = *GetById(string{it->second});
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

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, TestRangeBoundaries);
    RUN_TEST(tr, TestSameUser);
    RUN_TEST(tr, TestReplacement);
}

void Profile()
{
}
