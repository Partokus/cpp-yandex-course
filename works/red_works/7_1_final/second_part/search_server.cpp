#include "search_server.h"
#include "parse.h"
#include <algorithm>
#include <thread>

SearchServer::SearchServer(istream &document_input)
{
    UpdateDocumentBaseSingleThread(document_input);
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
    _m_updating_index.lock();

    _indexes_using_info.access().ref.updating_index = true;
    // ждём пока текущий индекс освободится
    while (_indexes_using_info.access().ref.cur_index_users_count != 0U)
    {
        this_thread::sleep_for(ThreadSleepTime);
    }

    _cur_index.data.clear();
    _cur_index.docs_count = 0U;
    for (string current_document; getline(document_input, current_document);)
    {
        const size_t doc_id = _cur_index.docs_count++;
        _cur_index.Add(current_document, doc_id);
    }

    _indexes_using_info.access().ref.updating_index = false;
    // ждём пока старый индекс освободится
    while (_indexes_using_info.access().ref.old_index_users_count != 0U)
    {
        this_thread::sleep_for(ThreadSleepTime);
    }

    _old_index = _cur_index;

    _m_updating_index.unlock();
}

void SearchServer::AddQueriesStreamSingleThread(istream &query_input, ostream &search_results_output)
{
    for (string current_query; getline(query_input, current_query);)
    {
        vector<string_view> words = SplitBy(current_query, ' ');

        bool use_old_index = false;
        if (auto info = _indexes_using_info.access(); info.ref.updating_index)
        {
            use_old_index = true;
            ++info.ref.old_index_users_count;
        }
        else
        {
            use_old_index = false;
            ++info.ref.cur_index_users_count;
        }
        auto &index = use_old_index ? _old_index : _cur_index;

        vector<size_t> doc_id_hits(index.docs_count);
        for (const string_view word : words)
        {
            for (const size_t doc_id : index.Lookup(string(word)))
            {
                ++doc_id_hits[doc_id];
            }
        }

        if (use_old_index)
        {
            --_indexes_using_info.access().ref.old_index_users_count;
        }
        else
        {
            --_indexes_using_info.access().ref.cur_index_users_count;
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

const Index::DocIdHits &Index::Lookup(const string &word) const
{
    if (auto it = data.find(word); it != data.end())
    {
        return it->second;
    }
    static DocIdHits empty;
    return empty;
}
