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

vector<string> SplitIntoWords(const string &line)
{
    // vector<string_view> result;
    // while (true)
    // {
    //     size_t space = str.find(' ');
    //     result.push_back(str.substr(0, space));
    //     if (space == str.npos)
    //     {
    //         break;
    //     }
    //     else
    //     {
    //         str.remove_prefix(space + 1);
    //         // ищем лишние пробелы, так как их может быть больше одного
    //         size_t not_space = str.find_first_not_of(' ');
    //         str.remove_prefix(not_space);
    //     }
    // }
    // return result;
    istringstream words_input(line);
    return {istream_iterator<string>(words_input), istream_iterator<string>()};
}

SearchServer::SearchServer(istream &document_input)
{
    UpdateDocumentBase(document_input);
}

void SearchServer::UpdateDocumentBase(istream &document_input)
{
    static constexpr size_t MaxDocsCount = 50'000U + 1U;
    _docs.clear();
    _docs.reserve(MaxDocsCount);

    for (string current_document; getline(document_input, current_document);)
    {
        _docs.push_back({});
        Doc &doc = _docs.back();

        string_view str = current_document;
        while (true)
        {
            size_t space = str.find(' ');
            ++doc[string(str.substr(0, space))];
            if (space == str.npos)
            {
                break;
            }
            else
            {
                str.remove_prefix(space + 1);
                // ищем лишние пробелы, так как их может быть больше одного
                size_t not_space = str.find_first_not_of(' ');
                str.remove_prefix(not_space);
            }
        }
    }

    _docs.shrink_to_fit();
}

void SearchServer::AddQueriesStream(istream &query_input, ostream &search_results_output)
{
    static constexpr size_t ThreadsCount = 1U;

    vector<future<void>> futures;

    for (size_t i = 0; i < ThreadsCount; ++i)
    {
        futures.push_back(async(
            [&query_input, &search_results_output, this]
            {
                AddQueriesStreamSingleThread(query_input, search_results_output);
            }));
    }

    for (auto &f : futures)
    {
        f.get();
    }
}

void SearchServer::AddQueriesStreamSingleThread(istream &query_input, ostream &search_results_output)
{
    static mutex mutex_get_line;

    for (string current_query; getline(query_input, current_query);)
    {
        const auto words = SplitIntoWords(current_query);

        map<DocId, HitCount> docid_count = LookUp(words);

        vector<pair<size_t, size_t>> search_results(docid_count.begin(), docid_count.end());

        sort(
            begin(search_results),
            end(search_results),
            [](pair<size_t, size_t> lhs, pair<size_t, size_t> rhs)
            {
                int64_t lhs_docid = lhs.first;
                auto lhs_hit_count = lhs.second;
                int64_t rhs_docid = rhs.first;
                auto rhs_hit_count = rhs.second;
                return make_pair(lhs_hit_count, -lhs_docid) > make_pair(rhs_hit_count, -rhs_docid);
            });

        search_results_output << current_query << ':';
        for (auto [docid, hitcount] : Head(search_results, 5))
        {
            search_results_output << " {"
                                  << "docid: " << docid << ", "
                                  << "hitcount: " << hitcount << '}';
        }
        search_results_output << '\n';
    }
}

// void InvertedIndex::Add(string document)
// {
//     docs.push_back(move(document));

//     const size_t docid = docs.size() - 1;
//     for (auto &word : SplitIntoWords(docs.back()))
//     {
//         index[move(word)].push_back(docid);
//     }
// }

// const list<size_t> &InvertedIndex::Lookup(const string &word) const
// {
//     if (auto it = index.find(word); it != index.end())
//     {
//         return it->second;
//     }
//     else
//     {
//         static list<size_t> empty_list;
//         return empty_list;
//     }
// }
