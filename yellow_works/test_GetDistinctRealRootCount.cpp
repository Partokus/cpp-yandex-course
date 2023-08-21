#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <optional>
#include <variant>
#include <cmath>
#include <numeric>
#include <limits>
#include <tuple>

using namespace std;

template <class T>
ostream &operator<<(ostream &os, const vector<T> &s)
{
  os << "{";
  bool first = true;
  for (const auto &x : s)
  {
    if (!first)
    {
      os << ", ";
    }
    first = false;
    os << x;
  }
  return os << "}";
}

template <class T>
ostream &operator<<(ostream &os, const set<T> &s)
{
  os << "{";
  bool first = true;
  for (const auto &x : s)
  {
    if (!first)
    {
      os << ", ";
    }
    first = false;
    os << x;
  }
  return os << "}";
}

template <class K, class V>
ostream &operator<<(ostream &os, const map<K, V> &m)
{
  os << "{";
  bool first = true;
  for (const auto &kv : m)
  {
    if (!first)
    {
      os << ", ";
    }
    first = false;
    os << kv.first << ": " << kv.second;
  }
  return os << "}";
}

template <class T, class U>
void AssertEqual(const T &t, const U &u, const string &hint = {})
{
  if (t != u)
  {
    ostringstream os;
    os << "Assertion failed: " << t << " != " << u;
    if (!hint.empty())
    {
      os << " hint: " << hint;
    }
    throw runtime_error(os.str());
  }
}

void Assert(bool b, const string &hint)
{
  AssertEqual(b, true, hint);
}

class TestRunner
{
public:
  template <class TestFunc>
  void RunTest(TestFunc func, const string &test_name)
  {
    try
    {
      func();
      cerr << test_name << " OK" << endl;
    }
    catch (exception &e)
    {
      ++fail_count;
      cerr << test_name << " fail: " << e.what() << endl;
    }
    catch (...)
    {
      ++fail_count;
      cerr << "Unknown exception caught" << endl;
    }
  }

  ~TestRunner()
  {
    if (fail_count > 0)
    {
      cerr << fail_count << " unit tests failed. Terminate" << endl;
      exit(1);
    }
  }

private:
  int fail_count = 0;
};

int GetDistinctRealRootCount(double a, double b, double c)
{
  double d = pow(b, 2) - 4 * a * c;

  if (a != 0)
  {
    if (d > 0)
    {
      return 2;
    }
    else if (d == 0)
    {
      return 1;
    }
  }
  else if (b != 0)
  {
    return 1;
  }
  return 0;
}

void testGetDistinctRealRootCount()
{
  AssertEqual(GetDistinctRealRootCount(5, 2, 4), 0, "Zero 0");
  AssertEqual(GetDistinctRealRootCount(5, -2, 4), 0, "Zero 1");
  AssertEqual(GetDistinctRealRootCount(0, 3, 5), 1, "One 0");
  AssertEqual(GetDistinctRealRootCount(0, 3, 500), 1, "One 1");
  AssertEqual(GetDistinctRealRootCount(8, 8, 2), 1, "One 2");
  AssertEqual(GetDistinctRealRootCount(5, 2, -4), 2, "Two 0");
  AssertEqual(GetDistinctRealRootCount(-5, 2, 4), 2, "Two 1");
  AssertEqual(GetDistinctRealRootCount(-5, -2, 4), 2, "Two 2");
  AssertEqual(GetDistinctRealRootCount(0, 0, 5), 0, "Zero 2");
  AssertEqual(GetDistinctRealRootCount(0, 5, 0), 1, "One 3");
  AssertEqual(GetDistinctRealRootCount(5, 0, 0), 1, "One 4");
}

int main()
{
  TestRunner runner;
  runner.RunTest(testGetDistinctRealRootCount, "GetDistinctRealRootCount");
  return 0;
}
