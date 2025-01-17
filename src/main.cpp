#include <profile.h>
#include <test_runner.h>

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
#include <stdexcept>

using namespace std;

void TestAll();
void Profile();

enum class Gender {
  FEMALE,
  MALE
};

struct Person {
  int age;
  Gender gender;
  bool is_employed;
};

template <typename InputIt>
int ComputeMedianAge(InputIt range_begin, InputIt range_end) {
  if (range_begin == range_end) {
    return 0;
  }
  vector<typename iterator_traits<InputIt>::value_type> range_copy(
      range_begin,
      range_end
  );
  auto middle = begin(range_copy) + range_copy.size() / 2;
  nth_element(
      begin(range_copy), middle, end(range_copy),
      [](const Person& lhs, const Person& rhs) {
        return lhs.age < rhs.age;
      }
  );
  return middle->age;
}

int main() {
  int person_count;
  cin >> person_count;
  vector<Person> persons;
  persons.reserve(person_count);
  for (int i = 0; i < person_count; ++i) {
    int age, gender, is_employed;
    cin >> age >> gender >> is_employed;
    Person person{
        age,
        static_cast<Gender>(gender),
        is_employed == 1
    };
    persons.push_back(person);
  }

  auto females_end = partition(
      begin(persons), end(persons),
      [](const Person& p) {
        return p.gender == Gender::FEMALE;
      }
  );
  auto employed_females_end = partition(
      begin(persons), females_end,
      [](const Person& p) {
        return p.is_employed;
      }
  );
  auto employed_males_end = partition(
      females_end, end(persons),
      [](const Person& p) {
        return p.is_employed;
      }
  );

  cout << "Median age = "
       << ComputeMedianAge(begin(persons), end(persons))         << endl
       << "Median age for females = "
       << ComputeMedianAge(begin(persons), females_end)          << endl
       << "Median age for males = "
       << ComputeMedianAge(females_end, end(persons))            << endl
       << "Median age for employed females = "
       << ComputeMedianAge(begin(persons), employed_females_end) << endl
       << "Median age for unemployed females = "
       << ComputeMedianAge(employed_females_end, females_end)    << endl
       << "Median age for employed males = "
       << ComputeMedianAge(females_end, employed_males_end)      << endl
       << "Median age for unemployed males = "
       << ComputeMedianAge(employed_males_end, end(persons))     << endl;

  return 0;
}

const vector<Person> etalon_persons =
{
        {31, Gender::MALE, false},
        {40, Gender::FEMALE, true},
        {24, Gender::MALE, true},
        {20, Gender::FEMALE, true},
        {80, Gender::FEMALE, false},
        {78, Gender::MALE, false},
        {10, Gender::FEMALE, false},
        {55, Gender::MALE, true},
};

void TestComputeMedianAge()
{
    ASSERT_EQUAL(ComputeMedianAge(etalon_persons.begin(), etalon_persons.begin()), 0);
    ASSERT_EQUAL(ComputeMedianAge(etalon_persons.begin(), etalon_persons.end()), 40);
    ASSERT_EQUAL(ComputeMedianAge(etalon_persons.begin(), etalon_persons.begin() + 1), 31);
}

void TestReadPersons()
{
    istringstream iss(R"(
      8
      31 1 0
      40 0 1
      24 1 1
      20 0 1
      80 0 0
      78 1 0
      10 0 0
      55 1 1
    )");

    ASSERT_EQUAL(etalon_persons, ReadPersons(iss));
}

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, TestComputeMedianAge);
    RUN_TEST(tr, TestReadPersons);
}

void Profile()
{
}
