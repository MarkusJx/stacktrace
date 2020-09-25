#ifndef STACKTRACE_ADDR2LINE_HPP
#define STACKTRACE_ADDR2LINE_HPP

#include "addr2line.h"
#include <vector>
#include <cstring>
#include <map>

namespace addr2line {
    /**
     * The result of addr2line.
     * If status == 0 info will contain an array of size
     * naddr with information about the addresses requested.
     * If status != 0, info will be nullptr.
     * This class will handle freeing any resources allocated.
     */
    class addr2line_res {
    public:
        /**
         * Create addr2line_res from a addr2line_result struct
         *
         * @param res the struct to copy the data from
         * @param naddr the number of address_infos in res.info
         */
        inline explicit addr2line_res(const addr2line_result &res, int naddr) : info(naddr), status(res.status),
                                                                                err_msg(res.err_msg) {
            if (status == 0)
                memcpy(info.data(), res.info, naddr * sizeof(address_info));
            else
                info.resize(0); // addr2line failed make info an empty vector
        }

        /**
         * Copy constructor
         *
         * @param res the instance to copy from
         */
        inline addr2line_res(const addr2line_res &res) = default;

        /**
         * Move constructor
         *
         * @param res the instance to move
         */
        inline addr2line_res(addr2line_res &&res) noexcept: info(res.info), status(res.status), err_msg(res.err_msg) {}

        /**
         * Operator=
         *
         * @param res the object to get the values from
         * @return this
         */
        inline addr2line_res &operator=(const addr2line_res &res) {
            info = res.info;
            status = res.status;
            err_msg = res.err_msg;

            return *this;
        }

        /**
         * The default destructor
         */
        inline ~addr2line_res() = default;

        std::vector<address_info> info; // The address infos
        int status; // The status
        const char *err_msg; // The error message, if available
    };

    // A map containing a file name as a key
    // and an addr2line_re object containing address information as a value
    typedef std::map<std::string, addr2line_res> address_map;

    /**
     * Parse an array of addresses created by backtrace(2)
     *
     * @param addr the addresses
     * @param naddr the number of addresses
     * @return a map with a file name as key and a vector containing hex addresses for this file
     */
    inline std::map<std::string, std::vector<std::string>> parseAddressArray(void **addr, int naddr) {
        char **messages = backtrace_symbols(addr, naddr);
        std::map<std::string, std::vector<std::string>> tmp;

        std::string msg, file, address;
        size_t o, p, c;
        for (int i = 0; i < naddr; i++) {
            msg = messages[i];
            o = msg.find('(');
            p = msg.find('+');
            if (o == msg.npos || p == msg.npos) continue;

            file = msg.substr(0, o);
            msg = msg.substr(p);

            c = msg.find(')');
            if (c == msg.npos) continue;

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

    /**
     * Process a file using logic from the addr2line tool
     *
     * @param file_name the path to the file
     * @param addr an array of the addresses to process
     * @param naddr the number of addresses to process
     * @param section_name the name of the section or nullptr if not needed
     * @param target the target or nullptr if not needed
     * @return the result of the operation
     */
    inline addr2line_res
    process(const char *file_name, const char **addr, int naddr, const char *section_name = nullptr,
            const char *target = nullptr) {
        addr2line_result result = ::process_file(file_name, section_name, target, addr, naddr);
        addr2line_res res(result, naddr);
        free(result.info);
        return res;
    }

    /**
     * Process a map created by parseAddressArray(2)
     *
     * @param m the map to process
     * @return an adress_map object
     */
    inline address_map process(const std::map<std::string, std::vector<std::string>> &m) {
        std::map<std::string, addr2line_res> tmp;
        for (const auto &p : m) {
            std::vector<const char *> data;
            for (const auto &s : p.second) data.push_back(s.c_str());

            addr2line_res res = process(p.first.c_str(), data.data(), data.size());
            tmp.insert_or_assign(p.first, res);
        }

        return tmp;
    }

    /**
     * Process an address array created by backtrace(2) to get address information from the backtrace
     * using addr2line logic
     *
     * @param addr the address array to process
     * @param naddr the number of addresses in the array
     * @return an address_map object
     */
    inline address_map processAddressArray(void **addr, int naddr) {
        std::map<std::string, std::vector<std::string>> m = parseAddressArray(addr, naddr);
        return process(m);
    }
}

#endif //STACKTRACE_ADDR2LINE_HPP
