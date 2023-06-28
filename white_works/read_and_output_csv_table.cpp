#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <optional>

using namespace std;

int main()
{
    ifstream input("input.txt");

    if (input)
    {
        int N = 0;
        int M = 0;

        input >> N >> M;

        // переходим к строке
        for (int i = 0; i < N; ++i)
        {
            if (i != 0)
            {
                cout << endl;
            }
            // выводим строку
            for (int k = 0; k < M; ++k)
            {
                if (k != 0)
                {
                    cout << " ";
                }

                int number = 0;
                input >> number;
                cout << setw(10) << number;
                input.ignore(1);
            }
        }
    }
    return 0;
}
