#include "test_runner.h"
#include "profile.h"

#include <numeric>
#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

using namespace std;

class Learner
{
private:
    set<string> dict{};

public:
    int Learn(const vector<string> &words)
    {
        const int prev_size = dict.size();
        dict.insert(words.cbegin(), words.cend());
        const int new_words_count = dict.size() - prev_size;
        return new_words_count;
    }

    vector<string> KnownWords()
    {
        return {dict.cbegin(), dict.cend()};
    }
};

int main()
{
    LOG_DURATION("Learner");
    Learner learner;
    string line;

    for (int i = 0; i < 28000; ++i)
    {
        line += to_string(i) + '\n';
    }

    vector<string> words;
    stringstream ss(line);
    string word;
    while (ss >> word)
    {
        words.push_back(word);
    }
    cout << learner.Learn(words) << "\n";

    cout << "=== known words ===\n";
    for (auto word : learner.KnownWords())
    {
        cout << word << "\n";
    }
    return 0;
}
