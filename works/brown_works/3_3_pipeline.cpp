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
#include <memory>

using namespace std;

void TestAll();
void Profile();

struct Email
{
    string from;
    string to;
    string body;
};

class Worker
{
public:
    virtual ~Worker() = default;
    virtual void Process(unique_ptr<Email> email) = 0;
    virtual void Run()
    {
        // только первому worker-у в пайплайне нужно это имплементировать
        throw logic_error("Unimplemented");
    }

protected:
    // реализации должны вызывать PassOn, чтобы передать объект дальше
    // по цепочке обработчиков
    void PassOn(unique_ptr<Email> email) const
    {
        if (_next)
        {
            _next->Process(move(email));
        }
    }

public:
    void SetNext(unique_ptr<Worker> next)
    {
        if (not _next)
        {
            _next = move(next);
        }
        else
        {
            _next->SetNext(move(next));
        }
    }

private:
    unique_ptr<Worker> _next;
};

class Reader : public Worker
{
public:
    Reader(istream &in)
        : _in(in)
    {
    }

    void Process(unique_ptr<Email>) override
    {
    }

    void Run() override
    {
        while (_in)
        {
            auto email = make_unique<Email>();

            if (not getline(_in, email->from))
            {
                continue;
            }

            getline(_in, email->to);
            getline(_in, email->body);

            PassOn(move(email));
        }
    }

private:
    istream &_in;
};

class Filter : public Worker
{
public:
    using Function = function<bool(const Email &)>;

    Filter(Function filter)
        : _filter(filter)
    {
    }

    void Process(unique_ptr<Email> email) override
    {
        if (_filter(*email))
        {
            PassOn(move(email));
        }
    }

private:
    Function _filter = nullptr;
};

class Copier : public Worker
{
public:
    Copier(string to)
        : _to(to)
    {
    }

    void Process(unique_ptr<Email> email) override
    {
        if (email->to == _to)
        {
            PassOn(move(email));
        }
        else
        {
            auto copy_email = make_unique<Email>(*email);
            copy_email->to = _to;
            PassOn(move(email));
            PassOn(move(copy_email));
        }
    }

private:
    string _to;
};

class Sender : public Worker
{
public:
    Sender(ostream &out)
        : _out(out)
    {
    }

    void Process(unique_ptr<Email> email) override
    {
        _out << email->from << '\n' <<
                email->to   << '\n' <<
                email->body << '\n';

        PassOn(move(email));
    }

private:
    ostream &_out;
};

class PipelineBuilder
{
public:
    // добавляет в качестве первого обработчика Reader
    explicit PipelineBuilder(istream &in)
        : _chain(make_unique<Reader>(in))
    {
    }

    // добавляет новый обработчик Filter
    PipelineBuilder &FilterBy(Filter::Function filter)
    {
        _chain->SetNext(make_unique<Filter>(filter));
        return *this;
    }

    // добавляет новый обработчик Copier
    PipelineBuilder &CopyTo(string recipient)
    {
        _chain->SetNext(make_unique<Copier>(recipient));
        return *this;
    }

    // добавляет новый обработчик Sender
    PipelineBuilder &Send(ostream &out)
    {
        _chain->SetNext(make_unique<Sender>(out));
        return *this;
    }

    // возвращает готовую цепочку обработчиков
    unique_ptr<Worker> Build()
    {
        return move(_chain);
    }

private:
    unique_ptr<Worker> _chain;
};

int main()
{
    TestAll();
    Profile();

    return 0;
}

void TestSanity()
{
    string input = ("erich@example.com\n"
                    "richard@example.com\n"
                    "Hello there\n"

                    "erich@example.com\n"
                    "ralph@example.com\n"
                    "Are you sure you pressed the right button?\n"

                    "ralph@example.com\n"
                    "erich@example.com\n"
                    "I do not make mistakes of that kind\n");
    istringstream inStream(input);
    ostringstream outStream;

    PipelineBuilder builder(inStream);
    builder.FilterBy([](const Email &email)
                     { return email.from == "erich@example.com"; });
    builder.CopyTo("richard@example.com");
    builder.Send(outStream);
    auto pipeline = builder.Build();

    pipeline->Run();

    string expectedOutput = ("erich@example.com\n"
                             "richard@example.com\n"
                             "Hello there\n"

                             "erich@example.com\n"
                             "ralph@example.com\n"
                             "Are you sure you pressed the right button?\n"

                             "erich@example.com\n"
                             "richard@example.com\n"
                             "Are you sure you pressed the right button?\n");

    string real_output = outStream.str();

    ASSERT_EQUAL(expectedOutput, real_output);
}

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, TestSanity);
}

void Profile()
{
}
