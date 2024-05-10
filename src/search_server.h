#pragma once

#include <istream>
#include <ostream>
#include <set>
#include <list>
#include <vector>
#include <map>
#include <string>
#include <mutex>

using namespace std;

class InvertedIndex
{
public:
  void Add(string document);
  const vector<size_t> &Lookup(const string &word) const;

private:
  map<string, vector<size_t>> index;
  size_t next_doc_id = 0U;
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

  Synchronized<map<size_t, string>> _search_results; // first is id
  size_t _next_search_result_id = 0U;

  void AddQueriesStreamSingleThread(istream &query_input);
};
