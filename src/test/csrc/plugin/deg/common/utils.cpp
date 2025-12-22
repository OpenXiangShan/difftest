/* Author: baichen.bai@alibaba-inc.com */


#include "deg/utils.h"


void help() {
    INFO("New DEG usage: $ ./deg [-h] -t TRACE -o OUTPUT [-v] [-s START] [-e END] \n \
    options:                                                                      \n \
        -h, --help            show this help message and exit                     \n \
        -t TRACE, --trace TRACE                                                   \n \
                              trace file specification (default: trace)           \n \
        -o OUTPUT, --output OUTPUT                                                \n \
                              report specification (default: report)              \n \
        -v, --view            view the new DEG (default: False)                   \n \
        -s START, --start START                                                   \n \
                              instruction start index (default: -1)               \n \
        -e END, --end END     instruction end intex (default: -1)                 \n"
    );
}

std::unordered_map<std::string, std::string>
deg_parse_args(int argc, const char **argv) noexcept {
    int index = 0, c = 0;
    std::unordered_map<std::string, std::string> args;

    auto msg = "./deg [-h] -t TRACE " \
        "-o OUTPUT " \
        "[-v] " \
        "[-s START] " \
        "[-e END]";

    while ((
        c = getopt_long(
            argc,
            const_cast<char **>(argv),
            "ht:o:vs:e:",
            long_options,
            &index)
        ) != EOF
    ) {
        switch(c) {
            case 'h':
                help();
                std::exit(EXIT_SUCCESS);
            case 't':
                args["trace"] = optarg;
                break;
            case 'o':
                args["output"] = optarg;
                break;
            case 'v':
                args["view"] = "view";
                break;
            case 's':
                args["start"] = optarg;
                break;
            case 'e':
                args["end"] = optarg;
                break;
            case '?':
            default:
                ERROR("$ %s\n", msg);
        }
    }

    #ifdef DEBUG_PARSE_ARGS
        TEST("debug parse args: \n");
        for (auto iter = args.begin(); iter != args.end(); iter++) {
            TEST("%s: %s\n", iter->first, iter->second);
        }
        std::cout << std::endl;
    #endif

    return args;
}


sys_nanoseconds timestamp() {
    return std::chrono::system_clock::now();
}


bool if_dir_exist(const std::string& path_name) {
    // if (std::filesystem::exists(path_name) && \
    //     std::filesystem::is_directory(path_name))
    if (boost::filesystem::exists(path_name) && \
        boost::filesystem::is_directory(path_name))
        return true;
    return false;
}


bool if_file_exist(const std::string& file_name) {
    // if (std::filesystem::exists(file_name) && \
    //     std::filesystem::is_regular_file(file_name))
    if (boost::filesystem::exists(file_name) && \
        boost::filesystem::is_regular_file(file_name))
        return true;
    return false;
}


bool create_dir(const std::string& path_name) {
    if (if_dir_exist(path_name))
        return true;
    // return std::filesystem::create_directories(path_name);
    return boost::filesystem::create_directories(path_name);
}


bool create_file(const std::string& file_name) {
    if (if_file_exist(file_name))
        return true;
    std::ofstream file(file_name);
    file.close();
    return if_file_exist(file_name);
}


std::string add_string_to_file_name(
    const std::string& file_name,
    const std::string& pattern
) {
    // std::filesystem::path name(file_name);
    boost::filesystem::path name(file_name);
    std::string extension = name.extension().string(), \
        s_name = name.string();
    auto s = remove_suffix(s_name, extension);
    return s + pattern + extension;
}


std::string get_dir_path(const std::string& path) {
    boost::filesystem::path p(path);
    return p.parent_path().string();
}


std::string get_file_name(const std::string& path) {
    // return file.extension
    boost::filesystem::path p(path);
    return p.filename().string();
}


std::string get_plain_file_name(const std::string& path) {
    // return file.extension
    boost::filesystem::path p(path);
    return p.stem().string();
}


void split(
    std::vector<std::string>& segments,
    std::string line,
    const std::string& separator
) noexcept {
    boost::split(
        segments,
        line,
        boost::is_any_of(separator),
        boost::token_compress_on
    );
}


std::string remove_suffix(std::string& s, std::string& sub) noexcept {
    return s.assign(s.begin(), s.end() - sub.length());
}


bool starts_with(const std::string& s, const std::string& p) noexcept {
    return boost::starts_with(s, p);
}


std::string& lstrip(std::string& s) {
    auto iter = std::find_if(s.begin(), s.end(), [](char c) {
            return !std::isspace<char>(c, std::locale::classic());
        }
    );
    s.erase(s.begin(), iter);
    return s;
}


std::string& rstrip(std::string& s) {
    auto iter = std::find_if(s.rbegin(), s.rend(), [](char c) {
            return !std::isspace<char>(c, std::locale::classic());
        }
    );
    s.erase(iter.base(), s.end());
    return s;
}


std::string& strip(std::string& s) {
    return lstrip(rstrip(s));
}


latency accumulate(std::vector<latency> v) {
    return std::accumulate(
        v.begin(),
        v.end(),
        decltype(v)::value_type(0)
    );
}
