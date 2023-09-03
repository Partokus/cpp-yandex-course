#pragma once
#include <iostream>
#include "test_asserts.h"

class TestRunner
{
public:
    ~TestRunner();

    template <class TestFunc>
    void RunTest(TestFunc func, const std::string &test_name);

private:
    int fail_count = 0;
};

template <class TestFunc>
void TestRunner::RunTest(TestFunc func, const std::string &test_name)
{
    try
    {
        func();
        std::cerr << test_name << " OK" << std::endl;
    }
    catch (std::exception &e)
    {
        ++fail_count;
        std::cerr << test_name << " fail: " << e.what() << std::endl;
    }
    catch (...)
    {
        ++fail_count;
        std::cerr << "Unknown exception caught" << std::endl;
    }
}