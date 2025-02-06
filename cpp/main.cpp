#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <optional>
#include <memory>
#include <boost/program_options.hpp>

#include "approx_mpsi.hpp"

// Command-line options
struct Options {
    size_t party_count;
    size_t set_size;
    size_t domain_size;
    size_t bin_count;
    size_t hash_count;
    double latency;
    double bytes_per_sec;
    size_t repetitions;
    std::string results_filename;
};

std::optional<Options> parse_options(int argc, char* argv[]) {
    namespace po = boost::program_options;

    Options options;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Show help message")
        ("party-count,n", po::value<size_t>(&options.party_count)->required(), "Number of parties")
        ("set-size,k", po::value<size_t>(&options.set_size)->required(), "Size of each set")
        ("domain-size,u", po::value<size_t>(&options.domain_size)->required(), "Size of the domain")
        ("bin-count,m", po::value<size_t>(&options.bin_count)->required(), "Number of bins")
        ("hash-count,s", po::value<size_t>(&options.hash_count)->required(), "Number of hash functions")
        ("latency,l", po::value<double>(&options.latency)->default_value(0.0), "Network latency in seconds")
        ("bytes-per-sec,b", po::value<double>(&options.bytes_per_sec)->default_value(0.0), "Bandwidth in bytes per second")
        ("repetitions,r", po::value<size_t>(&options.repetitions)->required(), "Number of repetitions")
        ("results-filename,f", po::value<std::string>(&options.results_filename)->required(), "Output results filename");

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

int main(int argc, char* argv[]) {
    // Parse options
    auto opt = parse_options(argc, argv);
    if (!opt) return 1;

    const auto& options = *opt;
    std::cout << "Parsed Options:\n"
              << "  Party Count: " << options.party_count << "\n"
              << "  Set Size: " << options.set_size << "\n"
              << "  Domain Size: " << options.domain_size << "\n"
              << "  Bin Count: " << options.bin_count << "\n"
              << "  Hash Count: " << options.hash_count << "\n"
              << "  Latency: " << options.latency << "\n"
              << "  Bytes per Sec: " << options.bytes_per_sec << "\n"
              << "  Repetitions: " << options.repetitions << "\n"
              << "  Results Filename: " << options.results_filename << "\n";

    if (options.domain_size < options.set_size) {
        std::cerr << "Error: Domain size must be greater than or equal to set size\n";
        return 1;
    }
    // Initialize the network description
    //FullMesh network_description = (options.latency == 0.0 && options.bytes_per_sec == 0.0)
      //                                 ? FullMesh::new_default()
        //                               : FullMesh::new_with_overhead(options.latency, options.bytes_per_sec);

    FullMesh network_description = (options.latency == 0.0 && options.bytes_per_sec == 0.0)
                                        ? FullMesh::new_default()
                                        : FullMesh(options.latency, options.bytes_per_sec, options.party_count);
    // Run the protocol
    ApproximateMpsi protocol(options.bin_count, options.hash_count, options.domain_size, options.set_size, options.results_filename);
    /*Stats stats =*/ 
    protocol.evaluate("Experiment", options.party_count, network_description, options.repetitions);

    // Output results
    //stats.output_party_csv(1); //, options.results_filename);

    return 0;
}
