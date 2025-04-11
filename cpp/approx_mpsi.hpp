#ifndef APPROXIMATE_MPSI_HPP
#define APPROXIMATE_MPSI_HPP

#include <vector>
#include <optional>
#include <iostream>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <cstdint>
#include "FullMesh.hpp"
#include "Stats.hpp"
#include "Channels.hpp"
#include "secret_sharing_simd.hpp"
#include "Set.hpp"
#include "hash_funcs.hpp"

// Constants
//constexpr size_t SHARE_BYTE_COUNT = 5;

typedef struct thread_data {
    size_t id;
    bool is_completed;
    std::optional<Set> output;
};

class Party {
public:
    virtual ~Party() = default;
    virtual std::optional<Set> run(size_t id, size_t n_parties, const std::optional<Set>& input,
                                    Channels& channels, thread_data* th_data) = 0;
};

// ApproximateMpsi class: High-level protocol definition
class ApproximateMpsi {
public:
    // Constructor
    ApproximateMpsi(FullMesh& net, size_t bin_count, size_t hash_count, std::string hash_func, size_t domain_size, size_t set_size /*, Stats& stats*/);

    std::vector<Set> gen_sets_with_uniform_intersection(size_t n_parties, size_t set_size, size_t domain_size);
    std::vector<std::optional<Set>> generate_inputs(size_t n_parties) /*const*/;
    bool validate_outputs(const std::vector<std::optional<Set>>& inputs,
                            const std::vector<std::optional<Set>>& outputs) /*const*/;
    // Main evaluation function
    /*Stats*/ 
    void evaluate(const std::string& experiment_name, size_t party_count,
                                const FullMesh& network_description, size_t repetitions);
    std::vector<std::unique_ptr<Party>> setup_parties(size_t n_parties);
    std::vector<std::unique_ptr<Party>> setup_parties2(size_t n_parties, size_t seeds_sz_factor);

private:
    size_t bin_count;
    size_t hash_count;
    std::string hash_func;
    size_t domain_size;
    size_t set_size;
    //Stats& stats;
    FullMesh& network;
};

// ApproximateMpsiParty class: Represents a single party in the protocol
class ApproximateMpsiParty : public Party {
public:
    // Constructor
    ApproximateMpsiParty(FullMesh& net, std::vector<std::array<uint8_t, RAND_SECRET_SIZE>> seeds, size_t bin_count, size_t hash_count, std::string hash_func /*, Stats& stats*/);

    // Public interface
    //std::optional<Set> run(size_t id, size_t n_parties, const std::optional<Set>& input, 
    //                       Channels& channels);
    std::optional<Set> run(size_t id, size_t n_parties, const Input& input, Channels& channels, thread_data* th_data);
private:
    std::vector<std::array<uint8_t, RAND_SECRET_SIZE>> seeds;
    size_t bin_count;
    size_t hash_count;
    std::string hash_func;
    //Stats& stats;
    FullMesh& network;

    // Internal functions
    /*void run_server_approx(size_t n_parties, Channels& channels);
    std::optional<Set> run_querier_approx(const Set& input, Channels& channels);
    void run_client_approx(const Set& input, Channels& channels);
   */
    std::vector<bool> compute_query_results(size_t id, const std::vector<std::vector<size_t>>& query_patterns, 
    const SimdBytes& aggregated_share);
    std::vector<std::vector<size_t>> generate_query_patterns(const Set& input);
    Set extract_intersection(const Set& input, const std::vector<bool>& results);
    //std::vector<size_t> bloom_filter_indices(const size_t element, size_t bin_count, size_t hash_count);

    void run_server_approx(size_t id, size_t n_parties, Channels& channels);
    Set run_querier_approx(size_t id, const Set& input, Channels& channels);
    void run_client_approx(size_t id, const Set& input, Channels& channels);
};

#endif // APPROXIMATE_MPSI_HPP
