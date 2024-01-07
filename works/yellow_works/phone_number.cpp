#include <test_runner.h>
#include <phone_number.h>
#include <algorithm>

using namespace std;

string check_and_get_next_word(stringstream &ss, const string &hint, const char line_end)
{
    string result;
    getline(ss, result, line_end);
    if (ss.fail() or result.empty())
    {
        throw invalid_argument(hint);
    }
    return result;
}

void parse(const string &international_number,
           string &country_code,
           string &city_code,
           string &local_number)
{
    stringstream ss(international_number);

    if (ss.peek() != '+')
    {
        throw invalid_argument("'+' lost");
    }
    ss.ignore(1);

    country_code = check_and_get_next_word(ss, "country code empty", '-');
    city_code = check_and_get_next_word(ss, "city code empty", '-');
    local_number = check_and_get_next_word(ss, "local number empty", '\n');
}

PhoneNumber::PhoneNumber(const string &international_number)
{
    parse(international_number, country_code_, city_code_, local_number_);
}

string PhoneNumber::GetCountryCode() const
{
    return country_code_;
}

string PhoneNumber::GetCityCode() const
{
    return city_code_;
}

string PhoneNumber::GetLocalNumber() const
{
    return local_number_;
}

string PhoneNumber::GetInternationalNumber() const
{
    return "+" + country_code_ + "-" + city_code_ + "-" + local_number_;
}

bool IsPhoneNumberCorrect(const string &international_number)
{
    try
    {
        PhoneNumber pn{international_number};
        return true;
    }
    catch (const std::exception &e)
    {
        return false;
    }
}

void TestPhoneNumberConstructor()
{
    Assert(not IsPhoneNumberCorrect("7964-799-3255"), "Phone number constructor 0");
    Assert(not IsPhoneNumberCorrect("+3-495"), "Phone number constructor 1");
    Assert(not IsPhoneNumberCorrect("+3-"), "Phone number constructor 2");
    Assert(not IsPhoneNumberCorrect(""), "Phone number constructor 3");
    Assert(not IsPhoneNumberCorrect("-3"), "Phone number constructor 4");
    Assert(not IsPhoneNumberCorrect("+3-495"), "Phone number constructor 5");
    Assert(not IsPhoneNumberCorrect("+3-495-"), "Phone number constructor 6");
    Assert(not IsPhoneNumberCorrect("+3-495+5"), "Phone number constructor 7");
    Assert(not IsPhoneNumberCorrect("+3+495-5"), "Phone number constructor 8");
    Assert(not IsPhoneNumberCorrect("+3a495b5"), "Phone number constructor 9");
    Assert(not IsPhoneNumberCorrect("a3-495-5"), "Phone number constructor 10");
    Assert(not IsPhoneNumberCorrect("+3"), "Phone number constructor 11");
    Assert(not IsPhoneNumberCorrect("+3--"), "Phone number constructor 11");
    Assert(not IsPhoneNumberCorrect("+3--12"), "Phone number constructor 11");

    Assert(IsPhoneNumberCorrect("+7-495-1"), "Phone number constructor 12");
    Assert(IsPhoneNumberCorrect("+7-495-134-33-22"), "Phone number constructor 13");
    Assert(IsPhoneNumberCorrect("+7-495-1343322"), "Phone number constructor 14");
    Assert(IsPhoneNumberCorrect("+7-cpp-course"), "Phone number constructor 15");
    Assert(IsPhoneNumberCorrect("+7-cpp-cou-rse"), "Phone number constructor 16");
    Assert(IsPhoneNumberCorrect("+7-1-2"), "Phone number constructor 17");
    Assert(IsPhoneNumberCorrect("+c-p-p"), "Phone number constructor 18");
    Assert(IsPhoneNumberCorrect("+coursemy-papa123-pap"), "Phone number constructor 12");
    Assert(IsPhoneNumberCorrect("+ - - "), "Phone number constructor 12");
}

void TestPhoneNumberGet(const string &number,
                        const string &country_code,
                        const string &city_code,
                        const string &local_number)
{
    PhoneNumber pn{number};
    AssertEqual(pn.GetCountryCode(), country_code, number);
    AssertEqual(pn.GetCityCode(), city_code, number);
    AssertEqual(pn.GetLocalNumber(), local_number, number);
    AssertEqual(pn.GetInternationalNumber(), number, number);
}

void TestPhoneNumberGets()
{
    TestPhoneNumberGet("+7-495-1", "7", "495", "1");
    TestPhoneNumberGet("+3-495-134-33-22", "3", "495", "134-33-22");
    TestPhoneNumberGet("+7-495-1343322", "7", "495", "1343322");
    TestPhoneNumberGet("+7-cpp-course", "7", "cpp", "course");
    TestPhoneNumberGet("+7-cpp-cou-rse", "7", "cpp", "cou-rse");
    TestPhoneNumberGet("+7-1-2", "7", "1", "2");
    TestPhoneNumberGet("+c-p-p", "c", "p", "p");
    TestPhoneNumberGet("+coursemy-papa123-pap", "coursemy", "papa123", "pap");
    TestPhoneNumberGet("+ - - ", " ", " ", " ");
}

void TestAll()
{
    TestRunner tr = {};
    tr.RunTest(TestPhoneNumberConstructor, "TestPhoneNumberConstructor");
    tr.RunTest(TestPhoneNumberGets, "TestPhoneNumberGets");
}