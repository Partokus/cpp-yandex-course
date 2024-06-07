#include <profile.h>
#include <test_runner.h>

#include <algorithm>
#include <iterator>
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <random>
#include <thread>

#include <fstream>

#include <search_server.h>
#include <parse.h>

using namespace std;

void TestAll();
void Profile();

int main()
{
    TestAll();
    Profile();
    return 0;
}

void TestFunctionality(
    const vector<string> &docs,
    const vector<string> &queries,
    const vector<string> &expected)
{
    istringstream docs_input(Join('\n', docs));
    istringstream docs_input2(Join('\n', docs));
    istringstream queries_input(Join('\n', queries));

    SearchServer srv;
    srv.UpdateDocumentBase(docs_input);
    srv.UpdateDocumentBase(docs_input2);
    ostringstream queries_output;
    srv.AddQueriesStream(queries_input, queries_output);

    const string result = queries_output.str();
    const auto lines = SplitBy(Strip(result), '\n');
    ASSERT_EQUAL(lines.size(), expected.size());
    for (size_t i = 0; i < lines.size(); ++i)
    {
        ASSERT_EQUAL(lines[i], expected[i]);
    }
}

void TestSerpFormat()
{
    const vector<string> docs = {
        "london is the capital of great britain",
        "i am travelling down the river"};
    const vector<string> queries = {"london", "the"};
    const vector<string> expected = {
        "london: {docid: 0, hitcount: 1}",
        Join(' ', vector{
                      "the:",
                      "{docid: 0, hitcount: 1}",
                      "{docid: 1, hitcount: 1}"})};

    TestFunctionality(docs, queries, expected);
}

void TestBadAllocFormat()
{
    const vector<string> docs =
        {
            "   c  d d e f   "};
    const vector<string> queries = {"d"};
    const vector<string> expected = {"d: {docid: 0, hitcount: 2}"};

    TestFunctionality(docs, queries, expected);
}

void TestTop5()
{
    const vector<string> docs = {
        "milk a",
        "milk b",
        "milk c",
        "milk d",
        "milk e",
        "milk f",
        "milk g",
        "water a",
        "water b",
        "fire and earth"};

    const vector<string> queries = {"milk", "water", "rock"};
    const vector<string> expected = {
        Join(' ', vector{
                      "milk:",
                      "{docid: 0, hitcount: 1}",
                      "{docid: 1, hitcount: 1}",
                      "{docid: 2, hitcount: 1}",
                      "{docid: 3, hitcount: 1}",
                      "{docid: 4, hitcount: 1}"}),
        Join(' ', vector{
                      "water:",
                      "{docid: 7, hitcount: 1}",
                      "{docid: 8, hitcount: 1}",
                  }),
        "rock:",
    };
    TestFunctionality(docs, queries, expected);
}

void TestHitcount()
{
    const vector<string> docs = {
        "the river goes through the entire city there is a house near it",
        "the wall",
        "walle",
        "is is is is",
    };
    const vector<string> queries = {"the is"};
    const vector<string> expected = {
        // Join(' ', vector{
        //               "the:",
        //               "{docid: 0, hitcount: 2}",
        //               "{docid: 1, hitcount: 1}",
        //           }),
        // "wall: {docid: 1, hitcount: 1}",
        // "all:",
        // Join(' ', vector{
        //               "is:",
        //               "{docid: 3, hitcount: 4}",
        //               "{docid: 0, hitcount: 1}",
        //           }),
        Join(' ', vector{
                      "the is:",
                      "{docid: 3, hitcount: 4}",
                      "{docid: 0, hitcount: 3}",
                      "{docid: 1, hitcount: 1}",
                  }),
    };
    TestFunctionality(docs, queries, expected);
}

void TestRanking()
{
    const vector<string> docs = {
        "london is the capital of great britain",
        "paris is the capital of france",
        "berlin is the capital of germany",
        "rome is the capital of italy",
        "madrid is the capital of spain",
        "lisboa is the capital of portugal",
        "bern is the capital of switzerland",
        "moscow is the capital of russia",
        "kiev is the capital of ukraine",
        "minsk is the capital of belarus",
        "astana is the capital of kazakhstan",
        "beijing is the capital of china",
        "tokyo is the capital of japan",
        "bangkok is the capital of thailand",
        "welcome to moscow the capital of russia the third rome",
        "amsterdam is the capital of netherlands",
        "helsinki is the capital of finland",
        "oslo is the capital of norway",
        "stockgolm is the capital of sweden",
        "riga is the capital of latvia",
        "tallin is the capital of estonia",
        "warsaw is the capital of poland",
    };

    const vector<string> queries = {"moscow is the capital of russia"};
    const vector<string> expected = {
        Join(' ', vector{
                      "moscow is the capital of russia:",
                      "{docid: 7, hitcount: 6}",
                      "{docid: 14, hitcount: 6}",
                      "{docid: 0, hitcount: 4}",
                      "{docid: 1, hitcount: 4}",
                      "{docid: 2, hitcount: 4}",
                  })};
    TestFunctionality(docs, queries, expected);
}

void TestBasicSearch()
{
    const vector<string> docs = {
        "we are ready to go",
        "come on everybody shake you hands",
        "i love this game",
        "just like exception safety is not about writing try catch everywhere in your code move semantics are not about typing double ampersand everywhere in your code",
        "daddy daddy daddy dad dad dad",
        "tell me the meaning of being lonely",
        "just keep track of it",
        "how hard could it be",
        "it is going to be legen wait for it dary legendary",
        "we dont need no education"};

    const vector<string> queries = {
        "we need some help",
        "it",
        "i love this game",
        "i love this game",
        "i love this game",
        "tell me why",
        "dislike",
        "about"};

    const vector<string> expected = {
        Join(' ', vector{
                      "we need some help:",
                      "{docid: 9, hitcount: 2}",
                      "{docid: 0, hitcount: 1}"}),
        Join(' ', vector{
                      "it:",
                      "{docid: 8, hitcount: 2}",
                      "{docid: 6, hitcount: 1}",
                      "{docid: 7, hitcount: 1}",
                  }),
        "i love this game: {docid: 2, hitcount: 4}",
        "i love this game: {docid: 2, hitcount: 4}",
        "i love this game: {docid: 2, hitcount: 4}",
        "tell me why: {docid: 5, hitcount: 2}",
        "dislike:",
        "about: {docid: 3, hitcount: 2}",
    };
    TestFunctionality(docs, queries, expected);
}

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, TestSerpFormat);
    RUN_TEST(tr, TestBadAllocFormat);
    RUN_TEST(tr, TestTop5);
    RUN_TEST(tr, TestHitcount);
    RUN_TEST(tr, TestRanking);
    RUN_TEST(tr, TestBasicSearch);
}

void CreateDocumentsAndQueriesFiles()
{
    constexpr size_t DocsCount = 800U;                // Количество документов. 50'000 max
    constexpr size_t OneDocWordsCount = 1000U;        // 1000 max
    constexpr size_t DocsDifferentWordsCount = 1000U; // 10'000 max
    constexpr size_t WordLenght = 50U;                // 100 max

    constexpr size_t QueriesCount = 10'000U;             // 500'000 max
    constexpr size_t OneQueryDifferentWordsCount = 10U; // 10 max

    constexpr size_t MaxSpacesBtwTwoWordsCount = 10U;

    ofstream docs_ostream("docs.txt");
    ofstream queries_ostream("queries.txt");

    // создаём набор различных слов
    set<string> different_words;
    while (different_words.size() != DocsDifferentWordsCount)
    {
        // создаём слово
        string word;
        while (word.size() < WordLenght)
        {
            word += to_string(rand());
        }
        while (word.size() != WordLenght)
        {
            word.pop_back();
        }

        different_words.insert(word);
    }
    vector<string> words(different_words.begin(), different_words.end());
    different_words.clear();

    // создаём документы
    vector<string> docs(DocsCount);
    for (auto &doc : docs)
    {
        // заполняем документ словами с различным
        // количеством пробелов между ними
        for (size_t i = 0U; i < OneDocWordsCount; ++i)
        {
            const size_t word_index = rand() % DocsDifferentWordsCount;

            doc += words[word_index];

            if (i != OneDocWordsCount - 1)
            {
                size_t spaces_btw_two_words_count = rand() % MaxSpacesBtwTwoWordsCount;
                if (spaces_btw_two_words_count == 0)
                {
                    spaces_btw_two_words_count = 1;
                }
                const string spaces = string(spaces_btw_two_words_count, ' ');

                doc += spaces;
            }
        }

        docs_ostream << doc << '\n';
    }

    // создаём запросы
    vector<string> queries(QueriesCount);
    for (auto &query : queries)
    {
        // заполняем запросы уникальными словами с различным
        // количеством пробелов между ними
        for (size_t i = 0U; i < OneQueryDifferentWordsCount; ++i)
        {
            const size_t word_index = rand() % DocsDifferentWordsCount;

            const size_t spaces_btw_two_words_count = rand() % MaxSpacesBtwTwoWordsCount;
            const string spaces = string(spaces_btw_two_words_count, ' ');

            // подтверждаем уникальность слова
            if (query.find(words[word_index]) != query.npos)
            {
                --i;
                continue;
            }

            query += words[word_index];

            if (i != OneQueryDifferentWordsCount - 1)
            {
                size_t spaces_btw_two_words_count = rand() % MaxSpacesBtwTwoWordsCount;
                if (spaces_btw_two_words_count == 0)
                {
                    spaces_btw_two_words_count = 1;
                }
                const string spaces = string(spaces_btw_two_words_count, ' ');

                query += spaces;
            }
        }

        queries_ostream << query << '\n';
    }
}

void ProfileSearchServer(istream &document_input, istream &query_input,
                         ostream &search_results_output, ifstream &expected_search_results)
{
    LOG_DURATION("SearchServer Total");
    SearchServer srv;
    {
        LOG_DURATION("SearchServer UpdateDocumentBase");
        srv.UpdateDocumentBase(document_input);
    }
    {
        LOG_DURATION("SearchServer AddQueriesStream");
        srv.AddQueriesStream(query_input, search_results_output);
    }

    // ifstream search_results_input("search_results_output.txt");

    // if (not search_results_input)
    // {
    //     throw runtime_error("Didn't open file");
    // }

    // for (string search_result; getline(search_results_input, search_result);)
    // {
    //     string expected_search_result;
    //     getline(expected_search_results, expected_search_result);

    //     ASSERT_EQUAL(search_result, expected_search_result);
    // }
}

void Profile()
{
    // CreateDocumentsAndQueriesFiles();
    // return;

    ifstream docs_file("docs.txt");
    ifstream queries_file("queries.txt");

    ofstream search_results_output("search_results_output.txt");
    ifstream expected_earch_results("expected_earch_results.txt");

    if (not docs_file or not queries_file)
    {
        throw runtime_error("Didn't open file");
    }

    {
        // LOG_DURATION("map and vector");
        // map<size_t, size_t> c;
        // {
        //     LOG_DURATION("constructing");
        //     for (int i = 0; i < 1'000'000; ++i)
        //     {
        //         c[i] = i;
        //     }
        // }

        // {
        //     LOG_DURATION("iterating");
        //     for (auto &i : c)
        //     {
        //         ++i.second;
        //     }
        // }
    }

    ProfileSearchServer(docs_file, queries_file, search_results_output, expected_earch_results);
}
