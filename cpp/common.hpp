#ifndef COMMON_HPP
#define COMMON_HPP

#include <string>

// Command-line options
struct Options {
    size_t party_count;
    size_t set_size;
    size_t domain_size;
    size_t bin_count;
    size_t hash_count;
    std::string hash_function;
    double latency;
    double bytes_per_sec;
    size_t repetitions;
    std::string results_filename;
    bool stats;
};

extern Options &g_options;
#endif // COMMON_HPP