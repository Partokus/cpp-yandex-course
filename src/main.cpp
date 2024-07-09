#include <profile.h>
#include <test_runner.h>

#include <algorithm>
#include <iterator>
#include <map>
#include <vector>
#include <string>
#include <forward_list>
#include <iterator>

using namespace std;

void TestAll();
void Profile();

int main()
{
    TestAll();
    Profile();

    int countValueOne = 0;
    const int rangeValue = 100000;
    for (int i = 1; i < rangeValue; i++)
    {
        std::string strValue = to_string(i);
        for (int j = 0; j < strValue.length(); j++)
            if (strValue[j] == '1')
                countValueOne++;
    }

    std::cout << countValueOne << endl;

    return 0;
}

void TestAll()
{
    TestRunner tr{};
}

void Profile()
{
}
