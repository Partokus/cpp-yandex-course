#pragma once
#include <synchronized.h>
#include <istream>
#include <ostream>
#include <vector>
#include <map>
#include <string>
#include <mutex>
#include <chrono>
#include <thread>

using namespace std;

using DocIdHits = vector<size_t>;

class Index
{
public:
    void Add(string_view document, size_t doc_id);
    const DocIdHits &Lookup(const string &word) const;

    void operator+=(Index &&other);

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
    mutex _m_getline;
    size_t _docs_count = 0U;
    Synchronized<map<size_t, string>> _search_results;
    size_t _next_search_result_id = 0U;

    const size_t ThreadsCount = thread::hardware_concurrency() ? thread::hardware_concurrency() : 1U;
    static constexpr size_t MaxDocsCount = 50'000U + 1U;
    static constexpr size_t MaxQueriesCount = 500'000U + 1U;
    static constexpr size_t MaxRelevantSearchResults = 5U;

    Index UpdateDocumentBaseSingleThread(istream &document_input);
    void AddQueriesStreamSingleThread(istream &query_input);

    // для профилирования
    // chrono::steady_clock::time_point _startTime;
    // std::chrono::microseconds _dur{0};
};
