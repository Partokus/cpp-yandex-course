#pragma once
#include "synchronized.h"
#include <istream>
#include <ostream>
#include <vector>
#include <map>
#include <string>
#include <chrono>
#include <future>

using namespace std;

using DocIdHits = vector<size_t>;

class Index
{
public:
    void Add(string_view document, size_t doc_id);
    const DocIdHits &Lookup(const string &word) const;

    map<string, DocIdHits> data;
    size_t docs_count = 0U;
};

class SearchServer
{
public:
    SearchServer() = default;
    explicit SearchServer(istream &document_input);
    void UpdateDocumentBase(istream &document_input);
    void AddQueriesStream(istream &query_input, ostream &search_results_output);

private:
    Index _cur_index;
    Index _old_index;

    struct IndexesUsingInfo
    {
        bool updating_index = false;
        size_t cur_index_users_count = 0U;
        size_t old_index_users_count = 0U;
    };

    Synchronized<IndexesUsingInfo> _indexes_using_info{};

    mutex _m_update_index;

    vector<future<void>> _futures;

    void UpdateDocumentBaseSingleThread(istream &document_input);
    void AddQueriesStreamSingleThread(istream &query_input, ostream &search_results_output);

    static constexpr size_t MaxDocsCount = 50'000U + 1U;
    static constexpr size_t MaxQueriesCount = 500'000U + 1U;
    static constexpr size_t MaxRelevantSearchResults = 5U;

    static constexpr chrono::milliseconds ThreadSleepTime{1};

    // для профилирования
    // chrono::steady_clock::time_point _startTime;
    // std::chrono::microseconds _dur{0};
};
