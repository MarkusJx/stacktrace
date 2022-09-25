#include "stacktrace.hpp"
#ifndef STACKTRACE_NO_ADDR2LINE
#   include "addr2lineLib/addr2line.hpp"
#endif

#include <algorithm>
#include <iomanip>

using namespace markusjx::stacktrace;

/**
 * An exception thrown when a frame couldn't be created
 */
class frameCreationException : public std::exception {
public:
    using std::exception::exception;
};

frame::frame(const void *address) : function(), fullFile(), file(), line(0), address(address) {}

frame::frame(const frame &f) : function(f.getFunction()), fullFile(f.getFullFilePath()), file(f.getFile()),
                               line(f.getLine()), address(f.getAddress()) {}

frame::frame(frame &&f) noexcept: function(std::move(f.function)), fullFile(std::move(f.fullFile)),
                                  file(std::move(f.file)), line(f.getLine()), address(f.getAddress()) {}

STACKTRACE_NODISCARD const std::string &frame::getFunction() const noexcept {
    return function;
}

STACKTRACE_NODISCARD const std::string &frame::getFullFilePath() const noexcept {
    return fullFile;
}

STACKTRACE_NODISCARD const std::string &frame::getFile() const noexcept {
    return file;
}

STACKTRACE_NODISCARD size_t frame::getLine() const noexcept {
    return line;
}

STACKTRACE_NODISCARD const void *frame::getAddress() const noexcept {
    return address;
}

frame::~frame() = default;

/**
 * Remove everything but the last part of a path. Used to get file names.
 *
 * @param str the path
 * @return the file name
 */
static inline std::string removeSlash(const std::string &str) {
    return str.substr(str.rfind(STACKTRACE_SLASH) + 1);
}

#ifdef STACKTRACE_WINDOWS
using symbol_info_ptr = std::unique_ptr<SYMBOL_INFO, decltype(&free)>;

/**
 * Get a SYMBOL_INFO as a symbol_info_ptr
 *
 * @return the resulting pointer
 */
symbol_info_ptr getSymbolInfo() {
    const size_t array_size = 256;
    const size_t size = sizeof(SYMBOL_INFO) + (array_size - 1) * sizeof(TCHAR);
    auto *symbol = (SYMBOL_INFO *) calloc(1, size);

    if (!symbol) {
        return symbol_info_ptr(nullptr, free);
    }

    symbol->SizeOfStruct = sizeof(*symbol);
    symbol->MaxNameLen = array_size;

    return symbol_info_ptr(symbol, free);
}

// win_frame ==========================

win_frame::win_frame(const void *address) : frame(address) {}

win_frame::win_frame(const win_frame &f) : frame(f) {}

win_frame::win_frame(win_frame &&f) noexcept: frame(std::move(f)) {}

// win_debug_frame ====================

STACKTRACE_UNUSED win_debug_frame::win_debug_frame(const void *address, const handle_ptr &handle) : win_frame(address) {
    symbol_info_ptr symbol = getSymbolInfo();
    if (!symbol) {
        throw frameCreationException("Unable to allocate a SYMBOL_INFO struct");
    }

    // Get the function name
    if (!SymFromAddr(handle.get(), (intptr_t) address, nullptr, symbol.get())) {
        throw frameCreationException("Unable to get the function name from the address");
    }

    function = symbol->Name;

    unsigned long dwDisplacement;
    IMAGEHLP_LINE64 line64;
    line64.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

    // Get the function line number and file name
    if (!SymGetLineFromAddr64(handle.get(), (intptr_t) address, &dwDisplacement, &line64)) {
        throw frameCreationException("Unable to get information from the address");
    }

    line = line64.LineNumber;
    fullFile = line64.FileName;
    file = removeSlash(fullFile);
}

win_debug_frame::win_debug_frame(const win_debug_frame &f) : win_frame(f) {}

win_debug_frame::win_debug_frame(win_debug_frame &&f) noexcept: win_frame(std::move(f)) {}

STACKTRACE_NODISCARD std::string win_debug_frame::toString(bool fullPaths) const {
    std::string res = getFunction();
    res.append(" in ");

    if (fullPaths) res.append(this->getFullFilePath());
    else res.append(this->getFile());

    res.append(":").append(std::to_string(this->getLine()));
    return res;
}

STACKTRACE_NODISCARD win_frame_type win_debug_frame::getType() const noexcept {
    return win_frame_type::debug;
}

win_debug_frame::~win_debug_frame() = default;

// win_release_frame ==================

// A struct for storing information about a module
struct module {
    // The module name
    const char *name;

    // The module address
    uint64_t addr;
};

/**
 * A function for comparing modules
 *
 * @param m1 the first module to compare
 * @param m2 the second module to compare to
 * @return whether m1 < m2
 */
bool module_comp(const module &m1, const module &m2) noexcept {
    return m1.addr < m2.addr;
}

/**
 * A function for enumerating modules
 */
int __stdcall enumModules(const char *name, uint64_t baseAddr, void *ctx) {
    ((std::vector<module> *) ctx)->push_back({name, baseAddr});
    return true;
}

STACKTRACE_UNUSED win_release_frame::win_release_frame(const void *address, const handle_ptr &handle) : win_frame(
        address) {
    // vec is static so it only has to be filled once.
    // It stores the modules retrieved by SymEnumerateModules64
    static std::vector<module> vec;

    // fill vec if empty
    if (vec.empty()) {
        if (!SymEnumerateModules64(handle.get(), enumModules, (void *) &vec)) {
            throw frameCreationException("Error calling SymEnumerateModules64");
        }

        // Sort vec by ascending order
        std::sort(vec.begin(), vec.end(), module_comp);
    }

    module *m = nullptr;

    // Search for a suitable address space
    for (size_t i = 0; i < vec.size(); i++) {
        if ((uint64_t) address > vec[i].addr) {
            // If the address is between two address spaces, it is in vec[i],
            // if the address is bigger than the last address space in vec,
            // we assume it is in the last module in vec, therefore, vec[i].
            // If the address is bigger than vec[i + 1] (if exists), continue.
            if (i + 1 < vec.size()) {
                if ((uint64_t) address < vec[i + 1].addr) {
                    m = &vec[i];
                    // The module the address is in was set, abort search
                    break;
                }
            } else {
                m = &vec[i];
                // The module the address is in was set, abort search
                break;
            }
        } else {
            // The address is out of the address bounds
            break;
        }
    }

    if (!m) { // The address is out of the address bounds. Throw an exception.
        throw frameCreationException("Unable to map function pointer to module");
    }

    // Try to use SymFromAddr(4) to get the function name
    symbol_info_ptr symbol = getSymbolInfo();
    if (symbol && SymFromAddr(handle.get(), (intptr_t) address, nullptr, symbol.get())) {
        function = symbol->Name;
    }

    // If SymFromAddr(4) did not return a valid function name (length <= 1), use the functions address
    // as a function name
    if (function.size() <= 1) {
        // Convert the function address to a hexadecimal string the length of intptr_t
        std::stringstream ss;
        ss << "0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(sizeof(intptr_t) * 2)
           << (intptr_t) address;

        function = ss.str();
    }

    // We use the module name as the file name, no full file path exists.
    file = m->name;
}

win_release_frame::win_release_frame(const win_release_frame &f) : win_frame(f) {}

win_release_frame::win_release_frame(win_release_frame &&f) noexcept: win_frame(std::move(f)) {}

STACKTRACE_NODISCARD std::string win_release_frame::toString(bool) const {
    std::string res = getFunction();
    res.append(" in ").append(getFile());

    return res;
}

STACKTRACE_NODISCARD win_frame_type win_release_frame::getType() const noexcept {
    return win_frame_type::release;
}

win_release_frame::~win_release_frame() = default;

// getHandle ==========================

/**
 * Get a handle of this process in a handle_ptr
 *
 * @return the handle_ptr
 */
handle_ptr getHandle() {
    handle_ptr handle(GetCurrentProcess(), CloseHandle);

    // Call SymInitialize
    if (!SymInitialize(handle.get(), nullptr, true)) {
        handle.reset();
    }

    return handle;
}

#endif //Windows

#ifdef STACKTRACE_UNIX

// unix_frame =========================
unix_frame::unix_frame(void *address) : frame(address) {
    // Get the symbols using backtrace_symbols(2)
    char **symbols = backtrace_symbols(&address, 1);

    if (!init_using_addr2line(symbols[0])) {
        // Init using addr2line failed.
        // Try to use dladdr to get function name
        Dl_info dli;
        bool dladdr_ok = dladdr(address, &dli);

        // If dladdr returned a valid function name, use it
        if (dladdr_ok && dli.dli_sname) {
            // Demangle the function name
            int status = -1;
            char *demangled = abi::__cxa_demangle(dli.dli_sname, nullptr, nullptr, &status);
            if (status == 0) {
                function = demangled;
            } else {
                function = dli.dli_sname;
            }

            // Set the file name
            fullFile = dli.dli_fname;
            file = removeSlash(fullFile);
            free(demangled);
        } else if (dladdr_ok && dli.dli_fname) { // dladdr failed to get the function name
            // dladdr was able to get the file name, use it
            std::stringstream ss;
            ss << "0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(sizeof(intptr_t) * 2)
               << (intptr_t) address;

            function = ss.str();
            fullFile = dli.dli_fname;
            file = removeSlash(fullFile);
        } else {
            // dladdr failed, fall back to backtrace_symbols(2)
            function = symbols[0];
        }
    }
    free(symbols);
}

unix_frame::unix_frame(const std::string &function, const std::string &fullFile, const std::string &file, size_t line,
                       const void *address) : frame(address) {
    this->function = function;
    this->fullFile = fullFile;
    this->file = file;
    this->line = line;
}

/**
 * Copy constructor
 *
 * @param f the frame to copy from
 */
unix_frame::unix_frame(const unix_frame &f) = default;

/**
 * Move constructor
 *
 * @param f the frame to move
 */
unix_frame::unix_frame(unix_frame &&f) noexcept: frame(std::move(f)) {}

/**
 * Convert this to a string
 *
 * @param fullPath whether to use full paths
 * @return this frame as a string
 */
STACKTRACE_NODISCARD std::string unix_frame::toString(bool fullPath) const {
    if (!file.empty()) {
        std::string res = function;
        res.append(" in ");
        if (fullPath) res.append(fullFile);
        else res.append(file);

        if (line != 0) res.append(":").append(std::to_string(line));

        return res;
    } else {
        return function;
    }
}

bool unix_frame::init_using_addr2line(STACKTRACE_UNUSED const char *backtrace_sym) {
#ifndef STACKTRACE_NO_ADDR2LINE
    set_options(true, true, true, nullptr);

    addr2line::addr2line_res res = addr2line::processAddress(backtrace_sym);
    if (res.status == 0 && !res.info.empty()) {
        this->function = res.info[0].name;
        this->fullFile = res.info[0].filename;
        this->file = res.info[0].basename;
        this->line = res.info[0].line;

        return !function.empty() && !file.empty() && !fullFile.empty();
    } else {
        return false;
    }
#else
    return false;
#endif
}

#endif //Unix

// stacktrace =========================

stacktrace::stacktrace(STACKTRACE_UNUSED unsigned long framesToSkip, size_t maxFrames) : frames() {
    std::vector<void *> raw_frames(maxFrames, nullptr);
#ifdef STACKTRACE_WINDOWS
    size_t captured = ::RtlCaptureStackBackTrace(framesToSkip, (u_long) raw_frames.size(),
                                                 raw_frames.data(),
                                                 nullptr);

    SymSetOptions(SYMOPT_LOAD_LINES);

    // A handle for windows. Static to be used by every stacktrace object.
    // Will be created if it does not exist.
    static handle_ptr handle;
    if (!handle) {
        handle = getHandle();
    }

    raw_frames.resize(captured);
#else
    size_t captured = backtrace(raw_frames.data(), raw_frames.size());

    raw_frames.resize(captured);

    for (void *ptr : raw_frames) {
        if (ptr) {
            try {
                frames.push_back(new unix_frame(ptr));
            } catch (...) {
                // Ignore
            }
        } else {
            break;
        }
    }
#endif //Windows

#ifdef STACKTRACE_WINDOWS
#ifndef NDEBUG // Don't even try to use *_debug_frame in release builds
    for (void *ptr : raw_frames) {
        if (ptr) {
            try {
                frames.push_back(new win_debug_frame(ptr, handle));
            } catch (std::exception &e) {
#ifdef STACKTRACE_SHOW_ERRORS // Print errors if requested
                std::cerr << "[stacktrace.hpp:" << __LINE__ << "] Exception thrown: " << e.what() << std::endl;
#elif defined(UNREFERENCED_PARAMETER)
                UNREFERENCED_PARAMETER(e);
#endif //SHOW_ERRORS
            } catch (...) {
                // Ignore
            }
        } else {
            // ptr = nullptr, all frames were converted
            break;
        }
    }
#endif //Debug

    // If frames is empty, use win_release_frame
    if (frames.empty()) {
        for (void *ptr : raw_frames) {
            if (ptr) {
                try {
                    frames.push_back(new win_release_frame(ptr, handle));
                } catch (std::exception &e) {
#ifdef STACKTRACE_SHOW_ERRORS // Print errors if requested
                    std::cerr << "[stacktrace.hpp:" << __LINE__ << "] Exception thrown: " << e.what() << std::endl;
#elif defined(UNREFERENCED_PARAMETER)
                    UNREFERENCED_PARAMETER(e);
#endif //SHOW_ERRORS
                } catch (...) {
                    // Ignore
                }
            } else {
                // ptr = nullptr, all frames were converted
                break;
            }
        }
    }
#endif //Windows
}

stacktrace::stacktrace(const stacktrace &trace) : frames() {
    copyFrames(trace.getFrames());
}

stacktrace::stacktrace(stacktrace &&trace) noexcept: frames(std::move(trace.frames)) {}

stacktrace &stacktrace::operator=(const stacktrace &trace) {
    copyFrames(trace.getFrames());
    return *this;
}

stacktrace &stacktrace::operator=(stacktrace &&trace) noexcept {
    frames = std::move(trace.frames);
    return *this;
}

STACKTRACE_NODISCARD STACKTRACE_UNUSED const std::vector<frame *> &stacktrace::getFrames() const noexcept {
    return frames;
}

STACKTRACE_NODISCARD const frame *stacktrace::operator[](size_t index) const {
    return frames.at(index);
}

std::vector<frame *>::iterator stacktrace::begin() noexcept {
    return frames.begin();
}

STACKTRACE_NODISCARD std::vector<frame *>::const_iterator stacktrace::begin() const noexcept {
    return frames.begin();
}

std::vector<frame *>::iterator stacktrace::end() noexcept {
    return frames.end();
}

STACKTRACE_NODISCARD std::vector<frame *>::const_iterator stacktrace::end() const noexcept {
    return frames.end();
}

STACKTRACE_NODISCARD size_t stacktrace::size() const noexcept {
    return frames.size();
}

STACKTRACE_NODISCARD bool stacktrace::empty() const noexcept {
    return frames.empty();
}

STACKTRACE_NODISCARD stacktrace::operator bool() const noexcept {
    return !frames.empty();
}

STACKTRACE_NODISCARD std::string stacktrace::toString(bool fullPaths) const {
    std::stringstream ss;
    for (size_t i = 0; i < frames.size(); i++) {
        ss << " " << i << "# " << frames[i]->toString(fullPaths) << std::endl;
    }

    return ss.str();
}

stacktrace::~stacktrace() noexcept {
    // Delete all frames
    for (const auto &p : frames) {
        delete p;
    }
}

void stacktrace::copyFrames(const std::vector<frame *> &toCopyFrom) {
    if (!frames.empty()) {
        for (frame *ptr : frames) delete ptr;
        frames = std::vector<frame *>();
    }

    frames.reserve(toCopyFrom.size());

    // Copy the frames depending on the os
    for (frame *f : toCopyFrom) {
#ifdef STACKTRACE_WINDOWS
#   ifdef NDEBUG
        frames.push_back(new win_release_frame(*(win_release_frame *) f));
#   else
        if (((win_frame *) f)->getType() == win_frame_type::debug) {
            frames.push_back(new win_debug_frame(*(win_debug_frame *) f));
        } else {
            frames.push_back(new win_release_frame(*(win_release_frame *) f));
        }
#   endif //Release
#else
        frames.push_back(new unix_frame(*(unix_frame *) f));
#endif //Windows
    }
}