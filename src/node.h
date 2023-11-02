#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <functional>
#include <memory>

#include "date.h"

enum class Comparison
{
    Less,
    LessOrEqual,
    Greater,
    GreaterOrEqual,
    Equal,
    NotEqual
};

enum class LogicalOperation
{
    And,
    Or
};

class Node
{
public:
    virtual ~Node() = default;

    virtual bool Evaluate(const Date &date, const std::string &event) const = 0;
};

class DateComparisonNode : public Node
{
public:
    DateComparisonNode(const Comparison cmp, const Date &date);

    bool Evaluate(const Date &date, const std::string &) const override;

private:
    std::function<bool(const Date &)> _cmp{};
    const Date DateVar;
};

class EventComparisonNode : public Node
{
public:
    EventComparisonNode(const Comparison cmp, const std::string &event);

    bool Evaluate(const Date &, const std::string &event) const override;

private:
    std::function<bool(const std::string &)> _cmp{};
    const std::string Event;
};

class LogicalOperationNode : public Node
{
public:
    LogicalOperationNode(const LogicalOperation logical_operation, const std::shared_ptr<Node> left_node, const std::shared_ptr<Node> right_node);

    bool Evaluate(const Date &date, const std::string &event) const override;

private:
    const LogicalOperation LogicalOperationVar;
    const std::shared_ptr<Node> LeftNode;
    const std::shared_ptr<Node> RightNode;
};

class EmptyNode : public Node
{
public:
    bool Evaluate(const Date &, const std::string &) const override;
};