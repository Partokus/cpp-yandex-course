#pragma once
#include <cinttypes>
#include <iostream>
#include <ostream>
#include <string_view>
#include <type_traits>

void PrintJsonString(std::ostream &os, std::string_view value);

enum class State
{
    Idle,
    Number,
    String,
    Boolean,
    Null,
    Key,
    BeginArray,
    EndArray,
    BeginObject,
    EndObject
};

template <State state = State::Idle>
class JsonObjectPrinter
{
public:
    JsonObjectPrinter(std::ostream &os) : _os(os)
    {
        if constexpr (state == State::Idle)
        {
            _os << "{";
        }
    }
    ~JsonObjectPrinter()
    {
        if constexpr (state == State::Idle)
        {
            _os << "}";
        }
        else if constexpr (state == State::BeginObject)
        {
        
        }
    }

    // T Number(int64_t value) { _os << value; }
    JsonObjectPrinter<State::String> String(std::string_view value)
    {
        static_assert(state != State::Idle, "First time need print Key");
        PrintJsonString(_os, value);
        return {_os};
    }

    JsonObjectPrinter<State::Key> Key(std::string_view value)
    {
        static_assert(state != State::Key, "Can't print Key immediatly after printed Key");
        if constexpr (state != State::Idle)
        {
            _os << ",";
        }
        PrintJsonString(_os, value);
        _os << ":";
        return {_os};
    }

    JsonObjectPrinter<State::BeginObject> BeginObject()
    {
        static_assert(state == State::EndArray, "Can't do BeginObject after EndObject");
        if constexpr (state != State::Idle)
        {
            _os << ",";
        }
        PrintJsonString(_os, value);
        _os << ":";
        return {_os};
    }

    JsonObjectPrinter<State::Boolean> Boolean(bool value) { value ? (_os << "true") : (_os << "false"); return {_os}; }
    JsonObjectPrinter<State::Null> Null() { _os << "null"; return {_os}; }



private:
    std::ostream &_os;
};

JsonObjectPrinter<State::Idle> PrintJsonObject(std::ostream &os);