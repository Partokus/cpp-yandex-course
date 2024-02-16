#include <profile.h>
#include <test_runner.h>
#include <iomanip>
#include <iostream>
#include <set>
#include <map>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <string_view>

using namespace std;

void TestAll();
void Profile();

class Translator
{
public:
    void Add(string_view source, string_view target)
    {
        const auto it_source = _words.insert(string{source}).first;
        const auto it_target = _words.insert(string{target}).first;
        _source[*it_source] = *it_target;
        _target[*it_target] = *it_source;
    }

    string_view TranslateForward(string_view source) const
    {
        if (const auto it = _source.find(source); it != _source.cend())
        {
            return it->second;
        }
        return {};
    }

    string_view TranslateBackward(string_view target) const
    {
        if (const auto it = _target.find(target); it != _target.cend())
        {
            return it->second;
        }
        return {};
    }

private:
    set<string> _words;
    map<string_view, string_view> _source;
    map<string_view, string_view> _target;
};

int main()
{
    TestAll();
    Profile();
    return 0;
}

void TestSimple()
{
    Translator translator;
    translator.Add(string("okno"), string("window"));
    translator.Add(string("stol"), string("table"));

    ASSERT_EQUAL(translator.TranslateForward("okno"), "window");
    ASSERT_EQUAL(translator.TranslateBackward("table"), "stol");
    ASSERT_EQUAL(translator.TranslateBackward("stol"), "");
}

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, TestSimple);
}

void Profile()
{
}