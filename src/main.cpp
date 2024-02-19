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
#include <random>

using namespace std;

void TestAll();
void Profile();

class Editor
{
public:
    Editor()
    {
        _text.reserve(MaxSymbolCount);
        _copy_buf.reserve(MaxSymbolCount);
    }

    void Left()
    {
        if (_cursor != 0U)
        {
            --_cursor;
        }
    }

    void Right()
    {
        if (_cursor != _text.size())
        {
            ++_cursor;
        }
    }

    void Insert(char token)
    {
        _text.insert(_cursor, &token, 1);
        ++_cursor;
    }

    void Cut(size_t tokens = 1)
    {
        Copy(tokens);
        _text.erase(_cursor, tokens);
    }

    void Copy(size_t tokens = 1)
    {
        _copy_buf = _text.substr(_cursor, tokens);
    }

    void Paste()
    {
        _text.insert(_cursor, _copy_buf);
        _cursor += _copy_buf.size();
    }

    string GetText() const
    {
        return _text;
    }

private:
    size_t _cursor = 0U;
    string _text;
    string _copy_buf;

    static constexpr size_t MaxSymbolCount = 10'000'000U;
};

int main()
{
    TestAll();
    Profile();
    return 0;
}

void TypeText(Editor &editor, const string &text)
{
    for (char c : text)
    {
        editor.Insert(c);
    }
}

void TestEditing()
{
    {
        Editor editor{};

        string hello = "hello, world";

        for (const auto &i : hello)
        {
            editor.Insert(i);
        }

        editor.Copy(50);
        editor.Paste();

        ASSERT_EQUAL(editor.GetText(), "hello, world");
    }
    {
        Editor editor;

        const size_t text_len = 12;
        const size_t first_part_len = 7;
        TypeText(editor, "hello, world");
        for (size_t i = 0; i < text_len; ++i)
        {
            editor.Left();
        }
        editor.Cut(first_part_len);
        for (size_t i = 0; i < text_len - first_part_len; ++i)
        {
            editor.Right();
        }
        TypeText(editor, ", ");
        editor.Paste();
        editor.Left();
        editor.Left();
        editor.Cut(3);

        ASSERT_EQUAL(editor.GetText(), "world, hello");
    }
    {
        Editor editor;

        TypeText(editor, "misprnit");
        editor.Left();
        editor.Left();
        editor.Left();
        editor.Cut(1);
        editor.Right();
        editor.Paste();

        ASSERT_EQUAL(editor.GetText(), "misprint");
    }
}

void TestReverse()
{
    Editor editor;

    const string text = "esreveR";
    for (char c : text)
    {
        editor.Insert(c);
        editor.Left();
    }

    ASSERT_EQUAL(editor.GetText(), "Reverse");
}

void TestNoText()
{
    Editor editor;
    ASSERT_EQUAL(editor.GetText(), "");

    editor.Left();
    editor.Left();
    editor.Right();
    editor.Right();
    editor.Copy(0);
    editor.Cut(0);
    editor.Paste();

    ASSERT_EQUAL(editor.GetText(), "");
}

void TestEmptyBuffer()
{
    Editor editor;

    editor.Paste();
    TypeText(editor, "example");
    editor.Left();
    editor.Left();
    editor.Paste();
    editor.Right();
    editor.Paste();
    editor.Copy(0);
    editor.Paste();
    editor.Left();
    editor.Cut(0);
    editor.Paste();

    ASSERT_EQUAL(editor.GetText(), "example");
}

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, TestEditing);
    RUN_TEST(tr, TestReverse);
    RUN_TEST(tr, TestNoText);
    RUN_TEST(tr, TestEmptyBuffer);
}

void Profile()
{
}