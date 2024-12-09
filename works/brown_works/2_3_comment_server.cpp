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

using namespace std;

void TestAll();
void Profile();

struct HttpHeader
{
    string name;
    string value;
};

enum class HttpCode
{
    Ok = 200,
    NotFound = 404,
    Found = 302
};

class HttpResponse
{
public:
    explicit HttpResponse(HttpCode code);

    HttpResponse &AddHeader(string name, string value);
    HttpResponse &SetContent(string a_content);
    HttpResponse &SetCode(HttpCode a_code);

    friend ostream &operator<<(ostream &output, const HttpResponse &resp);

private:
    HttpCode _code;
    vector<HttpHeader> _headers;
    string _content;
};

HttpResponse::HttpResponse(HttpCode code)
    : _code(code)
{
}

HttpResponse & HttpResponse::AddHeader(string name, string value)
{
    _headers.push_back({name, value});
    return *this;
}

HttpResponse & HttpResponse::SetContent(string a_content)
{
    _content = a_content;
    return *this;
}

HttpResponse & HttpResponse::SetCode(HttpCode a_code)
{
    _code = a_code;
    return *this;
}

ostream & operator<<(ostream &output, const HttpResponse &resp)
{
    string str_code;
    switch (resp._code)
    {
        case HttpCode::Ok:       str_code = "OK";        break;
        case HttpCode::Found:    str_code = "Found";     break;
        case HttpCode::NotFound: str_code = "Not found"; break;
    }

    output << "HTTP/1.1 " << static_cast<int>(resp._code) << " " <<
                             str_code << '\n';

    for (const auto &header : resp._headers)
    {
        output << header.name << ": " << header.value << '\n';
    }

    if (not resp._content.empty())
        output << "Content-Length: " << resp._content.size() << '\n';

    output << '\n';

    if (not resp._content.empty())
        output << resp._content;

    return output;
}

struct HttpRequest
{
    string method, path, body;
    map<string, string> get_params;
};

pair<string, string> SplitBy(const string &what, const string &by)
{
    size_t pos = what.find(by);
    if (by.size() < what.size() && pos < what.size() - by.size())
    {
        return {what.substr(0, pos), what.substr(pos + by.size())};
    }
    else
    {
        return {what, {}};
    }
}

template <typename T>
T FromString(const string &s)
{
    T x;
    istringstream is(s);
    is >> x;
    return x;
}

pair<size_t, string> ParseIdAndContent(const string &body)
{
    auto [id_string, content] = SplitBy(body, " ");
    return {FromString<size_t>(id_string), content};
}

struct LastCommentInfo
{
    size_t user_id, consecutive_count;
};

class CommentServer
{
private:
    vector<vector<string>> _comments;
    std::optional<LastCommentInfo> _last_comment;
    unordered_set<size_t> _banned_users;

public:
    void ServeRequest(const HttpRequest &req, ostream &os)
    {
        HttpResponse resp = ServeRequest(req);
        os << resp;
    }

    HttpResponse ServeRequest(const HttpRequest &req)
    {
        HttpResponse resp(HttpCode::NotFound);

        if (req.method == "POST")
        {
            if (req.path == "/add_user")
            {
                _comments.emplace_back();
                const string user_id = to_string(_comments.size() - 1);
                resp.SetCode(HttpCode::Ok).SetContent(user_id);
            }
            else if (req.path == "/add_comment")
            {
                auto [user_id, comment] = ParseIdAndContent(req.body);

                if (!_last_comment || _last_comment->user_id != user_id)
                {
                    _last_comment = LastCommentInfo{user_id, 1};
                }
                else if (++_last_comment->consecutive_count > 3)
                {
                    _banned_users.insert(user_id);
                }

                if (_banned_users.count(user_id) == 0)
                {
                    _comments[user_id].push_back(string(comment));
                    resp.SetCode(HttpCode::Ok);
                }
                else
                {
                    resp.SetCode(HttpCode::Found).AddHeader("Location", "/captcha");
                }
            }
            else if (req.path == "/checkcaptcha")
            {
                if (auto [id, response] = ParseIdAndContent(req.body); response == "42")
                {
                    _banned_users.erase(id);
                    if (_last_comment && _last_comment->user_id == id)
                    {
                        _last_comment.reset();
                    }
                    resp.SetCode(HttpCode::Ok);
                }
                else
                {
                    resp.SetCode(HttpCode::Found).AddHeader("Location", "/captcha");
                }
            }
        }
        else if (req.method == "GET")
        {
            if (req.path == "/user_comments")
            {
                const size_t user_id = FromString<size_t>(req.get_params.at("user_id"));
                string response;
                for (const string &c : _comments[user_id])
                {
                    response += c + '\n';
                }

                resp.SetCode(HttpCode::Ok).SetContent(response);
            }
            else if (req.path == "/captcha")
            {
                resp.SetCode(HttpCode::Ok).SetContent(
                    "What's the answer for The Ultimate Question of Life, the Universe, and Everything?"
                );
            }
        }
        return resp;
    }
};

ostream &operator<<(ostream &output, const HttpHeader &h)
{
    return output << h.name << ": " << h.value;
}

bool operator==(const HttpHeader &lhs, const HttpHeader &rhs)
{
    return lhs.name == rhs.name && lhs.value == rhs.value;
}

struct ParsedResponse
{
    int code;
    vector<HttpHeader> headers;
    string content;
};

istream &operator>>(istream &input, ParsedResponse &r)
{
    string line;
    getline(input, line);

    {
        istringstream code_input(line);
        string dummy;
        code_input >> dummy >> r.code;
    }

    size_t content_length = 0;

    r.headers.clear();
    while (getline(input, line) && !line.empty())
    {
        if (auto [name, value] = SplitBy(line, ": "); name == "Content-Length")
        {
            istringstream length_input(value);
            length_input >> content_length;
        }
        else
        {
            r.headers.push_back({std::move(name), std::move(value)});
        }
    }

    r.content.resize(content_length);
    input.read(r.content.data(), r.content.size());
    return input;
}

int main()
{
    TestAll();
    Profile();
    return 0;
}

void Test(CommentServer &srv, const HttpRequest &request, const ParsedResponse &expected)
{
    stringstream ss;
    srv.ServeRequest(request, ss);
    ParsedResponse resp;
    ss >> resp;
    ASSERT_EQUAL(resp.code, expected.code);
    ASSERT_EQUAL(resp.headers, expected.headers);
    ASSERT_EQUAL(resp.content, expected.content);
}

template <typename CommentServer>
void TestServer()
{
    CommentServer cs;

    const ParsedResponse ok{200};
    const ParsedResponse redirect_to_captcha{302, {{"Location", "/captcha"}}, {}};
    const ParsedResponse not_found{404};

    Test(cs, {"POST", "/add_user"}, {200, {}, "0"});
    Test(cs, {"POST", "/add_user"}, {200, {}, "1"});
    Test(cs, {"POST", "/add_comment", "0 Hello"}, ok);
    Test(cs, {"POST", "/add_comment", "1 Hi"}, ok);
    Test(cs, {"POST", "/add_comment", "1 Buy my goods"}, ok);
    Test(cs, {"POST", "/add_comment", "1 Enlarge"}, ok);
    Test(cs, {"POST", "/add_comment", "1 Buy my goods"}, redirect_to_captcha);
    Test(cs, {"POST", "/add_comment", "0 What are you selling?"}, ok);
    Test(cs, {"POST", "/add_comment", "1 Buy my goods"}, redirect_to_captcha);
    Test(
        cs,
        {"GET", "/user_comments", "", {{"user_id", "0"}}},
        {200, {}, "Hello\nWhat are you selling?\n"});
    Test(
        cs,
        {"GET", "/user_comments", "", {{"user_id", "1"}}},
        {200, {}, "Hi\nBuy my goods\nEnlarge\n"});
    Test(
        cs,
        {"GET", "/captcha"},
        {200, {}, {"What's the answer for The Ultimate Question of Life, the Universe, and Everything?"}});
    Test(cs, {"POST", "/checkcaptcha", "1 24"}, redirect_to_captcha);
    Test(cs, {"POST", "/checkcaptcha", "1 42"}, ok);
    Test(cs, {"POST", "/add_comment", "1 Sorry! No spam any more"}, ok);
    Test(
        cs,
        {"GET", "/user_comments", "", {{"user_id", "1"}}},
        {200, {}, "Hi\nBuy my goods\nEnlarge\nSorry! No spam any more\n"});

    Test(cs, {"GET", "/user_commntes"}, not_found);
    Test(cs, {"POST", "/add_uesr"}, not_found);
}

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, TestServer<CommentServer>);
}

void Profile()
{
}
