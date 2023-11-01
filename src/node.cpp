#include "node.h"

using namespace std;

DateComparisonNode::DateComparisonNode(const Comparison cmp, const Date &date)
    : DateVar(date)
{
    switch (cmp)
    {
    case Comparison::Less:
        _cmp = [this](const Date &date)
        {
            return date < DateVar;
        };
        break;
    case Comparison::LessOrEqual:
        _cmp = [this](const Date &date)
        {
            return date < DateVar or date == DateVar;
        };
        break;
    case Comparison::Greater:
        _cmp = [this](const Date &date)
        {
            return DateVar < date;
        };
        break;
    case Comparison::GreaterOrEqual:
        _cmp = [this](const Date &date)
        {
            return DateVar < date or DateVar == date;
        };
        break;
    case Comparison::Equal:
        _cmp = [this](const Date &date)
        {
            return date == DateVar;
        };
        break;
    case Comparison::NotEqual:
        _cmp = [this](const Date &date)
        {
            return date < DateVar or DateVar < date;
        };
        break;
    default:
        break;
    }
}

bool DateComparisonNode::Evaluate(const Date &date, const string &)
{
    return _cmp(date);
}

EventComparisonNode::EventComparisonNode(const Comparison cmp, const string &event)
    : Event(event)
{
    switch (cmp)
    {
    case Comparison::Less:
        _cmp = [this](const string &event)
        {
            return event < Event;
        };
        break;
    case Comparison::LessOrEqual:
        _cmp = [this](const string &event)
        {
            return event < Event or event == Event;
        };
        break;
    case Comparison::Greater:
        _cmp = [this](const string &event)
        {
            return event > Event;
        };
        break;
    case Comparison::GreaterOrEqual:
        _cmp = [this](const string &event)
        {
            return event > Event or event == Event;
        };
        break;
    case Comparison::Equal:
        _cmp = [this](const string &event)
        {
            return event == Event;
        };
        break;
    case Comparison::NotEqual:
        _cmp = [this](const string &event)
        {
            return event != Event;
        };
        break;
    default:
        break;
    }
}

bool EventComparisonNode::Evaluate(const Date &, const string &event)
{
    return _cmp(event);
}
