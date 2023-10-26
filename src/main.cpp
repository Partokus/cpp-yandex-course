#include <test_runner.h>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

class Person
{
public:
    Person(const string &name, const string &profession)
        : Name(name),
          Profession(profession)
    {
    }
    virtual ~Person() = default;

    virtual void Walk(const string &destination) const
    {
        cout << Profession << " " << Name << " walks to " << destination << endl;
    }

    const string Name;
    const string Profession;
};

class Student : public Person
{
public:
    Student(const string &name, const string &favouriteSong)
        : Person(name, "Student"),
          FavouriteSong(favouriteSong)
    {
    }

    void Walk(const string &destination) const override
    {
        cout << Profession << " " << Name << " walks to " << destination << endl;
        SingSong();
    }

    void Learn() const
    {
        cout << Profession << " " << Name << " learns" << endl;
    }

    void SingSong() const
    {
        cout << Profession << " " << Name << " sings song " << FavouriteSong << endl;
    }

private:
    const string FavouriteSong;
};

class Teacher : public Person
{
public:
    Teacher(const string &name, const string &subject)
        : Person(name, "Teacher"),
          Subject(subject)
    {
    }

    void Teach() const
    {
        cout << Profession << " " << Name << " teaches " << Subject << endl;
    }

private:
    const string Subject;
};

class Policeman : public Person
{
public:
    Policeman(const string &name)
        : Person(name, "Policeman")
    {
    }

    void Check(const Person &p) const
    {
        cout << Profession << " " << Name << " checks " << p.Profession << ". " << p.Profession << "'s name is " << p.Name << endl;
    }
};

void VisitPlaces(const Person &person, const vector<string> &places)
{
    for (const auto &place : places)
    {
        person.Walk(place);
    }
}

int main()
{
    Teacher t("Basta", "Rap");
    Student s("Guf", "My game");
    Policeman p("Ptaha");

    VisitPlaces(t, {"Moscow", "Kiev"});
    p.Check(s);
    VisitPlaces(s, {"Moscow", "Kiev"});

    t.Teach();
    s.Learn();
    return 0;
}
