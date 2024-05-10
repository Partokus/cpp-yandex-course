#pragma once

#include <iostream>
#include <istream>
#include <ostream>
#include <set>
#include <list>
#include <vector>
#include <map>
#include <string>
#include <mutex>

using namespace std;

using DocIdHits = vector<size_t>;

class InvertedIndex
{
public:
    void Add(string document, size_t doc_id);
    const DocIdHits &Lookup(const string &word) const;

    
    map<string, DocIdHits> index;

    void operator+=(InvertedIndex &&other)
    {
        while (not other.index.empty())
        {
            auto node = other.index.extract(other.index.begin());
            DocIdHits &doc_id_hits = index[move(node.key())];
            doc_id_hits.insert(doc_id_hits.begin(), node.mapped().begin(), node.mapped().end());
        }
    }
};

template <typename T>
class Synchronized
{
public:
    explicit Synchronized(T initial = T());

    struct Access
    {
        T &ref_to_value;
        lock_guard<mutex> guard;
    };

    Access GetAccess();

private:
    T _value;
    mutex _m;
};

template <typename T>
Synchronized<T>::Synchronized(T initial)
    : _value(move(initial))
{
}

template <typename T>
typename Synchronized<T>::Access Synchronized<T>::GetAccess()
{
    return {_value, lock_guard(_m)};
}

class SearchServer
{
public:
    SearchServer() = default;
    explicit SearchServer(istream &document_input);
    void UpdateDocumentBase(istream &document_input);
    void AddQueriesStream(istream &query_input, ostream &search_results_output);

private:
    InvertedIndex index;
    mutex _m_getline;

    size_t next_doc_id = 0U;

    Synchronized<map<size_t, string>> _search_results; // first is search result id
    size_t _next_search_result_id = 0U;

    InvertedIndex UpdateDocumentBaseSingleThread(istream &document_input);
    void AddQueriesStreamSingleThread(istream &query_input);

    static constexpr size_t ThreadsCount = 4U;
};
