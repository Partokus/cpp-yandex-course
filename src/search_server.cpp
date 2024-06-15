#include "search_server.h"
#include "parse.h"
#include <algorithm>

SearchServer::SearchServer(istream &document_input)
{
    UpdateDocumentBase(document_input);
}

void SearchServer::UpdateDocumentBase(istream &document_input)
{
    _futures.push_back(
        async(&SearchServer::UpdateDocumentBaseSingleThread, this, ref(document_input))
    );
}

void SearchServer::AddQueriesStream(istream &query_input, ostream &search_results_output)
{
    _futures.push_back(
        async(&SearchServer::AddQueriesStreamSingleThread, this, ref(query_input), ref(search_results_output))
    );
}

void SearchServer::UpdateDocumentBaseSingleThread(istream &document_input)
{
    Index new_index;
    size_t new_docs_count = 0U;

    for (string current_document; getline(document_input, current_document);)
    {
        const size_t doc_id = new_docs_count++;
        new_index.Add(current_document, doc_id);
    }

    _sync_bridge.access().ref.updating_base = true;

    while (_sync_bridge.access().ref.looking_up_count != 0U)
    {
    }
    _index = move(new_index);
    _docs_count = new_docs_count;

    _sync_bridge.access().ref.updating_base = false;
}

void SearchServer::AddQueriesStreamSingleThread(istream &query_input, ostream &search_results_output)
{
    for (string current_query; getline(query_input, current_query);)
    {
        vector<string_view> words = SplitBy(current_query, ' ');

        vector<size_t> doc_id_hits(_docs_count);

        for (const string_view word : words)
        {
            while (true)
            {
                if (auto &sync_bridge = _sync_bridge.access().ref; not sync_bridge.updating_base)
                {
                    ++sync_bridge.looking_up_count;
                    if (doc_id_hits.size() < _docs_count)
                    {
                        doc_id_hits.resize(_docs_count);
                    }
                    break;
                }
            }

            for (const size_t doc_id : _index.Lookup(string(word)))
            {
                ++doc_id_hits[doc_id];
            }

            --_sync_bridge.access().ref.looking_up_count;
        }

        vector<pair<size_t, size_t>> search_results;
        search_results.reserve(MaxRelevantSearchResults);

        // находим топ документов по релевантности
        while (search_results.size() != MaxRelevantSearchResults)
        {
            auto it_max_hits = max_element(doc_id_hits.begin(), doc_id_hits.end());
            if (*it_max_hits == 0U)
            {
                break;
            }
            const size_t doc_id = it_max_hits - doc_id_hits.begin();
            size_t &hits_count = *it_max_hits;
            search_results.emplace_back(doc_id, hits_count);
            hits_count = 0U;
        }

        sort(begin(search_results),
             end(search_results),
             [](pair<size_t, size_t> lhs, pair<size_t, size_t> rhs)
             {
                 if (rhs.second < lhs.second)
                 {
                     return true;
                 }
                 else if (rhs.second == lhs.second)
                 {
                     return rhs.first > lhs.first;
                 }
                 return false;
             }
        );

        search_results_output << current_query << ':';
        for (auto [doc_id, hit_count] : search_results)
        {
            search_results_output << " {docid: " << doc_id << ", hitcount: " << hit_count << '}';
        }
        search_results_output << '\n';
    }
}

void Index::Add(string_view document, size_t doc_id)
{
    for (auto &word : SplitBy(document, ' '))
    {
        data[string(word)].push_back(doc_id);
    }
}

const DocIdHits &Index::Lookup(const string &word) const
{
    if (auto it = data.find(word); it != data.end())
    {
        return it->second;
    }
    static DocIdHits empty;
    return empty;
}
