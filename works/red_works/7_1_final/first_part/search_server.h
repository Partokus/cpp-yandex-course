#pragma once
#include <istream>
#include <ostream>
#include <vector>
#include <map>
#include <string>
#include <chrono>

using namespace std;

class Index
{
public:
    using DocIdHits = vector<size_t>;

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
    Index _cur_index;
    size_t _docs_count = 0U;

    static constexpr size_t MaxDocsCount = 50'000U + 1U;
    static constexpr size_t MaxQueriesCount = 500'000U + 1U;
    static constexpr size_t MaxRelevantSearchResults = 5U;

    // для профилирования
    // chrono::steady_clock::time_point _startTime;
    // std::chrono::microseconds _dur{0};
};
