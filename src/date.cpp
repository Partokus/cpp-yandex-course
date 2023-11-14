#include "date.h"
#include <limits>
#include <tuple>
#include <sstream>

using namespace std;

void ensureSymbol(istringstream &ss, const string &date_str);
void checkOverflow(istringstream &ss);
void isDateValid(const Date &date);

Date ParseDate(istream &is)
{
    Date date{};
    string date_str{};

    // получаем полную строку даты, чтобы вывести в случае ошибки
    is >> date_str;

    // парсим дату
    istringstream ss_date(date_str);
    ss_date >> date.year;
    checkOverflow(ss_date);
    ensureSymbol(ss_date, date_str);
    ss_date >> date.month;
    checkOverflow(ss_date);
    ensureSymbol(ss_date, date_str);
    ss_date >> date.day;

    // если считали не int или строка в конце оказалась не пустая, то ошибка
    if ((ss_date.fail() and date.day != numeric_limits<int>::max()) or not ss_date.eof())
    {
        throw runtime_error("Wrong date format: " + date_str);
    }

    isDateValid(date);

    return date;
}

void ensureSymbol(istringstream &ss, const string &date_str)
{
    if (ss.peek() != '-')
    {
        throw runtime_error("Wrong date format: " + date_str);
    }
    ss.ignore(1);
}

void checkOverflow(istringstream &ss)
{
    if (ss.fail())
    {
        // если появилась ошибка, то игнорим её и идём дальше.
        // если это была ошибка из-за переполнения, то следующим символом должен быть '-', если формат правильный
        ss.clear();
    }
}

void isDateValid(const Date &date)
{
    static constexpr int Minmonth = 1;
    static constexpr int Maxmonth = 12;

    static constexpr int MinDay = 1;
    static constexpr int MaxDay = 31;

    if (date.month < Minmonth or date.month > Maxmonth)
    {
        throw runtime_error("Month value is invalid: " + to_string(date.month));
    }

    if (date.day < MinDay or date.day > MaxDay)
    {
        throw runtime_error("Day value is invalid: " + to_string(date.day));
    }
}

bool operator<(const Date &lhs, const Date &rhs)
{
    return make_tuple(lhs.year, lhs.month, lhs.day) < make_tuple(rhs.year, rhs.month, rhs.day);
}

bool operator==(const Date &lhs, const Date &rhs)
{
    return make_tuple(lhs.year, lhs.month, lhs.day) == make_tuple(rhs.year, rhs.month, rhs.day);
}

ostream &operator<<(ostream &os, const Date &date)
{
    os << setfill('0')
       << setw(4) << date.year << "-"
       << setw(2) << date.month << "-"
       << setw(2) << date.day;
    return os;
}