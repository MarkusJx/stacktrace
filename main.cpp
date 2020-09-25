#include <iostream>
#include <execinfo.h>
#include <sstream>
#include <cstdio>
#include "addr2line.hpp"

int main() {
    // Set some options
    set_options(true, true, true, nullptr);

    void *arr[128];
    int sz = backtrace(arr, sizeof(arr));

    addr2line::address_map m = addr2line::processAddressArray(arr, sz);

    for (const auto &p : m) {
        std::cout << "File: " << p.first << std::endl;
        auto res = p.second;

        // Only do this if the operation did not fail
        if (res.status == 0) {
            for (const auto &info : res.info) {
                // Print the information
                printf("\t%s in %s:%u, address: 0x%08lx\n", info.name, info.basename, info.line, info.address);
            }
        }
    }

    return 0;
}
