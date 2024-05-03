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

using namespace std;

void TestAll();
void Profile();

struct Stats
{
    map<string, int> word_frequences;

    void operator+=(const Stats &other)
    {
        for (const auto &[word, enters_count] : other.word_frequences)
        {
            word_frequences[word] += enters_count;
        }
    }
};

Stats ExploreLine(const set<string_view> &key_words, const string &line)
{
    Stats result{};

    string_view str = line;

    vector<string_view> words;
    while (true)
    {
        size_t space = str.find(' ');
        words.push_back(str.substr(0, space));
        if (space == str.npos)
        {
            break;
        }
        else
        {
            str.remove_prefix(space + 1);
            // ищем лишние пробелы, так как их может быть больше одного
            size_t not_space = str.find_first_not_of(' ');
            str.remove_prefix(not_space);
        }
    }

    for (const auto &word : words)
    {
        if (key_words.count(word))
        {
            ++result.word_frequences[string(word)];
        }
    }
    return result;
}

Stats ExploreKeyWordsSingleThread(const set<string> &key_words, istream &input)
{
    Stats result{};

    set<string_view> key_words_str(key_words.cbegin(), key_words.cend());

    static mutex mutex;
    string line;

    while (true)
    {
        mutex.lock();
        if (not getline(input, line))
        {
            mutex.unlock();
            break;
        }
        mutex.unlock();

        result += ExploreLine(key_words_str, line);
    }

    return result;
}

Stats ExploreKeyWords(const set<string> &key_words, istream &input)
{
    static constexpr size_t UseThreadsCount = 4U;
    Stats result{};

    vector<future<Stats>> futures;
    futures.reserve(UseThreadsCount);

    for (size_t i = 0U; i < UseThreadsCount; ++i)
    {
        futures.push_back(
            async(ExploreKeyWordsSingleThread, ref(key_words), ref(input))
        );
    }

    for (auto &f : futures)
    {
        result += f.get();
    }
    return result;
}

int main()
{
    TestAll();
    Profile();
    return 0;
}

void TestBasic()
{
    const set<string> key_words = {"yangle", "rocks", "sucks", "all"};

    stringstream ss;
    ss << "this new    yangle service   really rocks\n";
    ss << "It sucks when yangle isn't available\n";
    ss << "10 reasons why yangle is       the best IT company\n";
    ss << "yangle rocks others suck\n";
    ss << "Goondex really sucks, but  yangle rocks. Use yangle\n";

    const auto stats = ExploreKeyWords(key_words, ss);
    const map<string, int> expected = {
        {"yangle", 6},
        {"rocks", 2},
        {"sucks", 1}};
    ASSERT_EQUAL(stats.word_frequences, expected);
}

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, TestBasic);
}

void Profile()
{
}