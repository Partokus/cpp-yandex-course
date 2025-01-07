template <class T>
class ObjectPool
{
public:
    T *Allocate();
    T *TryAllocate();

    void Deallocate(T *object);

private:
    queue<unique_ptr<T>> free;
    unordered_set<unique_ptr<T>> allocated; // Изменили на unordered_set
};

template <typename T>
T *ObjectPool<T>::Allocate()
{
    if (free.empty())
    {
        free.push(make_unique<T>());
    }
    auto ptr = move(free.front());
    free.pop();
    T *ret = ptr.get();
    allocated.insert(move(ptr));
    return ret;
}

template <typename T>
T *ObjectPool<T>::TryAllocate()
{
    if (free.empty())
    {
        return nullptr;
    }
    return Allocate();
}

// Убрали функции сравнения, они больше не нужны

template <typename T>
void ObjectPool<T>::Deallocate(T *object)
{
    // Добавили создание временного unique_ptr
    unique_ptr<T> ptr(object);
    auto it = allocated.find(ptr);
    ptr.release();
    if (it == allocated.end())
    {
        throw invalid_argument("");
    }
    free.push(move(allocated.extract(it).value()));
}