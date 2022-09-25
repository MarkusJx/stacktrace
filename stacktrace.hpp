/*
 * stacktrace.hpp
 *
 * Licensed under the MIT License
 *
 * Copyright (c) 2020 MarkusJx
 *
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

#ifndef MARKUSJX_STACKTRACE_HPP
#define MARKUSJX_STACKTRACE_HPP

#include <string>
#include <vector>
#include <sstream>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#   define STACKTRACE_SLASH '\\'
#   define STACKTRACE_WINDOWS
#else
#   define STACKTRACE_SLASH '/'
#   define STACKTRACE_UNIX
#endif //Windows

#ifdef _MSVC_LANG

#   include <Windows.h>
#   include <dbghelp.h>
#   include <memory>

#   pragma comment(lib, "Dbghelp.lib")
#endif //MSVC

#ifdef STACKTRACE_UNIX

#   include <execinfo.h>
#   include <dlfcn.h>
#   include <cxxabi.h>

#endif //Unix

#if defined(_MSVC_LANG) || defined(__cplusplus)
#   if (defined(_MSVC_LANG) && _MSVC_LANG > 201402L) || __cplusplus > 201402L // C++17
#       define STACKTRACE_NODISCARD [[nodiscard]]
#       define STACKTRACE_UNUSED [[maybe_unused]]
#       define STACKTRACE_CXX17
#   else
#       define STACKTRACE_NODISCARD
#       define STACKTRACE_UNUSED
#   endif //C++17
#endif //MSVC || cplusplus

namespace markusjx {
    namespace stacktrace {

        /**
         * A stack frame. Virtual.
         */
        class frame {
        protected:
            /**
             * The frame constructor
             */
            explicit frame(const void *address);

            /**
             * Copy constructor
             *
             * @param f the frame to copy from
             */
            frame(const frame &f);

            /**
             * Move constructor
             *
             * @param f the frame to move
             */
            frame(frame &&f) noexcept;

        public:
            /**
             * Get the function name
             *
             * @return the function name
             */
            STACKTRACE_NODISCARD const std::string &getFunction() const noexcept;

            /**
             * Get the full file path
             *
             * @return the file path
             */
            STACKTRACE_NODISCARD const std::string &getFullFilePath() const noexcept;

            /**
             * Get the file name
             *
             * @return the file name
             */
            STACKTRACE_NODISCARD const std::string &getFile() const noexcept;

            /**
             * Get the line number of the call
             *
             * @return the line number
             */
            STACKTRACE_NODISCARD size_t getLine() const noexcept;

            /**
             * Get the stored function address
             *
             * @return the function pointer
             */
            STACKTRACE_NODISCARD const void *getAddress() const noexcept;

            /**
             * Convert the frame to a string. Purely virtual function so frame cannot be constructed.
             *
             * @param fullPaths whether the full path names should be used
             * @return the frame as a string
             */
            STACKTRACE_NODISCARD virtual std::string toString(bool fullPaths) const = 0;

            /**
             * The frame destructor
             */
            virtual ~frame();

        protected:
            std::string function;
            std::string fullFile;
            std::string file;
            size_t line;
            const void *address;
        };

#ifdef STACKTRACE_WINDOWS
        using handle_ptr = std::shared_ptr<void>;

        /**
         * The type of a frame on windows. Can be win_debug_frame or win_release_frame.
         */
        enum win_frame_type {
            debug = 1,
            release = 2
        };

        /**
         * A frame on windows. Virtual.
         */
        class win_frame : public frame {
        protected:
            /**
             * The constructor of win_frame
             */
            win_frame(const void *address);

            /**
             * Copy constructor of win_frame
             *
             * @param f the frame to copy from
             */
            win_frame(const win_frame &f);

            /**
             * Move constructor of win_frame
             *
             * @param f the frame to move
             */
            win_frame(win_frame &&f) noexcept;

        public:
            /**
             * Get the type of this frame
             *
             * @return the type of this frame
             */
            STACKTRACE_NODISCARD virtual win_frame_type getType() const noexcept = 0;
        };

        /**
         * A windows debug frame
         */
        class STACKTRACE_UNUSED win_debug_frame : public win_frame {
        public:
            /**
             * The win_debug_frame constructor
             *
             * @param address the address of the function
             * @param handle the handle of this process
             */
            STACKTRACE_UNUSED explicit win_debug_frame(const void *address, const handle_ptr &handle);

            /**
             * Copy constructor
             *
             * @param f the frame to copy from
             */
            win_debug_frame(const win_debug_frame &f);

            /**
             * Move constructor
             *
             * @param f the frame to move
             */
            win_debug_frame(win_debug_frame &&f) noexcept;

            /**
             * Convert the frame to a string. Purely virtual function so frame cannot be constructed.
             *
             * @param fullPaths whether the full path names should be used
             * @return the frame as a string
             */
            STACKTRACE_NODISCARD std::string toString(bool fullPaths) const override;

            /**
             * Get the type of this frame
             *
             * @return the type of this frame. In this case, it is win_frame_type::debug
             */
            STACKTRACE_NODISCARD win_frame_type getType() const noexcept override;

            /**
             * The destructor of win_debug_frame
             */
            ~win_debug_frame() override;
        };

        /**
         * A windows release frame
         */
        class STACKTRACE_UNUSED win_release_frame : public win_frame {
        public:
            /**
             * The constructor of win_release_frame
             *
             * @param address the address of the function
             * @param handle the handle of this process
             */
            STACKTRACE_UNUSED explicit win_release_frame(const void *address, const handle_ptr &handle);

            /**
             * Copy constructor
             *
             * @param f the frame to copy from
             */
            win_release_frame(const win_release_frame &f);

            /**
             * Move constructor
             *
             * @param f the frame to move
             */
            win_release_frame(win_release_frame &&f) noexcept;

            /**
             * Convert this frame to a string
             *
             * @param fullPath a unused parameter. Just here to override the original toString function
             * @return this frame as a string
             */
            STACKTRACE_NODISCARD std::string toString(bool) const override;

            /**
             * Get the type of this frame
             *
             * @return the type of this frame. In this case, it is win_frame_type::release
             */
            STACKTRACE_NODISCARD win_frame_type getType() const noexcept override;

            /**
             * The destructor of win_release_frame
             */
            inline ~win_release_frame() override;
        };

#endif //Windows

#ifdef STACKTRACE_UNIX

        /**
         * A unix stack frame.
         * Mostly based on this: https://gist.github.com/fmela/591333/c64f4eb86037bb237862a8283df70cdfc25f01d3
         */
        class unix_frame : public frame {
        public:
            /**
             * Construct a unix_frame object
             *
             * @param address the function address
             */
            explicit unix_frame(void *address);

            /**
             * Create a unix frame from existing data
             *
             * @param function the function
             * @param fullFile the full file path
             * @param file the file
             * @param line the line
             * @param address the address
             */
            STACKTRACE_UNUSED unix_frame(const std::string &function, const std::string &fullFile,
                                         const std::string &file, size_t line, const void *address);

            /**
             * Copy constructor
             *
             * @param f the frame to copy from
             */
            unix_frame(const unix_frame &f);

            /**
             * Move constructor
             *
             * @param f the frame to move
             */
            unix_frame(unix_frame &&f) noexcept;

            /**
             * Convert this to a string
             *
             * @param fullPath whether to use full paths
             * @return this frame as a string
             */
            STACKTRACE_NODISCARD inline std::string toString(bool fullPath) const override;

        private:
#ifndef STACKTRACE_NO_ADDR2LINE
            /**
             * Try to get the file name, function name and line using the addr2line tool
             *
             * @param backtrace_sym the result from backtrace_symbols
             * @return true, if the operation was successful
             */
            bool init_using_addr2line(const char *backtrace_sym);
#else

            /**
             * Addr2line on systems without libbfd. Static because it only returns false
             * and does nothing else
             *
             * @return false. All the time.
             */
            static bool init_using_addr2line(const char *);

#endif
        };

#endif //Unix

        /**
         * The stacktrace class
         */
        class stacktrace {
        public:
            /**
             * Create a stack trace.
             * General usage: <br>
             * <code>
             * // Create a stack trace <br>
             * markusjx::stacktrace::stacktrace trace;<br>
             * <br>
             * // Print the stack trace <br>
             * std::cout &lt;&lt; trace &lt;&lt; std::endl;
             * </code><br><br>
             *
             * On linux, to use addr2line to get line numbers you can type:<br>
             * <code>
             * #define STACKTRACE_USE_ADDR2LINE<br>
             * #include &lt;stacktrace.hpp&gt;
             * </code><br><br>
             *
             * If you are not using addr2line, make sure to export the symbols of your executable.
             * Also, link against dl on linux-based systems.
             *
             * @param framesToSkip the number of frames to skip
             * @param maxFrames the max number of frames to capture
             */
            explicit stacktrace(unsigned long framesToSkip = 0, size_t maxFrames = 128);

            /**
             * Copy constructor
             *
             * @param trace the object to copy from
             */
            stacktrace(const stacktrace &trace);

            /**
             * Move constructor
             *
             * @param trace the object to move
             */
            stacktrace(stacktrace &&trace) noexcept;

            /**
             * Operator =
             *
             * @param trace the object to copy from
             * @return this
             */
            stacktrace &operator=(const stacktrace &trace);

            /**
             * Operator =
             *
             * @param trace the object to move
             * @return this
             */
            stacktrace &operator=(stacktrace &&trace) noexcept;

            /**
             * Get the frame vector
             *
             * @return a reference to the frame vector
             */
            STACKTRACE_NODISCARD STACKTRACE_UNUSED const std::vector<frame *> &getFrames() const noexcept;

            /**
             * Get a frame pointer at an index
             *
             * @param index the index of the frame pointer
             * @return the frame pointer
             */
            STACKTRACE_NODISCARD const frame *operator[](size_t index) const;

            /**
             * begin()
             *
             * @return a vector iterator
             */
            std::vector<frame *>::iterator begin() noexcept;

            /**
             * begin()
             *
             * @return a vector const iterator
             */
            STACKTRACE_NODISCARD std::vector<frame *>::const_iterator begin() const noexcept;

            /**
             * end()
             *
             * @return a vector iterator
             */
            std::vector<frame *>::iterator end() noexcept;

            /**
             * end()
             *
             * @return a vector const iterator
             */
            STACKTRACE_NODISCARD std::vector<frame *>::const_iterator end() const noexcept;

            /**
             * Get the number of frames captured
             *
             * @return the size of the frame vector
             */
            STACKTRACE_NODISCARD size_t size() const noexcept;

            /**
             * Check if the frame vector is empty
             *
             * @return true, if frames.size = 0
             */
            STACKTRACE_NODISCARD bool empty() const noexcept;

            /**
             * Operator bool
             *
             * @return true, if frames.size != 0
             */
            STACKTRACE_NODISCARD operator bool() const noexcept;

            /**
             * Dump this stack trace
             *
             * @param fullPaths whether to use full paths for file names
             * @return the dumped stack trace
             */
            STACKTRACE_NODISCARD std::string toString(bool fullPaths = false) const;

            // Operator<< for streams
            friend inline std::ostream &operator<<(std::ostream &os, const stacktrace &data) {
                os << data.toString();
                return os;
            }

            /**
             * The stacktrace destructor
             */
            ~stacktrace() noexcept;

        private:
            // A vector containing all frames
            std::vector<frame *> frames;

            /**
             * Copy all frames from another frame vector
             *
             * @param toCopyFrom the vector to copy from
             */
            void copyFrames(const std::vector<frame *> &toCopyFrom);
        };
    }
}

// Undef everything
/*#undef STACKTRACE_SLASH
#undef STACKTRACE_NODISCARD
#undef STACKTRACE_UNUSED
#undef STACKTRACE_WINDOWS
#undef STACKTRACE_UNIX
#undef STACKTRACE_CXX17*/

#endif //MARKUSJX_STACKTRACE_HPP