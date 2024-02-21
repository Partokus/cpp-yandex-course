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
#include <list>

using namespace std;

void TestAll();
void Profile();

class Editor
{
public:
    void Left()
    {
        if (_cursor != _text.begin())
        {
            --_cursor;
        }
    }

    void Right()
    {
        if (_cursor != _text.end())
        {
            ++_cursor;
        }
    }

    void Insert(char token)
    {
        _text.insert(_cursor, token);
    }

    void Cut(size_t tokens = 1)
    {
        Iterator it = CopyPrivate(tokens);
        _cursor = _text.erase(_cursor, it);
    }

    void Copy(size_t tokens = 1)
    {
        CopyPrivate(tokens);
    }

    void Paste()
    {
        _text.insert(_cursor, _copy_buf.cbegin(), _copy_buf.cend());
    }

    string GetText() const
    {
        return {_text.cbegin(), _text.cend()};
    }

private:
    using Iterator = list<char>::iterator;
    list<char> _text;
    Iterator _cursor = _text.end();
    string _copy_buf;

    Iterator CopyPrivate(size_t tokens = 1)
    {
        _copy_buf.resize(tokens);
        Iterator it = _cursor;
        for (size_t i = 0; (i < tokens) and (it != _text.end()); ++i)
        {
            _copy_buf[i] = *it;
            ++it;
        }
        return it;
    }
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

        ASSERT_EQUAL(editor.GetText(), "hello, world");

        for (int i = 0; i < 7; ++i)
        {
            editor.Left();
        }
        editor.Cut(7);
        ASSERT_EQUAL(editor.GetText(), "hello");
        editor.Paste();
        ASSERT_EQUAL(editor.GetText(), "hello, world");
        editor.Insert('!');
        ASSERT_EQUAL(editor.GetText(), "hello, world!");

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