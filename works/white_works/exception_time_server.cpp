#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <optional>
#include <variant>
#include <cmath>
#include <numeric>

using namespace std;

string AskTimeServer()
{
    throw invalid_argument("invalid_argument");
    return "00:05:00";
}

class TimeServer
{
public:
    string GetCurrentTime()
    {
        try
        {
            last_fetched_time = AskTimeServer();
        }
        catch (const system_error &e)
        {
        }
        return last_fetched_time;
    }

private:
    string last_fetched_time = "00:00:00";
};

int main()
{
    // Меняя реализацию функции AskTimeServer, убедитесь, что это код работает корректно
    TimeServer ts;
    try
    {
        cout << ts.GetCurrentTime() << endl;
    }
    catch (exception &e)
    {
        cout << "Exception got: " << e.what() << endl;
    }
    return 0;
}
