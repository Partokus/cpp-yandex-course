#include <profile.h>
#include <test_runner.h>

#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
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
#include <stdexcept>
#include <ctime>

using namespace std;

void TestAll();
void Profile();

template <typename It>
class Range
{
public:
    Range(It begin, It end) : begin_(begin), end_(end) {}
    It begin() const { return begin_; }
    It end() const { return end_; }

private:
    It begin_;
    It end_;
};

pair<string_view, optional<string_view>> SplitTwoStrict(string_view s, string_view delimiter = " ")
{
    const size_t pos = s.find(delimiter);
    if (pos == s.npos)
    {
        return {s, nullopt};
    }
    else
    {
        return {s.substr(0, pos), s.substr(pos + delimiter.length())};
    }
}

pair<string_view, string_view> SplitTwo(string_view s, string_view delimiter = " ")
{
    const auto [lhs, rhs_opt] = SplitTwoStrict(s, delimiter);
    return {lhs, rhs_opt.value_or("")};
}

string_view ReadToken(string_view &s, string_view delimiter = " ")
{
    const auto [lhs, rhs] = SplitTwo(s, delimiter);
    s = rhs;
    return lhs;
}

int ConvertToInt(string_view str)
{
    // use std::from_chars when available to git rid of string copy
    size_t pos;
    const int result = stoi(string(str), &pos);
    if (pos != str.length())
    {
        std::stringstream error;
        error << "string " << str << " contains " << (str.length() - pos) << " trailing chars";
        throw invalid_argument(error.str());
    }
    return result;
}

template <typename Number>
void ValidateBounds(Number number_to_check, Number min_value, Number max_value)
{
    if (number_to_check < min_value || number_to_check > max_value)
    {
        std::stringstream error;
        error << number_to_check << " is out of [" << min_value << ", " << max_value << "]";
        throw out_of_range(error.str());
    }
}

struct IndexSegment
{
    size_t left;
    size_t right;

    size_t length() const
    {
        return right - left;
    }
    bool empty() const
    {
        return length() == 0;
    }

    bool Contains(IndexSegment other) const
    {
        return left <= other.left && other.right <= right;
    }
};

IndexSegment IntersectSegments(IndexSegment lhs, IndexSegment rhs)
{
    const size_t left = max(lhs.left, rhs.left);
    const size_t right = min(lhs.right, rhs.right);
    return {left, max(left, right)};
}

bool AreSegmentsIntersected(IndexSegment lhs, IndexSegment rhs)
{
    return !(lhs.right <= rhs.left || rhs.right <= lhs.left);
}

struct MoneyData
{
    double add = 0.0;
    double spend = 0.0;

    MoneyData operator+(const MoneyData &o) const
    { return {add + o.add, spend + o.spend}; }
};

struct BulkMoneyAdder
{
    double delta = 0.0;
};

struct BulkMoneySpender
{
    double delta = 0.0;
};

struct BulkTaxApplier
{
    double percentage = 0;

    double ComputeFactor() const
    {
        const double factor = 1.0 - percentage / 100.0;
        return factor;
    }
};

// return percentage
double Combine(double lhs_percentage, double rhs_percentage)
{
    const double lhs_factor = 1.0 - lhs_percentage / 100.0;
    const double rhs_factor = 1.0 - rhs_percentage / 100.0;
    return 100.0 * (1.0 - lhs_factor * rhs_factor);
}

class BulkLinearUpdater
{
public:
    BulkLinearUpdater() = default;

    BulkLinearUpdater(const BulkMoneyAdder &add)
        : add_(add)
    {
    }

    BulkLinearUpdater(const BulkMoneySpender &spend)
        : spend_(spend)
    {
    }

    BulkLinearUpdater(const BulkTaxApplier &tax)
        : tax_(tax)
    {
    }

    void CombineWith(const BulkLinearUpdater &other)
    {
        tax_.percentage = Combine(tax_.percentage, other.tax_.percentage);
        add_.delta = add_.delta * other.tax_.ComputeFactor() + other.add_.delta;
        spend_.delta += other.spend_.delta;
    }

    MoneyData Collapse(MoneyData origin, IndexSegment segment) const
    {
        MoneyData result{};
        result.add = origin.add * tax_.ComputeFactor() + add_.delta * segment.length();
        result.spend = origin.spend + spend_.delta * segment.length();
        return result;
    }

private:
    // apply tax first, then add
    BulkTaxApplier tax_;
    BulkMoneyAdder add_;
    BulkMoneySpender spend_;
};

template <typename Data, typename BulkOperation>
class SummingSegmentTree
{
public:
    SummingSegmentTree(size_t size) : root_(Build({0, size})) {}

    Data ComputeSum(IndexSegment segment) const
    {
        return this->TraverseWithQuery(root_, segment, ComputeSumVisitor{});
    }

    void AddBulkOperation(IndexSegment segment, const BulkOperation &operation)
    {
        this->TraverseWithQuery(root_, segment, AddBulkOperationVisitor{operation});
    }

private:
    struct Node;
    using NodeHolder = unique_ptr<Node>;

    struct Node
    {
        NodeHolder left;
        NodeHolder right;
        IndexSegment segment;
        MoneyData data;
        BulkOperation postponed_bulk_operation;
    };

    NodeHolder root_;

    static NodeHolder Build(IndexSegment segment)
    {
        if (segment.empty())
        {
            return nullptr;
        }
        else if (segment.length() == 1)
        {
            return make_unique<Node>(Node{
                .left = nullptr,
                .right = nullptr,
                .segment = segment,
            });
        }
        else
        {
            const size_t middle = segment.left + segment.length() / 2;
            return make_unique<Node>(Node{
                .left = Build({segment.left, middle}),
                .right = Build({middle, segment.right}),
                .segment = segment,
            });
        }
    }

    template <typename Visitor>
    static typename Visitor::ResultType TraverseWithQuery(const NodeHolder &node, IndexSegment query_segment, Visitor visitor)
    {
        if (!node || !AreSegmentsIntersected(node->segment, query_segment))
        {
            return visitor.ProcessEmpty(node);
        }
        else
        {
            PropagateBulkOperation(node);
            if (query_segment.Contains(node->segment))
            {
                return visitor.ProcessFull(node);
            }
            else
            {
                if constexpr (is_void_v<typename Visitor::ResultType>)
                {
                    TraverseWithQuery(node->left, query_segment, visitor);
                    TraverseWithQuery(node->right, query_segment, visitor);
                    return visitor.ProcessPartial(node, query_segment);
                }
                else
                {
                    return visitor.ProcessPartial(
                        node, query_segment,
                        TraverseWithQuery(node->left, query_segment, visitor),
                        TraverseWithQuery(node->right, query_segment, visitor));
                }
            }
        }
    }

    class ComputeSumVisitor
    {
    public:
        using ResultType = Data;

        Data ProcessEmpty(const NodeHolder &) const
        {
            return {};
        }

        Data ProcessFull(const NodeHolder &node) const
        {
            MoneyData data = node->data;
            return data.add - data.spend;
        }

        Data ProcessPartial(const NodeHolder &, IndexSegment, const Data &left_result, const Data &right_result) const
        {
            return left_result + right_result;
        }
    };

    class AddBulkOperationVisitor
    {
    public:
        using ResultType = void;

        explicit AddBulkOperationVisitor(const BulkOperation &operation)
            : operation_(operation)
        {
        }

        void ProcessEmpty(const NodeHolder &) const {}

        void ProcessFull(const NodeHolder &node) const
        {
            node->postponed_bulk_operation.CombineWith(operation_);
            node->data = operation_.Collapse(node->data, node->segment);
        }

        void ProcessPartial(const NodeHolder &node, IndexSegment) const
        {
            node->data = (node->left ? node->left->data : MoneyData()) + (node->right ? node->right->data : MoneyData());
        }

    private:
        const BulkOperation &operation_;
    };

    static void PropagateBulkOperation(const NodeHolder &node)
    {
        for (auto *child_ptr : {node->left.get(), node->right.get()})
        {
            if (child_ptr)
            {
                child_ptr->postponed_bulk_operation.CombineWith(node->postponed_bulk_operation);
                child_ptr->data = node->postponed_bulk_operation.Collapse(child_ptr->data, child_ptr->segment);
            }
        }
        node->postponed_bulk_operation = BulkOperation();
    }
};

class Date
{
public:
    static Date FromString(string_view str)
    {
        const int year = ConvertToInt(ReadToken(str, "-"));
        const int month = ConvertToInt(ReadToken(str, "-"));
        ValidateBounds(month, 1, 12);
        const int day = ConvertToInt(str);
        ValidateBounds(day, 1, 31);
        return {year, month, day};
    }

    // Weird legacy, can't wait for std::chrono::year_month_day
    time_t AsTimestamp() const
    {
        std::tm t;
        t.tm_sec = 0;
        t.tm_min = 0;
        t.tm_hour = 0;
        t.tm_mday = day_;
        t.tm_mon = month_ - 1;
        t.tm_year = year_ - 1900;
        t.tm_isdst = 0;
        return mktime(&t);
    }

private:
    int year_;
    int month_;
    int day_;

    Date(int year, int month, int day)
        : year_(year), month_(month), day_(day)
    {
    }
};

int ComputeDaysDiff(const Date &date_to, const Date &date_from)
{
    const time_t timestamp_to = date_to.AsTimestamp();
    const time_t timestamp_from = date_from.AsTimestamp();
    static constexpr int SECONDS_IN_DAY = 60 * 60 * 24;
    return (timestamp_to - timestamp_from) / SECONDS_IN_DAY;
}

static const Date START_DATE = Date::FromString("2000-01-01");
static const Date END_DATE = Date::FromString("2100-01-01");
static const size_t DAY_COUNT = ComputeDaysDiff(END_DATE, START_DATE);

size_t ComputeDayIndex(const Date &date)
{
    return ComputeDaysDiff(date, START_DATE);
}

IndexSegment MakeDateSegment(const Date &date_from, const Date &date_to)
{
    return {ComputeDayIndex(date_from), ComputeDayIndex(date_to) + 1};
}

class BudgetManager : public SummingSegmentTree<double, BulkLinearUpdater>
{
public:
    BudgetManager() : SummingSegmentTree(DAY_COUNT) {}
};

struct Request;
using RequestHolder = unique_ptr<Request>;

struct Request
{
    enum class Type
    {
        COMPUTE_INCOME,
        EARN,
        SPEND,
        PAY_TAX
    };

    Request(Type type) : type(type) {}
    static RequestHolder Create(Type type);
    virtual void ParseFrom(string_view input) = 0;
    virtual ~Request() = default;

    const Type type;
};

const unordered_map<string_view, Request::Type> STR_TO_REQUEST_TYPE = {
    {"ComputeIncome", Request::Type::COMPUTE_INCOME},
    {"Earn", Request::Type::EARN},
    {"Spend", Request::Type::SPEND},
    {"PayTax", Request::Type::PAY_TAX},
};

template <typename ResultType>
struct ReadRequest : Request
{
    using Request::Request;
    virtual ResultType Process(const BudgetManager &manager) const = 0;
};

struct ModifyRequest : Request
{
    using Request::Request;
    virtual void Process(BudgetManager &manager) const = 0;
};

struct ComputeIncomeRequest : ReadRequest<double>
{
    ComputeIncomeRequest() : ReadRequest(Type::COMPUTE_INCOME) {}
    void ParseFrom(string_view input) override
    {
        date_from = Date::FromString(ReadToken(input));
        date_to = Date::FromString(input);
    }

    double Process(const BudgetManager &manager) const override
    {
        return manager.ComputeSum(MakeDateSegment(date_from, date_to));
    }

    Date date_from = START_DATE;
    Date date_to = START_DATE;
};

struct EarnRequest : ModifyRequest
{
    EarnRequest() : ModifyRequest(Type::EARN) {}
    void ParseFrom(string_view input) override
    {
        date_from = Date::FromString(ReadToken(input));
        date_to = Date::FromString(ReadToken(input));
        income = ConvertToInt(input);
    }

    void Process(BudgetManager &manager) const override
    {
        const auto date_segment = MakeDateSegment(date_from, date_to);
        const double daily_income = income * 1.0 / date_segment.length();
        manager.AddBulkOperation(date_segment, BulkMoneyAdder{daily_income});
    }

    Date date_from = START_DATE;
    Date date_to = START_DATE;
    size_t income = 0;
};

struct SpendRequest : ModifyRequest
{
    SpendRequest() : ModifyRequest(Type::SPEND) {}
    void ParseFrom(string_view input) override
    {
        date_from = Date::FromString(ReadToken(input));
        date_to = Date::FromString(ReadToken(input));
        spent = ConvertToInt(input);
    }

    void Process(BudgetManager &manager) const override
    {
        const auto date_segment = MakeDateSegment(date_from, date_to);
        const double daily_spent = spent * 1.0 / date_segment.length();
        manager.AddBulkOperation(date_segment, BulkMoneySpender{daily_spent});
    }

    Date date_from = START_DATE;
    Date date_to = START_DATE;
    size_t spent = 0;
};

struct PayTaxRequest : ModifyRequest
{
    PayTaxRequest() : ModifyRequest(Type::PAY_TAX) {}
    void ParseFrom(string_view input) override
    {
        date_from = Date::FromString(ReadToken(input));
        date_to = Date::FromString(ReadToken(input));
        percentage = ConvertToInt(input);
    }

    void Process(BudgetManager &manager) const override
    {
        const BulkTaxApplier bulk_tax_applier{.percentage = percentage};
        manager.AddBulkOperation(MakeDateSegment(date_from, date_to), bulk_tax_applier);
    }

    Date date_from = START_DATE;
    Date date_to = START_DATE;
    double percentage = 0;
};

RequestHolder Request::Create(Request::Type type)
{
    switch (type)
    {
    case Request::Type::COMPUTE_INCOME:
        return make_unique<ComputeIncomeRequest>();
    case Request::Type::EARN:
        return make_unique<EarnRequest>();
    case Request::Type::SPEND:
        return make_unique<SpendRequest>();
    case Request::Type::PAY_TAX:
        return make_unique<PayTaxRequest>();
    default:
        return nullptr;
    }
}

template <typename Number>
Number ReadNumberOnLine(istream &stream)
{
    Number number;
    stream >> number;
    string dummy;
    getline(stream, dummy);
    return number;
}

optional<Request::Type> ConvertRequestTypeFromString(string_view type_str)
{
    if (const auto it = STR_TO_REQUEST_TYPE.find(type_str);
        it != STR_TO_REQUEST_TYPE.end())
    {
        return it->second;
    }
    else
    {
        return nullopt;
    }
}

RequestHolder ParseRequest(string_view request_str)
{
    const auto request_type = ConvertRequestTypeFromString(ReadToken(request_str));
    if (!request_type)
    {
        return nullptr;
    }
    RequestHolder request = Request::Create(*request_type);
    if (request)
    {
        request->ParseFrom(request_str);
    };
    return request;
}

vector<RequestHolder> ReadRequests(istream &in_stream = cin)
{
    const size_t request_count = ReadNumberOnLine<size_t>(in_stream);

    vector<RequestHolder> requests;
    requests.reserve(request_count);

    for (size_t i = 0; i < request_count; ++i)
    {
        string request_str;
        getline(in_stream, request_str);
        if (auto request = ParseRequest(request_str))
        {
            requests.push_back(move(request));
        }
    }
    return requests;
}

vector<double> ProcessRequests(const vector<RequestHolder> &requests)
{
    vector<double> responses;
    BudgetManager manager;
    for (const auto &request_holder : requests)
    {
        if (request_holder->type == Request::Type::COMPUTE_INCOME)
        {
            const auto &request = static_cast<const ComputeIncomeRequest &>(*request_holder);
            responses.push_back(request.Process(manager));
        }
        else
        {
            const auto &request = static_cast<const ModifyRequest &>(*request_holder);
            request.Process(manager);
        }
    }
    return responses;
}

void PrintResponses(const vector<double> &responses, ostream &stream = cout)
{
    for (const double response : responses)
    {
        stream << response << endl;
    }
}

int main()
{
    TestAll();

    cout.precision(25);
    const auto requests = ReadRequests();
    const auto responses = ProcessRequests(requests);
    PrintResponses(responses);

    return 0;
}

void TestBasic()
{
    {
        istringstream iss(R"(
16
Earn 2000-01-02 2000-01-02 2
Earn 2000-01-03 2000-01-03 2
Earn 2000-01-04 2000-01-04 2
PayTax 2000-01-02 2000-01-02 13
PayTax 2000-01-01 2000-01-01 13
PayTax 2000-01-05 2000-01-05 13
PayTax 2000-01-05 2099-01-05 13
ComputeIncome 2000-01-02 2000-01-02
ComputeIncome 2000-01-03 2000-01-03
ComputeIncome 2000-01-04 2000-01-04
ComputeIncome 2000-01-05 2000-01-05
ComputeIncome 2000-01-01 2000-01-01
ComputeIncome 2000-01-02 2000-01-04
ComputeIncome 2001-01-02 2099-01-04
PayTax 2000-01-03 2099-01-01 13
ComputeIncome 2000-01-02 2000-01-04
        )");
        ostringstream oss;
        oss.precision(25);
        const auto requests = ReadRequests(iss);
        const auto responses = ProcessRequests(requests);
        PrintResponses(responses, oss);
        string expect = "1.739999999999999991118216\n2\n2\n0\n0\n5.740000000000000213162821\n0\n5.219999999999999751310042\n";
        ASSERT_EQUAL(oss.str(), expect);
    }
    {
        istringstream iss(R"(
3
Earn 2000-01-01 2000-01-01 10
Spend 2000-01-01 2000-01-01 10
ComputeIncome 2000-01-01 2000-01-01
        )");
        ostringstream oss;
        oss.precision(25);
        const auto requests = ReadRequests(iss);
        const auto responses = ProcessRequests(requests);
        PrintResponses(responses, oss);
        string expect = "0\n";
        ASSERT_EQUAL(oss.str(), expect);
    }
    {
        istringstream iss(R"(
7
Earn 2000-01-01 2000-01-01 10
Spend 2000-01-01 2000-01-01 4
ComputeIncome 2000-01-01 2000-01-01
Earn 2000-01-01 2000-01-01 5
ComputeIncome 2000-01-01 2000-01-01
Spend 2000-01-01 2000-01-01 2
ComputeIncome 2000-01-01 2000-01-01
        )");
        ostringstream oss;
        oss.precision(25);
        const auto requests = ReadRequests(iss);
        const auto responses = ProcessRequests(requests);
        PrintResponses(responses, oss);
        string expect = "6\n11\n9\n";
        ASSERT_EQUAL(oss.str(), expect);
    }
    {
        istringstream iss(R"(
4
Earn 2000-01-01 2000-01-03 18
PayTax 2000-01-01 2000-01-05 15
PayTax 2000-01-01 2000-01-05 13
ComputeIncome 2000-01-01 2000-01-03
        )");
        ostringstream oss;
        oss.precision(25);
        const auto requests = ReadRequests(iss);
        const auto responses = ProcessRequests(requests);
        PrintResponses(responses, oss);
        string expect = "13.31099999999999816679974\n";
        ASSERT_EQUAL(oss.str(), expect);
    }
}

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, TestBasic);
}

void Profile()
{
}