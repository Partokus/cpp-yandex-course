#include <profile.h>
#include <test_runner.h>

#include <cassert>
#include <algorithm>
#include <iterator>
#include <map>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <string>
#include <forward_list>
#include <iterator>
#include <deque>
#include <tuple>
#include <random>
#include <functional>
#include <fstream>
#include <random>
#include <tuple>

using namespace std;

void TestAll();
void Profile();

int main()
{
    TestAll();
    Profile();
    return 0;
}

void TestAll()
{
    TestRunner tr{};
}

void Profile()
{
}
