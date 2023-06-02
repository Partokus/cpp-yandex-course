#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <optional>

using namespace std;

class ReversibleString
{
public:
    ReversibleString() {}
    ReversibleString(const string &new_s) noexcept
    {
        _s = new_s;
    }

    void Reverse() noexcept
    {
        reverse(_s.begin(), _s.end());
    }

    string ToString() const noexcept
    {
        return _s;
    }

private:
    string _s;
};

int main()
{
    ReversibleString s("live");
    s.Reverse();
    cout << s.ToString() << endl;

    s.Reverse();
    const ReversibleString &s_ref = s;
    string tmp = s_ref.ToString();
    cout << tmp << endl;

    ReversibleString empty;
    cout << '"' << empty.ToString() << '"' << endl;
    return 0;
}
