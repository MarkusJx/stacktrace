/*
 * addr2line.h
 * Used to get addr2line functionality without executing the program itself.
 *
 * MIT License
 * Copyright (c) 2020 MarkusJx
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef MARKUSJX_ADDR2LINE_H
#define MARKUSJX_ADDR2LINE_H

#ifdef __cplusplus
extern "C" {
#endif //C++

// Information about an address.
// Any values that could not be retrieved will be set to 0.
// Must be usually freed using free().
typedef struct address_info_s {
    char name[256]; // The name of the function
    char filename[512]; // The full path of the file
    char basename[256]; // The name of the file
    unsigned int line; // The line
    unsigned int discriminator; // A discriminator
    unsigned long address; // The address
} address_info;

// The result of a call to addr2line.
// If status == 0, info contains an array of size
// naddr and must be freed using free(). If status != 0,
// info will be nullptr and therefore not needs to be freed.
typedef struct addr2line_result_s {
    // The information about the addresses requested.
    // Is an array the size of naddr.
    // Must be freed using free().
    address_info *info;
    // The status of the call.
    // Equals to 0 if the call succeeded, 1, if a general error occurred,
    // 2, if an allocation error occurred (out of memory etc.).
    int status;
    // The error message (if available).
    // Will be nullptr if no error message is available.
    // Must not be freed whatsoever.
    const char *err_msg;
} addr2line_result;

/**
 * Process a file using logic from the addr2line tool
 *
 * @param file_name the path to the file
 * @param section_name the name of the section or nullptr if not needed
 * @param target the target or nullptr if not needed
 * @param addr an array of the addresses to process
 * @param naddr the number of addresses to process
 * @return the result of the operation
 */
addr2line_result
process_file(const char *file_name, const char *section_name, const char *target, const char **addr, int naddr);

/**
 * Set some options for addr2line
 *
 * @param unwind_inlines whether to unwind inlined functions. Equals to the -i option
 * @param no_recurse_limit whether to not have a recurse limit. Equals to the -r option
 * @param demangle whether to demangle function names. Equals to the -C option
 * @param demangling_style the demangling style or nullptr if not needed
 */
void set_options(int unwind_inlines, int no_recurse_limit, int demangle, const char *demangling_style);

/**
 * Get the last error from bfd
 *
 * @return the error message
 */
const char *bfd_getError();

/**
 * Check if bfd is ok
 *
 * @return 1 if bfd is ok, 0 otherwise
 */
int bfd_ok();

#ifdef __cplusplus
}
#endif //C++

#endif //MARKUSJX_ADDR2LINE_H
