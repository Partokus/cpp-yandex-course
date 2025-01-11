#pragma once
#include <cstddef>
#include <stdexcept>
#include <algorithm>

using namespace std;

namespace RAII
{

template <typename Provider>
class Booking
{
public:
    Booking(const Booking &) = delete;
    Booking & operator=(const Booking &) = delete;

    Booking(Provider *provider, int)
        : _provider(provider)
    {
    }

    Booking(Booking &&other)
    {
        std::swap(_provider, other._provider);
    }

    Booking & operator=(Booking &&other)
    {
        std::swap(_provider, other._provider);
        return *this;
    }

    ~Booking()
    {
        if (_provider)
        {
            _provider->CancelOrComplete(*this);
        }
    }

private:
    Provider *_provider = nullptr;
};

} // namespace RAII