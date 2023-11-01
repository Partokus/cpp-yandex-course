#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <functional>

#include "date.h"

// Node, EmptyNode, DateComparisonNode, EventComparisonNode и LogicalOperationNode

enum class Comparison
{
    Less,
    LessOrEqual,
    Greater,
    GreaterOrEqual,
    Equal,
    NotEqual
};

class Node
{
public:
    virtual ~Node() = default;

    virtual bool Evaluate(const Date &date, const std::string &event) = 0;
};

class DateComparisonNode : public Node
{
public:
    DateComparisonNode(const Comparison cmp, const Date &date);

    bool Evaluate(const Date &date, const std::string &) override;

private:
    std::function<bool(const Date &)> _cmp{};
    const Date DateVar;
};

class EventComparisonNode : public Node
{
public:
    EventComparisonNode(const Comparison cmp, const std::string &event);

    bool Evaluate(const Date &, const std::string &event) override;

private:
    std::function<bool(const std::string &)> _cmp{};
    const std::string Event;
};