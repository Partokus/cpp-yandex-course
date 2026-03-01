#pragma once
#include <istream>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace Json
{

class Node : std::variant<std::vector<Node>,
                          std::map<std::string, Node>,
                          double,
                          bool,
                          std::string>
{
public:
    using variant::variant;

    const std::vector<Node> & AsArray() const
    {
        return std::get<std::vector<Node>>(*this);
    }

    const std::map<std::string, Node> & AsMap() const
    {
        return std::get<std::map<std::string, Node>>(*this);
    }

    int AsInt() const
    {
        return std::get<double>(*this);
    }

    double AsDouble() const
    {
        return std::get<double>(*this);
    }

    double AsBool() const
    {
        return std::get<bool>(*this);
    }

    const std::string & AsString() const
    {
        return std::get<std::string>(*this);
    }
};

class Document
{
public:
    explicit Document(Node root);

    const Node &GetRoot() const;

private:
    Node root;
};

Document Load(std::istream & input);

}