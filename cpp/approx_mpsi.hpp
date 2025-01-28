#ifndef APPROXIMATE_MPSI_HPP
#define APPROXIMATE_MPSI_HPP

#include <vector>
#include <optional>
#include <iostream>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include "FullMesh.hpp"
#include "Stats.hpp"

// Constants
constexpr size_t SHARE_BYTE_COUNT = 5;

// Forward Declarations
class Set;
class Channels;

// ApproximateMpsi class: High-level protocol definition
class ApproximateMpsi {
public:
    // Constructor
    ApproximateMpsi(size_t bin_count, size_t hash_count, size_t domain_size, size_t set_size);

    // Main evaluation function
    Stats evaluate(const std::string& experiment_name, size_t party_count,
                   const FullMesh& network_description, size_t repetitions);

private:
    size_t bin_count;
    size_t hash_count;
    size_t domain_size;
    size_t set_size;
};

// ApproximateMpsiParty class: Represents a single party in the protocol
class ApproximateMpsiParty {
public:
    // Constructor
    ApproximateMpsiParty(std::vector<std::array<uint8_t, 16>> seeds, size_t bin_count, size_t hash_count);

    // Public interface
    std::optional<Set> run(size_t id, size_t n_parties, const std::optional<Set>& input, 
                           Channels& channels);

private:
    std::vector<std::array<uint8_t, 16>> seeds;
    size_t bin_count;
    size_t hash_count;

    // Internal functions
    void run_server_approx(size_t n_parties, Channels& channels);
    Set run_querier_approx(const Set& input, Channels& channels);
    void run_client_approx(const Set& input, Channels& channels);
};

#endif // APPROXIMATE_MPSI_HPP
