#ifndef STACKTRACE_ADDR2LINE_HPP
#define STACKTRACE_ADDR2LINE_HPP

#include "addr2line.h"
#include <vector>
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
        explicit addr2line_res(const addr2line_result &res, int naddr);

        /**
         * Copy constructor
         *
         * @param res the instance to copy from
         */
        addr2line_res(const addr2line_res &res);

        /**
         * Move constructor
         *
         * @param res the instance to move
         */
        addr2line_res(addr2line_res &&res) noexcept;

        /**
         * Operator=
         *
         * @param res the object to get the values from
         * @return this
         */
        addr2line_res &operator=(const addr2line_res &res);

        /**
         * The default destructor
         */
        ~addr2line_res();

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
    std::map<std::string, std::vector<std::string>> parseAddressArray(void **addr, int naddr);

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
    addr2line_res process(const char *file_name, const char **addr, int naddr, const char *section_name = nullptr,
                          const char *target = nullptr);

    /**
     * Process a map created by parseAddressArray(2)
     *
     * @param m the map to process
     * @return an adress_map object
     */
    address_map processMap(const std::map<std::string, std::vector<std::string>> &m);

    /**
     * Process an address array created by backtrace(2) to get address information from the backtrace
     * using addr2line logic
     *
     * @param addr the address array to process
     * @param naddr the number of addresses in the array
     * @return an address_map object
     */
    address_map processAddressArray(void **addr, int naddr);

    addr2line_res processAddress(const char *addr);
}

#endif //STACKTRACE_ADDR2LINE_HPP
