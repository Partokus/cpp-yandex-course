#include <profile.h>
#include <iostream>
#include <test_runner.h>
#include <vector>
#include <string>
#include <algorithm>
#include <iterator>
#include <numeric>
#include <list>
#include <utility>
#include <functional>

using namespace std;

void TestAll();
void Profile();

template <typename T>
class PriorityCollection
{
public:
    using Priority = size_t;
    using Objects = list<T>;
    using ObjectIterator = typename Objects::iterator;
    using Id = ObjectIterator *;

    // Добавить объект с нулевым приоритетом
    // с помощью перемещения и вернуть его идентификатор
    Id Add(T &&object);

    // Добавить все элементы диапазона [range_begin, range_end)
    // с помощью перемещения, записав выданные им идентификаторы
    // в диапазон [ids_begin, ...)
    template <typename ObjInputIt, typename IdOutputIt>
    void Add(ObjInputIt range_begin, ObjInputIt range_end,
             IdOutputIt ids_begin);

    // Определить, принадлежит ли идентификатор какому-либо
    // хранящемуся в контейнере объекту
    bool IsValid(Id id) const;

    // Получить объект по идентификатору
    const T &Get(Id id) const;

    // Увеличить приоритет объекта на 1
    void Promote(Id id);

    // Получить объект с максимальным приоритетом и его приоритет
    pair<const T &, int> GetMax() const;

    // Аналогично GetMax, но удаляет элемент из контейнера
    pair<T, int> PopMax();

private:
    map<Priority, Objects> _collection;
    map<Id, Priority> _object_priority;
};

template <typename T>
typename PriorityCollection<T>::Id PriorityCollection<T>::Add(T &&object)
{
    _collection[0].push_back(move(object));
    ObjectIterator it_back = prev(_collection[0].end());
    ObjectIterator it_back2 = prev(it_back);
    Id ptr_on_it_back = &it_back2;
    size_t ptr1 = reinterpret_cast<size_t>(ptr_on_it_back);
    ptr_on_it_back = &it_back;
    size_t ptr2 = reinterpret_cast<size_t>(ptr_on_it_back);
    _object_priority[ptr_on_it_back] = 0U;
    return ptr_on_it_back;
}

template <typename T>
template <typename ObjInputIt, typename IdOutputIt>
void PriorityCollection<T>::Add(ObjInputIt range_begin, ObjInputIt range_end,
                                IdOutputIt ids_begin)
{
    for (auto it = range_begin; it != range_end; ++it)
    {
        *ids_begin++ = Add(*it);
    }
}

template <typename T>
bool PriorityCollection<T>::IsValid(Id id) const
{
    return _object_priority.count(id);
}

template <typename T>
const T &PriorityCollection<T>::Get(Id id) const
{
    const ObjectIterator it = *id;
    return *it;
}

template <typename T>
void PriorityCollection<T>::Promote(Id id)
{
    const Priority priority = _object_priority[id];
    const Priority next_priority = priority + 1U;
    const ObjectIterator it = *id;
    _collection[next_priority].push_back(move(*it));
    _collection[priority].erase(it);
}

template <typename T>
pair<const T &, int> PriorityCollection<T>::GetMax() const
{
    const auto &[priority, objects] = _collection.back();
    return {objects.back(), priority};
}

template <typename T>
pair<T, int> PriorityCollection<T>::PopMax()
{
    auto it_collection_back = prev(_collection.end());
    auto &[priority, objects] = *it_collection_back;
    T obj = move(objects.back());
    objects.pop_back();
    if (objects.empty())
    {
        _collection.erase(it_collection_back);
    }
    return {move(obj), priority};
}





int main()
{
    TestAll();
    Profile();
    return 0;
}

class StringNonCopyable : public string
{
public:
    using string::string; // Позволяет использовать конструкторы строки
    StringNonCopyable(const StringNonCopyable &) = delete;
    StringNonCopyable(StringNonCopyable &&) = default;
    StringNonCopyable &operator=(const StringNonCopyable &) = delete;
    StringNonCopyable &operator=(StringNonCopyable &&) = default;
};

void TestNoCopy()
{
    PriorityCollection<StringNonCopyable> strings;
    const auto white_id = strings.Add("white");
    const auto yellow_id = strings.Add("yellow");
    const auto red_id = strings.Add("red");

    strings.Promote(yellow_id);
    for (int i = 0; i < 2; ++i)
    {
        strings.Promote(red_id);
    }
    strings.Promote(yellow_id);
    {
        const auto item = strings.PopMax();
        ASSERT_EQUAL(item.first, "red");
        ASSERT_EQUAL(item.second, 2);
    }
    {
        const auto item = strings.PopMax();
        ASSERT_EQUAL(item.first, "yellow");
        ASSERT_EQUAL(item.second, 2);
    }
    {
        const auto item = strings.PopMax();
        ASSERT_EQUAL(item.first, "white");
        ASSERT_EQUAL(item.second, 0);
    }
}

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, TestNoCopy);
}

// class Logger
// {
// public:
//     Logger() { cout << "Default ctor\n"; }
//     Logger(const Logger &) { cout << "Copy ctor\n"; }
//     void operator=(const Logger &) { cout << "Copy assignment ctor\n"; }
//     Logger(Logger &&) { cout << "Move ctor\n"; }
//     void operator=(Logger &&) { cout << "Move assignment ctor\n"; }
// };

// template <typename T>
// void PushBack(T &&object)
// {

// }

void Profile()
{
    // Logger log;

    // PushBack(log);
}
