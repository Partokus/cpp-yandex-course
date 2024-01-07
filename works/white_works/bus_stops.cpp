#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <map>

using namespace std;

int main()
{
    map<string, vector<string>> bus_and_stops{};
    vector<string> bus_and_stops_map_keys{};

    int q{};
    cin >> q;

    for (int i = 0; i < q; ++i)
    {
        string request{};
        cin >> request;

        if (request == "NEW_BUS")
        {
            string bus{};
            cin >> bus;

            int stop_count{};
            cin >> stop_count;

            for (int k = 0; k < stop_count; ++k)
            {
                string stop{};
                cin >> stop;

                bus_and_stops[bus].push_back(stop);
            }

            bus_and_stops_map_keys.push_back(bus);
        }
        else if (request == "BUSES_FOR_STOP")
        {
            string stop{};
            cin >> stop;

            bool stop_found{};

            for (auto const &k : bus_and_stops_map_keys)
            {
                // проверяем, есть ли значение stop в векторе bus_and_stops[k]
                if (find(begin(bus_and_stops[k]), end(bus_and_stops[k]), stop) != end(bus_and_stops[k]))
                {
                    cout << k << " ";
                    stop_found = true;
                }
            }
            if (!stop_found)
            {
                cout << "No stop";
            }
            cout << endl;
        }
        else if (request == "STOPS_FOR_BUS")
        {
            string bus{};
            cin >> bus;

            if (bus_and_stops.count(bus))
            {
                for (auto const &k : bus_and_stops[bus])
                {
                    bool interchange_found{};

                    cout << "Stop " << k << ": ";

                    for (auto const &j : bus_and_stops_map_keys)
                    {
                        if (j == bus)
                        {
                            continue;
                        }

                        if (find(begin(bus_and_stops[j]), end(bus_and_stops[j]), k) != end(bus_and_stops[j]))
                        {
                            cout << j << " ";
                            interchange_found = true;
                        }
                    }
                    if (!interchange_found)
                    {
                        cout << "no interchange";
                    }
                    cout << endl;
                }
            }
            else
            {
                cout << "No bus" << endl;
            }
        }
        else if (request == "ALL_BUSES")
        {
            if (!bus_and_stops.empty())
            {
                for (auto const &[key, value] : bus_and_stops)
                {
                    cout << "Bus " << key << ": ";
                    for (auto const &v : value)
                    {
                        cout << v << " ";
                    }
                    cout << endl;
                }
            }
            else
            {
                cout << "No buses" << endl;
            }
        }
    }
    return 0;
}
