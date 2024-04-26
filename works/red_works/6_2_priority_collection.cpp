#include <profile.h>
#include <test_runner.h>
#include <vector>
#include <string>
#include <algorithm>
#include <iterator>
#include <set>

using namespace std;

void TestAll();
void Profile();

template <typename T>
class PriorityCollection
{
public:
    using Id = size_t;

    // Добавить объект с нулевым приоритетом
    // с помощью перемещения и вернуть его идентификатор
    Id Add(T object);

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
    struct Object
    {
        Id id = 0U;
        T value;

        bool operator<(const Object &other) const
        {
            return id < other.id;
        }
    };

    using Priority = int;
    using Objects = set<Object>;
    using ObjectIterator = typename Objects::iterator;

    map<Priority, Objects> _collection;
    map<Id, Priority> _objects_priorities;
    map<Id, ObjectIterator> _objects_iterators;

    size_t _next_id = 0U;
};

template <typename T>
typename PriorityCollection<T>::Id PriorityCollection<T>::Add(T object)
{
    const size_t id = _next_id++;

    _collection[0].insert(Object{id, move(object)});

    _objects_priorities[id] = 0U;
    _objects_iterators[id] = prev(_collection[0].end());

    return id;
}

template <typename T>
template <typename ObjInputIt, typename IdOutputIt>
void PriorityCollection<T>::Add(ObjInputIt range_begin, ObjInputIt range_end,
                                IdOutputIt ids_begin)
{
    for (auto it = range_begin; it != range_end; ++it)
    {
        *ids_begin++ = Add(move(*it));
    }
}

template <typename T>
bool PriorityCollection<T>::IsValid(Id id) const
{
    return _objects_priorities.count(id);
}

template <typename T>
const T &PriorityCollection<T>::Get(Id id) const
{
    return _objects_iterators.at(id)->value;
}

template <typename T>
void PriorityCollection<T>::Promote(Id id)
{
    const Priority obj_old_priority = _objects_priorities[id];
    const ObjectIterator obj_old_it = _objects_iterators[id];

    auto node_handle = _collection[obj_old_priority].extract(obj_old_it);

    if (_collection[obj_old_priority].empty())
    {
        _collection.erase(obj_old_priority);
    }

    const Priority obj_new_priority = obj_old_priority + 1U;

    _collection[obj_new_priority].insert(move(node_handle.value()));
    const ObjectIterator obj_new_it = prev(_collection[obj_new_priority].end());

    _objects_priorities[id] = obj_new_priority;
    _objects_iterators[id] = obj_new_it;
}

template <typename T>
pair<const T &, int> PriorityCollection<T>::GetMax() const
{
    const auto &[priority, objects] = *prev(_collection.end());
    return {(prev(objects.end()))->value, priority};
}

template <typename T>
pair<T, int> PriorityCollection<T>::PopMax()
{
    pair<T, int> result;

    auto it_objects = prev(_collection.end());

    auto &[priority, objects] = *it_objects;

    auto node_handle = objects.extract(prev(objects.end()));

    Object &obj = node_handle.value();

    result.first = move(obj.value);
    result.second = priority;

    Id deleting_id = obj.id;

    if (objects.empty())
    {
        _collection.erase(it_objects);
    }

    _objects_priorities.erase(deleting_id);
    _objects_iterators.erase(deleting_id);

    return result;
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

void Profile()
{
}