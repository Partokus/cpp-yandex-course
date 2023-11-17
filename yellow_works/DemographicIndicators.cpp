#include <test_runner.h>
#include <algorithm>
#include <numeric>
#include <cmath>

using namespace std;

void TestAll()
{
    TestRunner tr = {};
}

// enum class Gender
// {
//     FEMALE,
//     MALE
// };

// struct Person
// {
//     int age;
//     Gender gender;
//     bool is_employed;
// };

// // Это пример функции, его не нужно отправлять вместе с функцией PrintStats
// template <typename InputIt>
// int ComputeMedianAge(InputIt range_begin, InputIt range_end)
// {
//     if (range_begin == range_end)
//     {
//         return 0;
//     }
//     vector<typename InputIt::value_type> range_copy(range_begin, range_end);
//     auto middle = begin(range_copy) + range_copy.size() / 2;
//     nth_element(
//         begin(range_copy), middle, end(range_copy),
//         [](const Person &lhs, const Person &rhs)
//         {
//             return lhs.age < rhs.age;
//         });
//     return middle->age;
// }

void PrintStats(vector<Person> persons)
{
    cout << "Median age = " << ComputeMedianAge(begin(persons), end(persons)) << endl;

    auto it = partition(begin(persons), end(persons), [](const auto &person){
        return person.gender == Gender::FEMALE;
    });
    cout << "Median age for females = " << ComputeMedianAge(begin(persons), it) << endl;

    it = partition(begin(persons), end(persons), [](const auto &person){
        return person.gender == Gender::MALE;
    });
    cout << "Median age for males = " << ComputeMedianAge(begin(persons), it) << endl;

    it = partition(begin(persons), end(persons), [](const auto &person){
        return person.gender == Gender::FEMALE and person.is_employed;
    });
    cout << "Median age for employed females = " << ComputeMedianAge(begin(persons), it) << endl;

    it = partition(begin(persons), end(persons), [](const auto &person){
        return person.gender == Gender::FEMALE and not person.is_employed;
    });
    cout << "Median age for unemployed females = " << ComputeMedianAge(begin(persons), it) << endl;

    it = partition(begin(persons), end(persons), [](const auto &person){
        return person.gender == Gender::MALE and person.is_employed;
    });
    cout << "Median age for employed males = " << ComputeMedianAge(begin(persons), it) << endl;

    it = partition(begin(persons), end(persons), [](const auto &person){
        return person.gender == Gender::MALE and not person.is_employed;
    });
    cout << "Median age for unemployed males = " << ComputeMedianAge(begin(persons), it) << endl;
}

// int main()
// {
//     vector<Person> persons = {
//         {31, Gender::MALE, false},
//         {40, Gender::FEMALE, true},
//         {24, Gender::MALE, true},
//         {20, Gender::FEMALE, true},
//         {80, Gender::FEMALE, false},
//         {78, Gender::MALE, false},
//         {10, Gender::FEMALE, false},
//         {55, Gender::MALE, true},
//     };
//     PrintStats(persons);
//     return 0;
// }
