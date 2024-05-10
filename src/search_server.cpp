#include "search_server.h"
#include "iterator_range.h"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <iostream>
#include <cstdint>
#include <cmath>
#include <future>

template <class Iterator>
class Paginator
{
public:
    using Page = IteratorRange<Iterator>;

    Paginator(Iterator begin, Iterator end, size_t page_size)
    {
        _pages.reserve(std::ceil(static_cast<double>(end - begin) / page_size));

        Iterator it = begin;
        const size_t last_page_num = _pages.capacity() - 1;

        for (size_t page_num = 0; page_num < _pages.capacity(); ++page_num)
        {
            if (page_num != last_page_num)
            {
                _pages.push_back({it, it + page_size});
                it += page_size;
            }
            else
            {
                _pages.push_back({it, end});
            }
        }
    }

    typename vector<Page>::iterator begin()
    {
        return _pages.begin();
    }

    typename vector<Page>::iterator end()
    {
        return _pages.end();
    }

    typename vector<Page>::iterator begin() const
    {
        return _pages.begin();
    }

    typename vector<Page>::iterator end() const
    {
        return _pages.end();
    }

    size_t size() const
    {
        return _pages.size();
    }

private:
    vector<Page> _pages{};
};

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

SearchServer::SearchServer(istream &document_input)
{
    UpdateDocumentBase(document_input);
}

void SearchServer::UpdateDocumentBase(istream &document_input)
{
    vector<future<InvertedIndex>> futures;
    index.index.clear();
    next_doc_id = 0U;

    for (size_t i = 0; i < ThreadsCount; ++i)
    {
        futures.push_back(async(
            [&document_input, this]
            {
                return UpdateDocumentBaseSingleThread(document_input);
            }));
    }

    for (auto &f : futures)
    {
        index += f.get();
    }
}

InvertedIndex SearchServer::UpdateDocumentBaseSingleThread(istream &document_input)
{
    InvertedIndex new_index;

    _m_getline.lock();
    for (string current_document; getline(document_input, current_document);)
    {
        size_t doc_id = next_doc_id++;
        _m_getline.unlock();

        new_index.Add(move(current_document), doc_id);

        _m_getline.lock();
    }
    _m_getline.unlock();

    return new_index;
}

void SearchServer::AddQueriesStream(istream &query_input, ostream &search_results_output)
{
    vector<future<void>> futures;

    _search_results.GetAccess().ref_to_value.clear();
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

    for (auto &search_result : _search_results.GetAccess().ref_to_value)
    {
        search_results_output << search_result.second;
    }
}

void SearchServer::AddQueriesStreamSingleThread(istream &query_input)
{
    _m_getline.lock();
    for (string current_query; getline(query_input, current_query);)
    {
        const size_t search_result_id = _next_search_result_id++;
        _m_getline.unlock();

        const auto words = SplitIntoWords(current_query);

        map<size_t, size_t> docid_count;

        for (const auto &word : words)
        {
            for (const size_t docid : index.Lookup(string(word)))
            {
                ++docid_count[docid];
            }
        }

        vector<pair<size_t, size_t>> search_results(docid_count.begin(), docid_count.end());

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
             });

        string search_result = move(current_query) + ':';

        for (auto [docid, hitcount] : Head(search_results, 5))
        {
            search_result += " {docid: " + to_string(docid) + ", hitcount: " + to_string(hitcount) + '}';
        }
        search_result += '\n';

        _search_results.GetAccess().ref_to_value[search_result_id] = move(search_result);

        _m_getline.lock();
    }
    _m_getline.unlock();
}

void InvertedIndex::Add(string document, size_t doc_id)
{
    for (auto &word : SplitIntoWords(document))
    {
        index[string(word)].push_back(doc_id);
    }
}

const DocIdHits &InvertedIndex::Lookup(const string &word) const
{
    if (auto it = index.find(word); it != index.end())
    {
        return it->second;
    }
    else
    {
        static DocIdHits empty;
        return empty;
    }
}
