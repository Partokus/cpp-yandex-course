#include <profile.h>
#include <test_runner.h>

#include "animals.h"

#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <utility>
#include <map>
#include <optional>
#include <unordered_set>
#include <algorithm>
#include <numeric>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <cmath>
#include <random>
#include <memory>

using namespace std;

void TestAll();
void Profile();

using Zoo = vector<unique_ptr<Animal>>;

// Эта функция получает на вход поток ввода и читает из него описание зверей.
// Если очередное слово этого текста - Tiger, Wolf или Fox, функция должна поместить соответствующего зверя в зоопарк.
// В противном случае она должна прекратить чтение и сгенерировать исключение runtime_error.
Zoo CreateZoo(istream &in)
{
    Zoo zoo;
    string word;
    while (in >> word)
    {
        if (word == "Tiger")
        {
            zoo.push_back(make_unique<Tiger>());
        }
        else if (word == "Wolf")
        {
            zoo.push_back(make_unique<Wolf>());
        }
        else if (word == "Fox")
        {
            zoo.push_back(make_unique<Fox>());
        }
        else
        {
            throw runtime_error("Unknown animal!");
        }
    }
    return zoo;
}

// Эта функция должна перебрать всех зверей в зоопарке в порядке их создания
// и записать в выходной поток для каждого из них результат работы виртуальной функции voice.
// Разделяйте голоса разных зверей символом перевода строки '\n'.
void Process(const Zoo &zoo, ostream &out)
{
    for (const auto &animal : zoo)
    {
        out << animal->Voice() << "\n";
    }
}

int main()
{
    TestAll();
    Profile();

    return 0;
}

void TestZoo()
{
    istringstream input("Tiger Wolf Fox Tiger");
    ostringstream output;
    Process(CreateZoo(input), output);

    const string expected =
        "Rrrr\n"
        "Wooo\n"
        "Tyaf\n"
        "Rrrr\n";

    ASSERT_EQUAL(output.str(), expected);
}

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, TestZoo);
}

void Profile()
{
}
