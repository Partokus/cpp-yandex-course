#include <test_runner.h>
#include <algorithm>

using namespace std;

vector<string> SplitIntoWords(const string& s)
{
    auto it_first_word_symbol = s.begin();
    auto it_space = find(it_first_word_symbol, s.end(), ' ');
    vector<string> result = {string{it_first_word_symbol, it_space}};

    while (it_space != s.end())
    {
        it_first_word_symbol = it_space + 1;
        it_space = find(it_first_word_symbol, s.end(), ' ');
        result.push_back(string{it_first_word_symbol, it_space});
    }
    return result;
}

void TestSplitIntoWords()
{
    AssertEqual(SplitIntoWords("makaka banan dengi lave"), vector<string>{"makaka", "banan", "dengi", "lave"});
    AssertEqual(SplitIntoWords("bogatstvo clubi resorani buhlo"), vector<string>{"bogatstvo", "clubi", "resorani", "buhlo"});
    AssertEqual(SplitIntoWords("bogatstvo"), vector<string>{"bogatstvo"});
    AssertEqual(SplitIntoWords("b"), vector<string>{"b"});
    AssertEqual(SplitIntoWords("b a c"), vector<string>{"b", "a", "c"});
}

void TestAll()
{
    TestRunner tr = {};
    tr.RunTest(TestSplitIntoWords, "TestSplitIntoWords");
}

// int main()
// {
//     TestAll();

//     string s = "C Cpp Java Python";

//     vector<string> words = SplitIntoWords(s);
//     cout << words.size() << " ";
//     for (auto it = begin(words); it != end(words); ++it)
//     {
//         if (it != begin(words))
//         {
//             cout << "/";
//         }
//         cout << *it;
//     }
//     cout << endl;
//     return 0;
// }