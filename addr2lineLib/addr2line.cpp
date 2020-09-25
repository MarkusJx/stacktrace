#include "addr2line.hpp"

#include <execinfo.h>
#include <cstring>

addr2line::addr2line_res::addr2line_res(const addr2line_result &res, int naddr) : info(naddr), status(res.status),
                                                                                  err_msg(res.err_msg) {
    if (status == 0)
        memcpy(info.data(), res.info, naddr * sizeof(address_info));
    else
        info.resize(0); // addr2line failed make info an empty vector
}

addr2line::addr2line_res::addr2line_res(const addr2line_res &res) = default;

addr2line::addr2line_res::addr2line_res(addr2line_res &&res) noexcept: info(std::move(res.info)), status(res.status),
                                                                       err_msg(res.err_msg) {}

addr2line::addr2line_res &addr2line::addr2line_res::operator=(const addr2line::addr2line_res &res) {
    if (&res != this) {
        info = res.info;
        status = res.status;
        err_msg = res.err_msg;
    }

    return *this;
}

addr2line::addr2line_res::~addr2line_res() = default;

std::map<std::string, std::vector<std::string>> addr2line::parseAddressArray(void **addr, int naddr) {
    char **messages = backtrace_symbols(addr, naddr);
    std::map<std::string, std::vector<std::string>> tmp;

    std::string msg, file, address;
    size_t o, p, c;
    for (int i = 0; i < naddr; i++) {
        msg = messages[i];
        o = msg.find('(');
        p = msg.find('+');
        if (o == std::string::npos || p == std::string::npos) continue;

        file = msg.substr(0, o);
        msg = msg.substr(p);

        c = msg.find(')');
        if (c == std::string::npos) continue;

        address = msg.substr(0, c);

        if (tmp.find(file) == tmp.end()) {
            // File does not exist in tmp, insert new values
            tmp.insert(std::pair<std::string, std::vector<std::string>>(file, {address}));
        } else {
            // File exists as key in tmp, append address to the vector
            tmp.at(file).push_back(address);
        }
    }

    return tmp;
}

addr2line::addr2line_res
addr2line::process(const char *file_name, const char **addr, int naddr, const char *section_name, const char *target) {
    addr2line_result result = ::process_file(file_name, section_name, target, addr, naddr);
    addr2line_res res(result, naddr);
    free(result.info);
    return res;
}

addr2line::address_map addr2line::processMap(const std::map<std::string, std::vector<std::string>> &m) {
    std::map<std::string, addr2line_res> tmp;
    for (const auto &p : m) {
        std::vector<const char *> data;
        for (const auto &s : p.second) data.push_back(s.c_str());

        addr2line_res res = process(p.first.c_str(), data.data(), data.size());
        tmp.insert_or_assign(p.first, res);
    }

    return tmp;
}

addr2line::address_map addr2line::processAddressArray(void **addr, int naddr) {
    std::map<std::string, std::vector<std::string>> m = parseAddressArray(addr, naddr);
    return processMap(m);
}

addr2line::addr2line_res addr2line::processAddress(const char *addr) {
    addr2line_res res({nullptr, 1, nullptr}, 0);

    std::string msg, file, address;
    size_t o, p, c;
    msg = addr;
    o = msg.find('(');
    p = msg.find('+');
    if (o == std::string::npos || p == std::string::npos) return res;

    file = msg.substr(0, o);
    msg = msg.substr(p);

    c = msg.find(')');
    if (c == std::string::npos) return res;

    address = msg.substr(0, c);

    std::vector<const char *> data = {address.c_str()};

    return addr2line::process(file.c_str(), data.data(), 1, nullptr, nullptr);
}