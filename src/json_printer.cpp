#include "json_printer.h"

void PrintJsonString(std::ostream &os, std::string_view value)
{
    os << '"';
    for (char c : value)
    {
        switch (c)
        {
            case '"':
                os << "\\\"";
                break;
            case '\\':
                os << "\\\\";
                break;
            default:
                os << c;
                break;
        }
    }
    os << '"';
}

JsonObjectPrinter<State::Idle> PrintJsonObject(std::ostream &os)
{
    return JsonObjectPrinter<State::Idle>(os);
}