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

class SearchServer
{
public:
    SearchServer() = default;
    explicit SearchServer(istream &document_input);
    void UpdateDocumentBase(istream &document_input);
    void AddQueriesStream(istream &query_input, ostream &search_results_output);

private:
    using Word = string;
    using WordEnters = size_t;
    using Doc = map<Word, WordEnters>;
    vector<Doc> _docs; // map<слово, кол-во вхождение>

    void AddQueriesStreamSingleThread(istream &query_input, ostream &search_results_output);

    using DocId = size_t;
    using HitCount = size_t;
    map<DocId, HitCount> LookUp(const vector<string> &query_words)
    {
        map<DocId, HitCount> result;

        for (DocId doc_id = 0U; doc_id < _docs.size(); ++doc_id)
        {
            for (const auto &word : query_words)
            {
                if (auto it = _docs[doc_id].find(word); it != _docs[doc_id].end())
                {
                    result[doc_id] += it->second;
                }
            }
        }
        return move(result);
    }
};
