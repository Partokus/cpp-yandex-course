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

using namespace std;

struct Student
{
    string first_name;
    string last_name;
    int birth_day = 0;
    int birth_month = 0;
    int birth_year = 0;
};

int main()
{
    int students_number = 0;
    cin >> students_number;

    vector<Student> students(students_number);

    for (int i = 0; i < students_number; ++i)
    {
        cin >> students[i].first_name >> students[i].last_name >> students[i].birth_day >> students[i].birth_month >> students[i].birth_year;
    }

    int requests_number = 0;
    cin >> requests_number;

    for (int i = 0; i < requests_number; ++i)
    {
        string request;
        int student_number = 0;
        cin >> request >> student_number;
        --student_number;

        if (student_number >= students_number or student_number < 0)
        {
            cout << "bad request" << endl;
            continue;
        }

        if (request == "name")
        {
            cout << students.at(student_number).first_name << " " << students.at(student_number).last_name << endl;
        }
        else if (request == "date")
        {
            cout << students.at(student_number).birth_day << "." << students.at(student_number).birth_month << "." << students.at(student_number).birth_year << endl;
        }
        else
        {
            cout << "bad request" << endl;
        }
    }
    return 0;
}
