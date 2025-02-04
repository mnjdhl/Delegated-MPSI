#include <vector>
#include <array>
#include <bitset>
#include <numeric>
#include <random>
#include <algorithm>
#include <iterator>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <iostream>
#include <cassert>
#include <memory>
#include "approx_mpsi.hpp"

/* Method Definitions for 'Set' class */
Set::Set() = default;

Set::Set(std::initializer_list<size_t> init) : elements(init) {}

Set::Set(std::unordered_set<size_t> elems) : elements(std::move(elems)) {}

Set Set::intersection(const std::vector<Set>& sets) {
    if (sets.empty()) return {};
    Set result = sets.front();
    for (const auto& set : sets) {
        std::unordered_set<size_t> temp;
        for (size_t elem : result.elements) {
            if (set.elements.count(elem)) temp.insert(elem);
        }
        result.elements = std::move(temp);
    }
    return result;
}

std::vector<size_t> Set::to_vector() const {
    return {elements.begin(), elements.end()};
}

bool Set::operator==(const Set& other) const {
    return elements == other.elements;
}

std::unordered_set<size_t> Set::get_elements() const {
    return elements;
}

std::vector<size_t> Set::bloom_filter_indices(const size_t element, 
    size_t bin_count, size_t hash_count) {
    std::vector<size_t> indices;
    std::hash<size_t> hasher;

    for (size_t i = 0; i < hash_count; i++) {
        indices.push_back((hasher(element + i) % bin_count));
    }

    return indices;
}

void Set::insert(size_t element) {
    elements.insert(element);
}

// Convert the set into a Bloom filter representation
std::vector<bool> Set::to_bloom_filter(size_t bin_count, size_t hash_count) const {
    std::vector<bool> bloom_filter(bin_count, false);

    for (size_t element : elements) {
        auto indices = Set::bloom_filter_indices(element, bin_count, hash_count);
        for (size_t idx : indices) {
            bloom_filter[idx] = true;
        }
    }

    return bloom_filter;
}

/* Method Definitions for 'ApproximateMpsi' class */
ApproximateMpsi::ApproximateMpsi(size_t minimum_bin_count, size_t hash_count, size_t domain_size, size_t set_size)
        : bin_count((minimum_bin_count + 63) / 64 * 64),
          hash_count(hash_count),
          domain_size(domain_size),
          set_size(set_size) {

          }

std::vector<std::unique_ptr<Party>> ApproximateMpsi::setup_parties(size_t n_parties) {
    std::vector<std::vector<std::array<uint8_t, 16>>> party_seeds(n_parties - 1,
                                                                    std::vector<std::array<uint8_t, 16>>(n_parties - 1));
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint8_t> dist(0, 255);

    for (size_t i = 1; i < n_parties; ++i) {
        for (size_t j = i + 1; j < n_parties; ++j) {
            std::array<uint8_t, 16> seed{};
            for (auto& byte : seed) byte = dist(gen);

            party_seeds[i - 1][j - 1] = seed;
            party_seeds[j - 1][i - 1] = seed;
        }
    }

    party_seeds.insert(party_seeds.begin(), {});

    std::vector<std::unique_ptr<Party>> parties;
    for (const auto& seeds : party_seeds) {
        parties.push_back(std::make_unique<ApproximateMpsiParty>(seeds, bin_count, hash_count));
    }
    return parties;
}

std::vector<Set> ApproximateMpsi::gen_sets_with_uniform_intersection(size_t n_parties, size_t set_size, size_t domain_size) {
    std::vector<Set> sets;
    std::unordered_set<size_t> common_elements;
    std::mt19937 rng(std::random_device{}());

    // Step 1: Generate a common subset of elements
    size_t common_size = set_size / 2; // Half the set will be common
    while (common_elements.size() < common_size) {
        common_elements.insert(rng() % domain_size);
    }

    // Step 2: Generate unique sets for each party
    for (size_t i = 0; i < n_parties; ++i) {
        std::unordered_set<size_t> unique_elements = common_elements;

        while (unique_elements.size() < set_size) {
            unique_elements.insert(rng() % domain_size);
        }

        sets.emplace_back(Set(unique_elements));
    }

    return sets;
}

std::vector<std::optional<Set>> ApproximateMpsi::generate_inputs(size_t n_parties) /*const*/ {
    std::vector<std::optional<Set>> inputs;
    inputs.emplace_back(std::nullopt); // First element (None in Rust)

    // Generate sets with uniform intersection
    auto sets = gen_sets_with_uniform_intersection(n_parties, set_size, domain_size);
    for (auto& set : sets) {
        inputs.emplace_back(std::move(set));
    }

    return inputs;
}

bool ApproximateMpsi::validate_outputs(
    const std::vector<std::optional<Set>>& inputs,
    const std::vector<std::optional<Set>>& outputs) /*const*/ {
    
    // Compute expected intersection of all input sets (excluding the first element)
    std::vector<Set> input_sets;
    for (size_t i = 1; i < inputs.size(); ++i) {
        if (inputs[i].has_value()) {
            input_sets.push_back(inputs[i].value());
        }
    }
    Set expected_intersection = Set::intersection(input_sets);

    // Extract the protocol's output from the querying party (index 1)
    if (!outputs[1].has_value()) {
        return false; // Querying party must have an output
    }
    Set actual_intersection = outputs[1].value();

    return expected_intersection == actual_intersection;
}

/*
Stats ApproximateMpsi::evaluate(
    const std::string& experiment_name,
    size_t party_count,
    const FullMesh& network_description,
    size_t repetitions,
    const std::string& results_filename
) {
    std::cout << "Running experiment: " << experiment_name << " with " << party_count << " parties...\n";

    Stats stats(repetitions, results_filename);

    for (size_t rep = 0; rep < repetitions; ++rep) {
        std::cout << "Repetition " << rep + 1 << " / " << repetitions << std::endl;

        // Step 1: Setup parties
        //std::vector<ApproximateMpsiParty>
        auto parties = setup_parties(party_count);

        // Step 2: Generate inputs for each party
        std::vector<std::optional<Set>> inputs = generate_inputs(party_count);

        // Step 3: Start timing
        auto start_time = std::chrono::high_resolution_clock::now();

        // Step 4: Run protocol
        std::vector<std::optional<Set>> outputs(party_count);
        #pragma omp parallel for
        for (size_t i = 0; i < party_count; ++i) {
            outputs[i] = parties[i]->run(i, party_count, inputs[i], network_description);
        }

        // Step 5: Stop timing
        auto end_time = std::chrono::high_resolution_clock::now();
        double elapsed_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

        // Step 6: Validate outputs
        bool valid = validate_outputs(inputs, outputs);

        // Step 7: Log results
        stats.log_result(rep + 1, elapsed_ms, valid);

        std::cout << "Run " << rep + 1 << ": " << (valid ? "Success" : "Failure")
                  << " | Time: " << elapsed_ms << " ms" << std::endl;
    }

    // Print and return stats
    stats.print_summary();
    return {stats.get_average_time(), stats.was_successful()};
}*/

Stats ApproximateMpsi::evaluate(const std::string& experiment_name, size_t party_count,
                                const FullMesh& network_description, size_t repetitions,
                                const std::string& results_filename) {
    Stats stats(results_filename); //experiment_name + ".csv");  // Initialize Stats to log CSV output

    for (size_t i = 0; i < repetitions; ++i) {
        std::cout << "Running repetition " << (i + 1) << " of " << repetitions << "...\n";

        // Step 1: Generate inputs (randomized sets with uniform intersection)
        auto inputs = generate_inputs(party_count);

        // Step 2: Set up network communication (FullMesh)
        const FullMesh& network = network_description;

        // Step 3: Set up parties
        auto parties = setup_parties(party_count);

        // Step 4: Run protocol for all parties
        std::vector<std::optional<Set>> outputs(party_count);
        for (size_t id = 0; id < party_count; ++id) {
            outputs[id] = parties[id]->run(id, party_count, inputs[id], network.get_channels(id), stats);
        }

        // Step 5: Validate outputs
        bool success = validate_outputs(inputs, outputs);
        stats.log_experiment(i + 1, success);  // Log success/failure to CSV
    }

    return stats;
}

/* Method Definitions for 'ApproximateMpsiParty' class */
ApproximateMpsiParty::ApproximateMpsiParty(std::vector<std::array<uint8_t, 16>> seeds, size_t bin_count, size_t hash_count)
    : seeds(std::move(seeds)), bin_count(bin_count), hash_count(hash_count) {}

/*
void ApproximateMpsiParty::run_server_approx(size_t n_parties, Channels& channels) {
    // Receive shares
    std::vector<SimdBytes> received_shares;
    for (size_t i = 1; i < n_parties; ++i) {
        auto received_data = channels.receive(i);
        received_shares.push_back(SimdBytes::from_bytes(received_data));
    }

    // Aggregate shares
    SimdBytes aggregated_share = received_shares[0];
    for (size_t i = 1; i < received_shares.size(); ++i) {
        aggregated_share ^= received_shares[i];
    }

    // Process query patterns
    std::vector<uint8_t> query_data = channels.receive(1);
    std::vector<std::vector<size_t>> query_patterns; // Deserialize query patterns here

    std::vector<std::array<uint8_t, SHARE_BYTE_COUNT>> shares;
    for (size_t i = 0; i < aggregated_share.to_bytes().size(); i += SHARE_BYTE_COUNT) {
        std::array<uint8_t, SHARE_BYTE_COUNT> chunk{};
        std::copy_n(aggregated_share.to_bytes().begin() + i, SHARE_BYTE_COUNT, chunk.begin());
        shares.push_back(chunk);
    }

    std::vector<bool> results;
    for (const auto& query_pattern : query_patterns) {
        std::array<uint8_t, SHARE_BYTE_COUNT> xor_result{};
        for (size_t index : query_pattern) {
            for (size_t j = 0; j < SHARE_BYTE_COUNT; ++j) {
                xor_result[j] ^= shares[index][j];
            }
        }
        results.push_back(std::all_of(xor_result.begin(), xor_result.end(),
                                        [](uint8_t b) { return b == 0; }));
    }

    // Send results
    // Serialize results and send to querier (id = 1)
}

std::optional<Set> ApproximateMpsiParty::run_querier_approx(const Set& input, Channels& channels) {
    run_client_approx(input, channels);

    // Send query patterns
    std::vector<size_t> elements = input.to_vector();
    std::vector<std::vector<size_t>> query_patterns; // Generate patterns based on input
    // Serialize and send query patterns

    // Receive results
    std::vector<uint8_t> reply_data = channels.receive(0);
    std::vector<bool> query_results; // Deserialize reply data

    Set result;
    for (size_t i = 0; i < elements.size(); ++i) {
        if (query_results[i]) {
            result.elements.insert(elements[i]);
        }
    }
    return result;
}

void ApproximateMpsiParty::run_client_approx(const Set& input, Channels& channels) {
    // Encode the set as a Bloom filter and create shares
    SimdBytes share(SHARE_BYTE_COUNT * bin_count);
    std::vector<bool> bloom_filter; // Generate Bloom filter for the input set

    // Apply corruption based on Bloom filter
    SimdBytes corrupted_share(share.to_bytes().size());
    // Perform corruption logic here

    // Send corrupted share
    channels.send(corrupted_share.to_bytes(), 0);
}

std::optional<Set> ApproximateMpsiParty::run(size_t id, size_t n_parties, const std::optional<Set>& input,
                        Channels& channels) {
    if (id == 0) {
        run_server_approx(n_parties, channels);
        return std::nullopt;
    } else if (id == 1) {
        return run_querier_approx(input.value(), channels);
    } else {
        run_client_approx(input.value(), channels);
        return std::nullopt;
    }
}*/

std::vector<bool> ApproximateMpsiParty::compute_query_results(
    const std::vector<std::vector<size_t>>& query_patterns, 
    const SimdBytes& aggregated_share) 
{
    std::vector<bool> results;
    
    // Convert aggregated share into individual byte chunks
    std::vector<std::array<uint8_t, SHARE_BYTE_COUNT>> shares = aggregated_share.to_byte_chunks<SHARE_BYTE_COUNT>();

    for (const auto& query_pattern : query_patterns) {
        std::array<uint8_t, SHARE_BYTE_COUNT> xor_result = {0};

        // XOR all shares corresponding to query indices
        for (size_t index : query_pattern) {
            for (size_t i = 0; i < SHARE_BYTE_COUNT; i++) {
                xor_result[i] ^= shares[index][i];
            }
        }

        // If XOR result is all zero, it’s a match
        results.push_back(std::all_of(xor_result.begin(), xor_result.end(), [](uint8_t b) { return b == 0; }));
    }

    return results;
}

std::vector<std::vector<size_t>> ApproximateMpsiParty::generate_query_patterns(const Set& input) {
    std::vector<std::vector<size_t>> query_patterns;

    for (const auto& element : input.get_elements()) {
        query_patterns.push_back(Set::bloom_filter_indices(element, bin_count, hash_count));
    }

    return query_patterns;
}

Set ApproximateMpsiParty::extract_intersection(
    const Set& input, 
    const std::vector<bool>& query_results) 
{
    Set intersection;

    auto elements = input.get_elements();
    auto it = elements.begin();
    for (size_t i = 0; i < elements.size(); i++, ++it) {
        if (query_results[i]) {
            intersection.insert(*it);
        }
    }

    return intersection;
}

void ApproximateMpsiParty::run_server_approx(size_t n_parties, Channels& channels, Stats& stats) {
    auto start_time = std::chrono::steady_clock::now(); //std::chrono::high_resolution_clock::now();

    // Receive all clients' shares
    std::vector<SimdBytes> received_shares;
    for (size_t i = 1; i < n_parties; ++i) {
        auto data = channels.receive(i);
        if (data.size() > 0)
            received_shares.push_back(SimdBytes::from_bytes(channels.receive(i)));
    }

    // Aggregate shares
    SimdBytes aggregated_share;
    if (received_shares.size() > 0) {
        aggregated_share = received_shares[0];
        for (size_t i = 1; i < received_shares.size(); ++i) {
            aggregated_share ^= received_shares[i];
        }
    }

    // Receive query patterns from the querier (id = 1)
    std::vector<std::vector<size_t>> query_patterns;
    channels.receive(1, query_patterns);

    // Compute results
    std::vector<bool> results = compute_query_results(query_patterns, aggregated_share);

    // Send results to the querier (id = 1)
    channels.send(1, results);

    // Log execution time
    auto end_time = std::chrono::steady_clock::now();//std::chrono::high_resolution_clock::now();
    stats.log_duration("Server Execution Time", start_time, end_time);
}

Set ApproximateMpsiParty::run_querier_approx(const Set& input, Channels& channels, Stats& stats) {
    auto start_time = std::chrono::steady_clock::now();// std::chrono::high_resolution_clock::now();

    // Act as a client first
    run_client_approx(input, channels, stats);

    // Send query patterns
    std::vector<std::vector<size_t>> query_patterns = generate_query_patterns(input);
    channels.send(0, query_patterns);

    // Receive response from server 
    std::vector<bool> results;
    channels.receive(0, results);

    // Extract intersection
    Set output = extract_intersection(input, results);

    // Log execution time
    auto end_time = std::chrono::steady_clock::now();// std::chrono::high_resolution_clock::now();
    stats.log_duration("Querier Execution Time", start_time, end_time);

    return output;
}

void ApproximateMpsiParty::run_client_approx(const Set& input, Channels& channels, Stats& stats) {
    auto start_time = std::chrono::steady_clock::now(); //std::chrono::high_resolution_clock::now();

    // Encode input into a Bloom filter
    std::vector<bool> bloom_filter = input.to_bloom_filter(bin_count, hash_count);

    // Generate a zero share and corrupt it conditionally
    SimdBytes share = create_zero_share(seeds, SHARE_BYTE_COUNT * bin_count);
    SimdBytes corrupted_share = conditionally_corrupt_share(share, bloom_filter);

    // Send the share to the server
    channels.send(corrupted_share.to_bytes(), 0);

    // Log execution time
    auto end_time = std::chrono::steady_clock::now(); // std::chrono::high_resolution_clock::now();
    
    stats.log_duration("Client Execution Time", start_time, end_time);
}

std::optional<Set> ApproximateMpsiParty::run(size_t id, size_t n_parties, const Input& input, Channels& channels, Stats& stats) {
    Set output;
    if (id == 0) {
        run_server_approx(n_parties, channels, stats);
    } else if (id == 1) {
        output = run_querier_approx(*input, channels, stats);
    } else {
        run_client_approx(*input, channels, stats);
    }
    return output;
}




