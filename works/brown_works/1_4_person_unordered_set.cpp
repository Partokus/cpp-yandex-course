#include <profile.h>
#include <test_runner.h>

#include <cassert>
#include <algorithm>
#include <iterator>
#include <map>
#include <unordered_set>
#include <vector>
#include <string>
#include <forward_list>
#include <iterator>
#include <deque>
#include <tuple>
#include <random>

using namespace std;

void TestAll();
void Profile();

using namespace std;

struct Address
{
    string city;
    string street;
    int building = 0;

    bool operator==(const Address &other) const
    {
        return tie(city, street, building) == tie(other.city, other.street, other.building);
    }
};

struct Person
{
    string name;
    int height = 0;
    double weight = 0.0;
    Address address{};

    bool operator==(const Person &other) const
    {
        return tie(name, height, weight, address) == tie(other.name, other.height, other.weight, other.address);
    }
};

struct AddressHasher
{
    size_t operator()(const Address &p) const
    {
        const size_t res1 = str_hasher(p.city);
        const size_t res2 = str_hasher(p.street);
        const size_t res3 = int_hasher(p.building);

        // Ax^2 + Bx + C
        static constexpr size_t Coef = 514'229U;

        return res1 * Coef * Coef + res2 * Coef + res3;
    }

    // std::hash обычно не имеет состояния, поэтому создание
    // локальных переменных не сопряжено с накладными расходами,
    // а наоборот, помогает компилятору лучше оптимизировать код
    hash<string> str_hasher;
    hash<int> int_hasher;
};

struct PersonHasher
{
    size_t operator()(const Person &p) const
    {
        const size_t res1 = str_hasher(p.name);
        const size_t res2 = int_hasher(p.height);
        const size_t res3 = double_hasher(p.weight);
        const size_t res4 = address_hasher(p.address);

        // Ax^3 + Bx^2 + Cx + D
        static constexpr size_t Coef = 39'916'801U;

        return res1 * Coef * Coef * Coef + res2 * Coef * Coef + res3 * Coef + res4;
    }

    hash<string> str_hasher;
    hash<int> int_hasher;
    hash<double> double_hasher;
    AddressHasher address_hasher;
};

int main()
{
    TestAll();
    Profile();
    return 0;
}

// сгенерированы командой:
// $ sort -R /usr/share/dict/propernames | head -n 100
//
// http://www.freebsd.org/cgi/cvsweb.cgi/~checkout~/src/share/dict/propernames
const vector<string> WORDS = {
    "Kieran", "Jong", "Jisheng", "Vickie", "Adam", "Simon", "Lance",
    "Everett", "Bryan", "Timothy", "Daren", "Emmett", "Edwin", "List",
    "Sharon", "Trying", "Dan", "Saad", "Kamiya", "Nikolai", "Del",
    "Casper", "Arthur", "Mac", "Rajesh", "Belinda", "Robin", "Lenora",
    "Carisa", "Penny", "Sabrina", "Ofer", "Suzanne", "Pria", "Magnus",
    "Ralph", "Cathrin", "Phill", "Alex", "Reinhard", "Marsh", "Tandy",
    "Mongo", "Matthieu", "Sundaresan", "Piotr", "Ramneek", "Lynne", "Erwin",
    "Edgar", "Srikanth", "Kimberly", "Jingbai", "Lui", "Jussi", "Wilmer",
    "Stuart", "Grant", "Hotta", "Stan", "Samir", "Ramadoss", "Narendra",
    "Gill", "Jeff", "Raul", "Ken", "Rahul", "Max", "Agatha",
    "Elizabeth", "Tai", "Ellen", "Matt", "Ian", "Toerless", "Naomi",
    "Rodent", "Terrance", "Ethan", "Florian", "Rik", "Stanislaw", "Mott",
    "Charlie", "Marguerite", "Hitoshi", "Panacea", "Dieter", "Randell", "Earle",
    "Rajiv", "Ted", "Mann", "Bobbie", "Pat", "Olivier", "Harmon",
    "Raman", "Justin"};

void TestSmoke()
{
    vector<Person> points = {
        {"John", 180, 82.5, {"London", "Baker St", 221}},
        {"Sherlock", 190, 75.3, {"London", "Baker St", 221}},
    };

    unordered_set<Person, PersonHasher> point_set;
    for (const auto &point : points)
    {
        point_set.insert(point);
    }

    ASSERT_EQUAL(points.size(), point_set.size());
    for (const auto &point : points)
    {
        ASSERT_EQUAL(point_set.count(point), static_cast<size_t>(1));
    }
}

void TestPurity()
{
    Person person = {"John", 180, 82.5, {"London", "Baker St", 221}};
    PersonHasher hasher;

    auto hash = hasher(person);
    for (size_t t = 0; t < 100; ++t)
    {
        ASSERT_EQUAL(hasher(person), hash);
    }
};

void TestDistribution()
{
    auto seed = 42;
    mt19937 gen(seed);

    uniform_int_distribution<int> height_dist(150, 200);
    uniform_int_distribution<int> weight_dist(100, 240); // [50, 120]
    uniform_int_distribution<int> building_dist(1, 300);
    uniform_int_distribution<int> word_dist(0, WORDS.size() - 1);

    PersonHasher hasher;

    // выбираем число бакетов не очень большим простым числом
    // (unordered_set, unordered_map используют простые числа бакетов
    // в реализациях GCC и Clang, для непростых чисел бакетов
    // возникают сложности со стандартной хеш-функцией в этих реализациях)
    const size_t num_buckets = 2053;

    // мы хотим, чтобы число точек в бакетах было ~100'000
    const size_t perfect_bucket_size = 50;
    const size_t num_points = num_buckets * perfect_bucket_size;
    vector<size_t> buckets(num_buckets);
    for (size_t t = 0; t < num_points; ++t)
    {
        Person person;
        person.name = WORDS[word_dist(gen)];
        person.height = height_dist(gen);
        person.weight = weight_dist(gen) * 0.5;
        person.address.city = WORDS[word_dist(gen)];
        person.address.street = WORDS[word_dist(gen)];
        person.address.building = building_dist(gen);
        ++buckets[hasher(person) % num_buckets];
    }

    // Статистика Пирсона:
    // https://en.wikipedia.org/wiki/Pearson's_chi-squared_test
    //
    // Численной мерой равномерности распределения также может выступать
    // энтропия, но для ее порогов нет хорошей статистической интерпретации
    double pearson_stat = 0;
    for (auto bucket_size : buckets)
    {
        size_t size_diff = bucket_size - perfect_bucket_size;
        pearson_stat +=
            size_diff * size_diff / static_cast<double>(perfect_bucket_size);
    }

    // проверяем равномерность распределения методом согласия Пирсона
    // со статистической значимостью 0.95:
    //
    // если подставить вместо ++buckets[hasher(person) % num_buckets]
    // выражение ++buckets[gen() % num_buckets], то с вероятностью 95%
    // ASSERT ниже отработает успешно,
    //
    // т.к. статистика Пирсона приблизительно распределена по chi^2
    // с числом степеней свободы, равным num_buckets - 1,
    // и 95 процентиль этого распределения равен:
    // >>> scipy.stats.chi2.ppf(0.95, 2052)
    const double critical_value = 2158.4981036918693;
    ASSERT(pearson_stat < critical_value);
}

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, TestSmoke);
    RUN_TEST(tr, TestPurity);
    RUN_TEST(tr, TestDistribution);
}

void Profile()
{
}
