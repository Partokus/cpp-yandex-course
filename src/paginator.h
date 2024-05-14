#pragma once
#include <iterator_range.h>
#include <vector>
#include <cmath>

using namespace std;

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