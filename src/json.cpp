#include "json.h"

using namespace std;

namespace Json
{

Document::Document(Node root) : root(move(root))
{
}

const Node &Document::GetRoot() const
{
    return root;
}

Node LoadNode(istream &input);

Node LoadArray(istream &input)
{
    vector<Node> result;

    for (char c; input >> c && c != ']';)
    {
        if (c != ',')
        {
            input.putback(c);
        }
        result.push_back(LoadNode(input));
    }

    return Node(move(result));
}

Node LoadDouble(istream &input)
{
    string s;
    while (isdigit(input.peek()) or input.peek() == '.')
    {
        s.push_back(input.get());
    }
    return Node(stod(s));
}

Node LoadBool(istream &input)
{
    string s;
    if (input.peek() == 't')
    {
        input.ignore(4); // true
        return Node(true);
    }

    input.ignore(5); // false
    return Node(false);
}

Node LoadString(istream &input)
{
    string line;
    getline(input, line, '"');
    return Node(move(line));
}

Node LoadDict(istream &input)
{
    map<string, Node> result;

    for (char c; input >> c && c != '}';)
    {
        if (c == ',')
        {
            input >> c;
        }

        string key = LoadString(input).AsString();
        input >> c;
        result.emplace(move(key), LoadNode(input));
    }

    return Node(move(result));
}

Node LoadNode(istream &input)
{
    char c;
    input >> c;

    if (c == '[')
    {
        return LoadArray(input);
    }
    else if (c == '{')
    {
        return LoadDict(input);
    }
    else if (c == '"')
    {
        return LoadString(input);
    }
    else if (c == 't' or c == 'f')
    {
        input.putback(c);
        return LoadBool(input);
    }
    else
    {
        input.putback(c);
        return LoadDouble(input);
    }
}

Document Load(istream &input)
{
    return Document{LoadNode(input)};
}

} // namespace Json