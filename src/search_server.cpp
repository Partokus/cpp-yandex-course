#include <search_server.h>
#include <paginator.h>
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <future>

vector<string_view> SplitIntoWords(string_view str)
{
    vector<string_view> result;

    if (str.empty())
    {
        return {};
    }
    else if (str[0] == ' ')
    {
        size_t not_space = str.find_first_not_of(' ');
        str.remove_prefix(not_space);
    }

    while (true)
    {
        size_t space = str.find(' ');
        result.push_back(str.substr(0, space));
        if (space == str.npos)
        {
            break;
        }
        else
        {
            str.remove_prefix(space + 1);
            // ищем лишние пробелы, так как их может быть больше одного
            size_t not_space = str.find_first_not_of(' ');
            if (not_space == str.npos)
            {
                break;
            }
            str.remove_prefix(not_space);
        }
    }
    return result;
}

void Index::Add(string_view document, size_t doc_id)
{
    for (auto &word : SplitIntoWords(document))
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

SearchServer::SearchServer(istream &document_input)
{
    UpdateDocumentBase(document_input);
}

void SearchServer::UpdateDocumentBase(istream &document_input)
{
    _index.data.clear();
    _docs_count = 0U;

    for (string current_document; getline(document_input, current_document);)
    {
        const size_t doc_id = _docs_count++;
        _index.Add(current_document, doc_id);
    }
}

void SearchServer::AddQueriesStream(istream &query_input, ostream &search_results_output)
{
    vector<future<void>> futures;

    _search_results.access().ref.clear();
    _next_search_result_id = 0U;

    for (size_t i = 0; i < ThreadsCount; ++i)
    {
        futures.push_back(async(
            [&query_input, this]
            {
                AddQueriesStreamSingleThread(query_input);
            }));
    }

    for (auto &f : futures)
    {
        f.get();
    }

    // для профилирования
    // _startTime = chrono::steady_clock::now();
    // _dur += chrono::duration_cast<chrono::microseconds>(chrono::steady_clock::now() - _startTime);
    // cout << "time: " << chrono::duration_cast<chrono::milliseconds>(_dur).count() << endl;

    for (auto &[id, line] : _search_results.access().ref)
    {
        search_results_output << line;
    }
}

void SearchServer::AddQueriesStreamSingleThread(istream &query_input)
{
    _m_getline.lock();
    for (string current_query; getline(query_input, current_query);)
    {
        const size_t search_result_id = _next_search_result_id++;
        _m_getline.unlock();

        vector<string_view> words = SplitIntoWords(current_query);

        vector<size_t> doc_id_hits(_docs_count);

        for (const string_view word : words)
        {
            for (const size_t doc_id : _index.Lookup(string(word)))
            {
                ++doc_id_hits[doc_id];
            }
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

        string search_result = move(current_query) + ':';
        for (auto [doc_id, hit_count] : search_results)
        {
            search_result += " {docid: " + to_string(doc_id) + ", hitcount: " + to_string(hit_count) + '}';
        }
        search_result += '\n';

        _search_results.access().ref[search_result_id] = move(search_result);

        _m_getline.lock();
    }
    _m_getline.unlock();
}
