#include "stacktrace.hpp"
#include "test.hpp"
#include <iostream>

void fn_5() {
    std::cout << "Call in fn_5:" << std::endl << markusjx::stacktrace::stacktrace() << std::endl;
}

void fn_4() {
    std::cout << "Call in fn_4:" << std::endl << markusjx::stacktrace::stacktrace() << std::endl;
    fn_5();
}

void fn_3() {
    std::cout << "Call in fn_3:" << std::endl << markusjx::stacktrace::stacktrace() << std::endl;
    fn_4();
}

void fn_2() {
    std::cout << "Call in fn_2:" << std::endl << markusjx::stacktrace::stacktrace() << std::endl;
    fn_3();
}

void fn_1() {
    std::cout << "Call in fn_1:" << std::endl << markusjx::stacktrace::stacktrace() << std::endl;
    fn_2();
}

int main() {
    std::cout << "Call in main:" << std::endl << markusjx::stacktrace::stacktrace() << std::endl;
    fn_1();
    test_1();
    test::test_2();

    return 0;
}
