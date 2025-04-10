#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <optional>
#include <memory>
#include <boost/program_options.hpp>

#include "approx_mpsi.hpp"
#include "common.hpp"

Options &g_options = *(new Options());
Stats g_stats;
// = *(new Stats(0, "results.csv"));

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
        ("hash-function,c", po::value<std::string>(&options.hash_function)->default_value("blake3_xof"), "Hash function to use")
        ("latency,l", po::value<double>(&options.latency)->default_value(0.0), "Network latency in seconds")
        ("bytes-per-sec,b", po::value<double>(&options.bytes_per_sec)->default_value(0.0), "Bandwidth in bytes per second")
        ("repetitions,r", po::value<size_t>(&options.repetitions)->required(), "Number of repetitions")
        ("results-filename,f", po::value<std::string>(&options.results_filename)->required(), "Output results filename")
        ("stats,t", po::value<bool>(&options.stats)->default_value(false), "Output stats");

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

    g_options = *opt;
    std::cout << "Parsed Options:\n"
              << "  Party Count: " << g_options.party_count << "\n"
              << "  Set Size: " << g_options.set_size << "\n"
              << "  Domain Size: " << g_options.domain_size << "\n"
              << "  Bin Count: " << g_options.bin_count << "\n"
              << "  Hash Count: " << g_options.hash_count << "\n"
              << "  Hash Function: " << g_options.hash_function << "\n"
              << "  Latency: " << g_options.latency << "\n"
              << "  Bytes per Sec: " << g_options.bytes_per_sec << "\n"
              << "  Repetitions: " << g_options.repetitions << "\n"
              << "  Results Filename: " << g_options.results_filename << "\n"
              << "  Stats: " << g_options.stats << "\n";

    if (g_options.domain_size < g_options.set_size) {
        std::cerr << "Error: Domain size must be greater than or equal to set size\n";
        return 1;
    }
    
    if (find_hash_func(g_options.hash_function) == false) {
        std::cerr << "Error: Hash function not found\n";
        return 1;
    }

    //Stats mstats = Stats(g_options.repetitions, g_options.results_filename);
    g_stats = *(new Stats(g_options.repetitions, g_options.results_filename));
    // Initialize the network description
    //FullMesh network_description = (g_options.latency == 0.0 && g_options.bytes_per_sec == 0.0)
      //                                 ? FullMesh::new_default()
        //                               : FullMesh::new_with_overhead(g_options.latency, g_options.bytes_per_sec);

    /*
    FullMesh network_description = (g_options.latency == 0.0 && g_options.bytes_per_sec == 0.0)
                                        ? FullMesh::new_default()
                                        : FullMesh(g_options.latency, g_options.bytes_per_sec, g_options.party_count);
    */
    FullMesh network_description = (g_options.latency < 0.0 || g_options.bytes_per_sec < 0.0)
                                        ? FullMesh::new_default()
                                        : FullMesh(g_options.latency, g_options.bytes_per_sec, g_options.party_count /*, g_stats*/);
    // Run the protocol
    ApproximateMpsi protocol(network_description, g_options.bin_count, g_options.hash_count, g_options.hash_function, g_options.domain_size, g_options.set_size /*, g_stats*/);
    /*Stats stats =*/ 
    protocol.evaluate("Experiment", g_options.party_count, network_description, g_options.repetitions);

    // Output results
    //stats.output_party_csv(1); //, g_options.results_filename);

    return 0;
}
