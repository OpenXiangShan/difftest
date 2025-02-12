/* Author: baichen.bai@alibaba-inc.com */

#ifndef __UTILS_H__
#define __UTILS_H__


#include <map>
#include <deque>
#include <stack>
#include <vector>
#include <string>
#include <chrono>
#include <limits>
#include <cfloat>
#include <thread>
#include <numeric>
#include <cstdlib>
#include <ostream>
#include <fstream>
#include <utility>
#include <cassert>
#include <dlfcn.h>
#include <cxxabi.h>
#include <getopt.h>
#include <unistd.h>
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/classification.hpp>
#include "logger.h"
#include "deg_defs.h"
#include "ppk_assert.h"


#define FP_MAX std::numeric_limits<double>::max()
#define DEG_UNREACHABLE __builtin_unreachable() // the macro could incure segmentation fault
#define NAME(x) (#x)
#define INFO(...) do {                       \
    get_logger()->log("INFO", __VA_ARGS__);  \
} while (0)
#define WARN(...) do {                       \
    get_logger()->log("WARN", __VA_ARGS__);  \
} while (0)
#define ERROR(...) do {                      \
    get_logger()->log("ERROR", __VA_ARGS__); \
    std::exit(EXIT_FAILURE);                 \
} while (0)
#define TEST(...) do {                       \
    get_logger()->log("TEST", __VA_ARGS__);  \
} while (0)


/*
 * Timer related helplers
 */
template <class Duration>
using sys_time = std::chrono::time_point<std::chrono::system_clock, Duration>;
using sys_nanoseconds = sys_time<std::chrono::nanoseconds>;

struct Timer
{
    std::string fn, title;
    std::chrono::time_point<std::chrono::steady_clock> start;
    Timer(std::string fn, std::string title):
        fn(std::move(fn)),
        title(std::move(title)),
        start(std::chrono::steady_clock::now()) {}

    ~Timer()
    {
        const auto elapsed =
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - start
            ).count();
        std::cout << "[INFO]: " << title.c_str() << \
            ": elapsed = " << elapsed / 1000.0 << \
            " ms." << std::endl;
    }
};

#ifndef TIMER_BENCHMARK
static constexpr inline void dummy_fn() { }
#define SETUP_BENCHMARK_TIMER(...) dummy_fn()
#else
#define SETUP_BENCHMARK_TIMER(title) Timer timer(__FUNCTION__, title)
#endif

// `fn` cannot be a member function
template<typename F>
auto timer_fn(F&& fn) {
    Dl_info info;
    dladdr(reinterpret_cast<void *>(&fn), &info);
    SETUP_BENCHMARK_TIMER(abi::__cxa_demangle(
        info.dli_sname, NULL, NULL, NULL)
    );
    return fn();
}


// `fn` cannot be a member function
template<typename F, typename ...Args>
auto timer_fn(F&& fn, Args&&... args) {
    Dl_info info;
    dladdr(reinterpret_cast<void *>(&fn), &info);
    SETUP_BENCHMARK_TIMER(abi::__cxa_demangle(
        info.dli_sname, NULL, NULL, NULL)
    );
    return fn(std::forward<Args>(args)...);
}


/*
 * Options related variables
 */
static struct option long_options[] = {
    {"help", no_argument, NULL, 'h'},
    {"trace", required_argument, NULL, 'c'},
    {"output", required_argument, NULL, 'o'},
    {"view", optional_argument, NULL, 'v'},
    {"start", optional_argument, NULL, 's'},
    {"end", optional_argument, NULL, 'e'}
};


std::unordered_map<std::string, std::string>
deg_parse_args(int, const char **) noexcept;
void help();
sys_nanoseconds timestamp();
bool if_dir_exist(const std::string&);
bool if_file_exist(const std::string&);
bool create_dir(const std::string&);
bool create_file(const std::string&);
std::string add_string_to_file_name(const std::string&, const std::string&);
std::string get_dir_path(const std::string&);
std::string get_file_name(const std::string&);
std::string get_plain_file_name(const std::string&);
void split(std::vector<std::string>&, std::string, const std::string&) noexcept;
std::string remove_suffix(std::string&, std::string&) noexcept;
bool starts_with(const std::string&, const std::string&) noexcept;
std::string& lstrip(std::string&);
std::string& rstrip(std::string&);
std::string& strip(std::string&);
latency accumulate(std::vector<latency>);

template <typename Enum>
constexpr typename std::underlying_type<Enum>::type
enum_to_underlying(Enum e) noexcept {
    return static_cast<typename std::underlying_type<Enum>::type>(e);
}


#endif
