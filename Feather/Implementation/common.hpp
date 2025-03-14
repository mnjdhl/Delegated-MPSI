#ifndef COMMON_HPP
#define COMMON_HPP
#include <optional>
#include <string>
#include <boost/program_options.hpp>

// Command-line options
struct Options {
    /*size_t party_count;
    size_t set_size;
    size_t domain_size;
    size_t bin_count;
    size_t hash_count;
    std::string hash_function;
    double latency;
    double bytes_per_sec;
    size_t repetitions;
    std::string results_filename;
    bool stats;*/
    int number_of_experiments;
    int number_of_clients;
    int pub_mod_bitsize;
    int max_setsize;
    int table_length;
    int bucket_max_load;
    int interSec_size;
    int xsize;
};

extern Options &g_options;

std::optional<Options> parse_options(int argc, char* argv[]) {
    namespace po = boost::program_options;

    Options options;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Show help message")
        ("number-of-experiments,n", po::value<int>(&options.number_of_experiments)->default_value(1), "Number of experiments")
        ("number-of-clients,c", po::value<int>(&options.number_of_clients)->default_value(2), "Number of clients")
        ("pub-mod-bitsize,u", po::value<int>(&options.pub_mod_bitsize)->default_value(40), "Public mod bitsize")
        ("max-setsize,k", po::value<int>(&options.max_setsize)->default_value(1024), "Max set size")
        ("table-length,m", po::value<int>(&options.table_length)->default_value(30), "Table length")
        ("bucket-max-load,s", po::value<int>(&options.bucket_max_load)->default_value(100), "Bucket max load")
        ("intersection-size,i", po::value<int>(&options.interSec_size)->default_value(1), "Intersection size")
        ("xsize,x", po::value<int>(&options.xsize)->default_value(201), "X size");

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help")) {
            std::cout << desc << "\n";
            return std::nullopt;
        }

        po::notify(vm);
    } catch (const po::error& e) {
        std::cerr << "Error: " << e.what() << "\n";
        std::cerr << desc << "\n";
        return std::nullopt;
    }

    return options;
}

bool validate_options(Options &g_opts) {
    if (g_opts.interSec_size > g_opts.max_setsize) {
        std::cerr << "Error: Intersection size must be less than or equal to max set size\n";
        return false;
    }
    if (g_opts.xsize < (2 * g_opts.bucket_max_load) + 1) {
        std::cerr << "Error: X size must be greater than 2 * bucket max load + 1\n";
        return false;
    }
    return true;
}

void display_options(struct Options &g_opts) {
    std::cout << "Parsed Options:\n"
              << "Number of experiments: " << g_opts.number_of_experiments << "\n"
              << "Number of clients: " << g_opts.number_of_clients << "\n"
              << "Public mod bitsize: " << g_opts.pub_mod_bitsize << "\n"
              << "Max set size: " << g_opts.max_setsize << "\n"
              << "Table length: " << g_opts.table_length << "\n"
              << "Bucket max load: " << g_opts.bucket_max_load << "\n"
              << "Intersection size: " << g_opts.interSec_size << "\n"
              << "X size: " << g_opts.xsize << "\n";
}   
#endif // COMMON_HPP