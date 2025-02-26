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
#include <stdexcept>

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

vector<string_view> Split(string_view s, string_view delimiter = " ")
{
    vector<string_view> parts;
    if (s.empty())
    {
        return parts;
    }
    while (true)
    {
        const auto [lhs, rhs_opt] = SplitTwoStrict(s, delimiter);
        parts.push_back(lhs);
        if (!rhs_opt)
        {
            break;
        }
        s = *rhs_opt;
    }
    return parts;
}

class Domain
{
public:
    explicit Domain(string_view text)
    {
        vector<string_view> parts = Split(text, ".");
        parts_reversed_.assign(rbegin(parts), rend(parts));
    }

    size_t GetPartCount() const
    {
        return parts_reversed_.size();
    }

    auto GetParts() const
    {
        return Range(rbegin(parts_reversed_), rend(parts_reversed_));
    }
    auto GetReversedParts() const
    {
        return Range(begin(parts_reversed_), end(parts_reversed_));
    }

    bool operator==(const Domain &other) const
    {
        return parts_reversed_ == other.parts_reversed_;
    }

private:
    vector<string> parts_reversed_;
};

ostream &operator<<(ostream &stream, const Domain &domain)
{
    bool first = true;
    for (const string_view part : domain.GetParts())
    {
        if (!first)
        {
            stream << '.';
        }
        else
        {
            first = false;
        }
        stream << part;
    }
    return stream;
}

// domain is subdomain of itself
bool IsSubdomain(const Domain &subdomain, const Domain &domain)
{
    const auto subdomain_reversed_parts = subdomain.GetReversedParts();
    const auto domain_reversed_parts = domain.GetReversedParts();
    return subdomain.GetPartCount() >= domain.GetPartCount() && equal(begin(domain_reversed_parts), end(domain_reversed_parts),
                                                                      begin(subdomain_reversed_parts));
}

bool IsSubOrSuperDomain(const Domain &lhs, const Domain &rhs)
{
    return lhs.GetPartCount() >= rhs.GetPartCount()
               ? IsSubdomain(lhs, rhs)
               : IsSubdomain(rhs, lhs);
}

class DomainChecker
{
public:
    template <typename InputIt>
    DomainChecker(InputIt domains_begin, InputIt domains_end)
    {
        sorted_domains_.reserve(distance(domains_begin, domains_end));
        for (const Domain &domain : Range(domains_begin, domains_end))
        {
            sorted_domains_.push_back(&domain);
        }
        sort(begin(sorted_domains_), end(sorted_domains_), IsDomainLess);
        sorted_domains_ = AbsorbSubdomains(move(sorted_domains_));
    }

    // Check if candidate is subdomain of some domain
    bool IsSubdomain(const Domain &candidate) const
    {
        const auto it = upper_bound(
            begin(sorted_domains_), end(sorted_domains_),
            &candidate, IsDomainLess);
        if (it == begin(sorted_domains_))
        {
            return false;
        }
        return ::IsSubdomain(candidate, **prev(it));
    }

private:
    vector<const Domain *> sorted_domains_;

    static bool IsDomainLess(const Domain *lhs, const Domain *rhs)
    {
        const auto lhs_reversed_parts = lhs->GetReversedParts();
        const auto rhs_reversed_parts = rhs->GetReversedParts();
        return lexicographical_compare(
            begin(lhs_reversed_parts), end(lhs_reversed_parts),
            begin(rhs_reversed_parts), end(rhs_reversed_parts));
    }

    static vector<const Domain *> AbsorbSubdomains(vector<const Domain *> domains)
    {
        domains.erase(
            unique(begin(domains), end(domains),
                   [](const Domain *lhs, const Domain *rhs)
                   {
                       return IsSubOrSuperDomain(*lhs, *rhs);
                   }),
            end(domains));
        return domains;
    }
};

vector<Domain> ReadDomains(istream &in_stream = cin)
{
    vector<Domain> domains;

    size_t count;
    in_stream >> count;
    domains.reserve(count);

    for (size_t i = 0; i < count; ++i)
    {
        string domain_text;
        in_stream >> domain_text;
        domains.emplace_back(domain_text);
    }
    return domains;
}

vector<bool> CheckDomains(const vector<Domain> &banned_domains, const vector<Domain> &domains_to_check)
{
    const DomainChecker checker(begin(banned_domains), end(banned_domains));

    vector<bool> check_results;
    check_results.reserve(domains_to_check.size());
    for (const Domain &domain_to_check : domains_to_check)
    {
        check_results.push_back(!checker.IsSubdomain(domain_to_check));
    }

    return check_results;
}

void PrintCheckResults(const vector<bool> &check_results, ostream &out_stream = cout)
{
    for (const bool check_result : check_results)
    {
        out_stream << (check_result ? "Good" : "Bad") << "\n";
    }
}

int main()
{
    TestAll();

    const vector<Domain> banned_domains = ReadDomains();
    const vector<Domain> domains_to_check = ReadDomains();
    PrintCheckResults(CheckDomains(banned_domains, domains_to_check));

    return 0;
}

void TestParse()
{
    istringstream iss(R"(
        4
        ya.ru
        maps.me
        m.ya.ru
        com
        7
        ya.ru
        ya.com
        m.maps.me
        moscow.m.ya.ru
        maps.com
        maps.ru
        ya.ya
    )");

    vector<Domain> bad_domens_expect{
        Domain{"ya.ru"},
        Domain{"maps.me"},
        Domain{"m.ya.ru"},
        Domain{"com"}
    };

    vector<Domain> domens_for_check_expect{
        Domain{"ya.ru"},
        Domain{"ya.com"},
        Domain{"m.maps.me"},
        Domain{"moscow.m.ya.ru"},
        Domain{"maps.com"},
        Domain{"maps.ru"},
        Domain{"ya.ya"}
    };

    vector<Domain> bad_domens = ReadDomains(iss);
    vector<Domain> domens_for_check = ReadDomains(iss);

    ASSERT_EQUAL(bad_domens, bad_domens_expect);
    ASSERT_EQUAL(domens_for_check, domens_for_check_expect);
}

void TestFilter()
{
    {
        istringstream iss(R"(
            4
            ya.ru
            maps.me
            m.ya.ru
            com
            7
            ya.ru
            ya.com
            m.maps.me
            moscow.m.ya.ru
            maps.com
            maps.ru
            ya.ya
        )");

        const vector<Domain> banned_domains = ReadDomains(iss);
        const vector<Domain> domains_to_check = ReadDomains(iss);
        vector<bool> is_good = CheckDomains(banned_domains, domains_to_check);

        const vector<bool> expect{
           false, false, false, false, false, true, true
        };

        ASSERT_EQUAL(is_good, expect);
    }

    {
        istringstream iss(R"(
            2
            com
            maps.ru
            7
            maps.com.ru
            ya.com
            ya.common
            ya.common.com
            com.ru
            com.ru.com
            maps.ru.c.maps.ru
        )");

        const vector<Domain> banned_domains = ReadDomains(iss);
        const vector<Domain> domains_to_check = ReadDomains(iss);
        const vector<bool> is_good = CheckDomains(banned_domains, domains_to_check);

        const vector<bool> expect{
           true, false, true, false, true, false, false
        };

        ASSERT_EQUAL(is_good, expect);
    }

    {
        istringstream iss(R"(
            2
            com
            maps.ru
            6
            ya.com
            ya.common
            ya.common.com
            com.ru
            com.ru.com
            maps.ru.c.maps.ru
        )");

        const vector<Domain> banned_domains = ReadDomains(iss);
        const vector<Domain> domains_to_check = ReadDomains(iss);
        const vector<bool> is_good = CheckDomains(banned_domains, domains_to_check);

        const vector<bool> expect{
            false, true, false, true, false, false
        };

        ASSERT_EQUAL(is_good, expect);
    }

    {
        istringstream iss(R"(
            7
            common.com
            pop
            com.com
            mem.sem.kem.pem
            a
            e
            p
            13
            ya.com
            ya.common
            ya.common.com
            com
            common
            pol.mem.sem.kem.pem.mol
            mem.sem.kem.pem.mol
            pol.mem.sem.kem.pem
            pol.mem.sam.kem.pem
            mem.sem.kem
            poppop
            po.p
            pop.pop
        )");

        const vector<Domain> banned_domains = ReadDomains(iss);
        const vector<Domain> domains_to_check = ReadDomains(iss);
        const vector<bool> is_good = CheckDomains(banned_domains, domains_to_check);

        const vector<bool> expect{
            true, true, false, true, true, true, true,
            false, true, true, true, false, false
        };

        ASSERT_EQUAL(is_good, expect);

        ostringstream oss;
        PrintCheckResults(expect, oss);
        ASSERT_EQUAL(oss.str(), "Good\nGood\nBad\nGood\nGood\nGood\nGood\nBad\nGood\nGood\nGood\nBad\nBad\n");
    }

    {
        Domain domain{"ya.ru.com.mac"};

        vector<string> expect_forward{"ya", "ru", "com", "mac"};
        vector<string> expect_reverse{"mac", "com", "ru", "ya"};

        const Range range_forward = domain.GetParts();
        const Range range_reverse = domain.GetReversedParts();

        ASSERT(std::equal(expect_forward.begin(), expect_forward.end(),
                          range_forward.begin(), range_forward.end()));
        ASSERT(std::equal(expect_reverse.begin(), expect_reverse.end(),
                          range_reverse.begin(), range_reverse.end()));
    }

    {
        Domain domain1{"ya.ru.com.mac"};
        Domain domain2{"ya.ru.com.mac"};

        ASSERT(IsSubdomain(domain1, domain2));
        ASSERT(IsSubdomain(domain1, domain2));
    }

    {
        Domain domain1{"ya.ru.com.mac"};
        Domain domain2{"com.mac"};
        Domain domain3{"com"};
        Domain domain4{"ru.com"};

        ASSERT(IsSubdomain(domain1, domain2));
        ASSERT(not IsSubdomain(domain2, domain1));
        ASSERT(not IsSubdomain(domain1, domain3));
        ASSERT(not IsSubdomain(domain1, domain4));
    }

    {
        vector<Domain> domains{
            Domain{"ya.ru.com.mac"},
            Domain{"m.maps.ru"},
            Domain{"com.mac"},
            Domain{"com"},
            Domain{"mac.com"},
        };

        DomainChecker dc{domains.begin(), domains.end()};

        ASSERT(dc.IsSubdomain(Domain{"ya.ru.com.mac"}));
        ASSERT(dc.IsSubdomain(Domain{"m.maps.ru"}));
        ASSERT(dc.IsSubdomain(Domain{"com.mac"}));
        ASSERT(!dc.IsSubdomain(Domain{"com.mac.kak"}));
        ASSERT(dc.IsSubdomain(Domain{"a.m.maps.ru"}));
        ASSERT(!dc.IsSubdomain(Domain{"a.m.maps.ru.m"}));
    }

    {
    }
}

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, TestParse);
    RUN_TEST(tr, TestFilter);
}

void Profile()
{
}