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
};

class SearchServer
{
public:
    SearchServer() = default;
    explicit SearchServer(istream &document_input);
    void UpdateDocumentBase(istream &document_input);
    void AddQueriesStream(istream &query_input, ostream &search_results_output);

private:
    Index _index;
    size_t _docs_count = 0U;

    struct SyncBridge
    {
        bool updating_base = false;
        size_t looking_up_count = 0U;
    };
    Synchronized<SyncBridge> _sync_bridge{};

    vector<future<void>> _futures;

    void UpdateDocumentBaseSingleThread(istream &document_input);
    void AddQueriesStreamSingleThread(istream &query_input, ostream &search_results_output);

    static constexpr size_t MaxDocsCount = 50'000U + 1U;
    static constexpr size_t MaxQueriesCount = 500'000U + 1U;
    static constexpr size_t MaxRelevantSearchResults = 5U;

    // для профилирования
    // chrono::steady_clock::time_point _startTime;
    // std::chrono::microseconds _dur{0};
};
