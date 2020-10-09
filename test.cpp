#include <iostream>
#include "test.hpp"
#include "stacktrace.hpp"

void test_1() {
    std::cout << "Call in test_1:" << std::endl << markusjx::stacktrace::stacktrace() << std::endl;
}

void test::test_2() {
    std::cout << "Call in test_2:" << std::endl << markusjx::stacktrace::stacktrace() << std::endl;
}