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
#include <tuple>

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
    unordered_set<Record, RecordHasher> _records;
    using RecordIt = unordered_set<Record, RecordHasher>::const_iterator;

    unordered_map<string_view, RecordIt> _id_index;
    multimap<int, RecordIt> _timestamp_index;
    multimap<int, RecordIt> _karma_index;
    unordered_multimap<string_view, RecordIt> _user_index;

    struct Iterators
    {
        unordered_map<string_view, RecordIt>::iterator id_it{};
        multimap<int, RecordIt>::iterator timestamp_it{};
        multimap<int, RecordIt>::iterator karma_it{};
        unordered_multimap<string_view, RecordIt>::iterator user_it{};
    };
    unordered_map<string_view, Iterators> _index_iterators;
};

bool Database::Put(const Record &record)
{
    auto [it, inserted] = _records.insert(record);
    if (not inserted)
    {
        return false;
    }

    Iterators iters{};

    iters.id_it = _id_index.emplace(it->id, it).first;
    iters.timestamp_it = _timestamp_index.emplace(it->timestamp, it);
    iters.karma_it = _karma_index.emplace(it->karma, it);
    iters.user_it = _user_index.emplace(it->user, it);

    _index_iterators.emplace(it->id, iters);

    return true;
}

const Record *Database::GetById(const string &id) const
{
    if (auto it = _id_index.find(id); it != _id_index.end())
    {
        const Record &record = *it->second;
        return &record;
    }
    return nullptr;
}

bool Database::Erase(const string &id)
{
    auto it = _id_index.find(id);
    if (it == _id_index.end())
    {
        return false;
    }

    auto it_iterators = _index_iterators.find(id);
    Iterators iterators = it_iterators->second;

    _id_index.erase(iterators.id_it);
    _timestamp_index.erase(iterators.timestamp_it);
    _karma_index.erase(iterators.karma_it);
    _user_index.erase(iterators.user_it);

    _index_iterators.erase(it_iterators);

    const Record &record = *it->second;
    _records.erase(record);
    return true;
}

template <typename Callback>
void Database::RangeByTimestamp(int low, int high, Callback callback) const
{
    auto begin = _timestamp_index.lower_bound(low);
    auto end = _timestamp_index.upper_bound(high);

    for (auto it = begin; it != end; ++it)
    {
        const Record &record = *it->second;
        if (not callback(record))
        {
            return;
        }
    }
}

template <typename Callback>
void Database::RangeByKarma(int low, int high, Callback callback) const
{
    auto begin = _karma_index.lower_bound(low);
    auto end = _karma_index.upper_bound(high);

    for (auto it = begin; it != end; ++it)
    {
        const Record &record = *it->second;
        if (not callback(record))
        {
            return;
        }
    }
}

template <typename Callback>
void Database::AllByUser(const string &user, Callback callback) const
{
    for (auto [it, end] = _user_index.equal_range(user); it != end; ++it)
    {
        const Record &record = *it->second;
        if (not callback(record))
        {
            return;
        }
    }
}

int main()
{
    TestAll();
    Profile();
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

    ASSERT(not db.Put({"id2", "111", "222", 1, 11}));

    count = 0;
    db.RangeByTimestamp(1536107260, 1536107260, [&count](const Record &)
                        {
        ++count;
    return true; });

    ASSERT_EQUAL(2, count);

    db.Put({"id3", "O>>-<", "master2", 1536107261, bad_karma});

    count = 0;
    db.RangeByTimestamp(1536107260, 1536107261, [&count](const Record &)
                        {
        ++count;
    return true; });

    ASSERT_EQUAL(3, count);

    count = 0;
    db.RangeByTimestamp(1536107259, 1536107259, [&count](const Record &)
                        {
        ++count;
    return true; });

    ASSERT_EQUAL(0, count);

    db.Erase("id1");
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
    // ifstream ifile("../input_data.txt");

    vector<Record> records;
    records.reserve(RecordsCount);

    if (not ifile)
    {
        throw runtime_error("Didn't open file");
    }

    for (size_t i = 0; i < RecordsCount; ++i)
    {
        Record r;
        ifile >> r.id >> r.title >> r.user >> r.timestamp >> r.karma;
        records.push_back(r);
    }

    {
        LOG_DURATION("Put");
        for (const Record &r : records)
        {
            db.Put(r);
        }
    }

    {
        LOG_DURATION("GetById");
        for (const Record &r : records)
        {
            db.GetById(r.id);
        }
    }

    size_t counter = 0U;
    auto cb = [&counter](const Record &rec)
    {
        ++counter;
        return true;
    };

    {
        LOG_DURATION("RangeByTimestamp");
        for (const Record &r : records)
        {
            const int low = r.timestamp <= r.karma ? r.timestamp : r.karma;
            const int high = r.timestamp >= r.karma ? r.timestamp : r.karma;
            db.RangeByTimestamp(low, high, cb);
        }
    }

    {
        LOG_DURATION("RangeByKarma");
        for (const Record &r : records)
        {
            const int low = r.timestamp <= r.karma ? r.timestamp : r.karma;
            const int high = r.timestamp >= r.karma ? r.timestamp : r.karma;
            db.RangeByKarma(low, high, cb);
        }
    }

    {
        LOG_DURATION("AllByUser");
        for (const Record &r : records)
        {
            db.AllByUser(r.user, cb);
        }
    }

    {
        LOG_DURATION("EraseAll");
        for (const Record &r : records)
        {
            db.Erase(r.id);
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
    // ProfileLong();
}
