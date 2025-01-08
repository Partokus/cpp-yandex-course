#include "Common.h"
#include <deque>
#include <algorithm>
#include <atomic>
#include <mutex>
#include <iostream>

using namespace std;

class LruCache : public ICache
{
public:
    LruCache(shared_ptr<IBooksUnpacker> books_unpacker,
             const Settings &settings)
        : _books_unpacker(books_unpacker),
          _settings(settings)
    {
    }

    BookPtr GetBook(const string &book_name) override
    {
        {
            lock_guard g(_mutex);
            auto it = find_if(_books.cbegin(), _books.cend(),
                [&book_name](const BookPtr &v){
                return v->GetName() == book_name;
            });

            // если книга уже есть в кэше
            if (it != _books.cend())
            {
                if (it != _books.cbegin())
                {
                    // перемещаем книгу в начало очереди,
                    // показывая, что она имеет максимальный ранг
                    BookPtr temp = move(*it);
                    _books.erase(it);
                    _books.push_front(move(temp));
                }
                return _books.front();
            }
        }

        BookPtr new_book = move(_books_unpacker->UnpackBook(book_name));

        const size_t new_book_size = new_book->GetContent().size();

        // если книга больше максимального размера кэша, то
        // не имеет смысла её кэшировать, сразу отдаём дальше
        if (new_book_size > _settings.max_memory)
        {
            return new_book;
        }

        // пока в кэше нет места для новой книги,
        // удаляем книги с конца очереди в соответствии
        // с их маленьким рангом
        lock_guard g(_mutex);
        while (new_book_size > _remain_memory)
        {
            _remain_memory += _books.back()->GetContent().size();
            _books.pop_back();
        }

        _books.push_front(move(new_book));
        _remain_memory -= new_book_size;

        return _books.front();
    }

private:
    shared_ptr<IBooksUnpacker> _books_unpacker;
    const Settings &_settings;
    deque<BookPtr> _books;
    size_t _remain_memory = _settings.max_memory;
    mutex _mutex;
};

unique_ptr<ICache> MakeCache(
    shared_ptr<IBooksUnpacker> books_unpacker,
    const ICache::Settings &settings)
{
    return make_unique<LruCache>(move(books_unpacker), settings);
}
