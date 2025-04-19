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
#include <future>
#include "approx_mpsi.hpp"

extern Stats g_stats;


/* Method Definitions for 'ApproximateMpsi' class */
ApproximateMpsi::ApproximateMpsi(FullMesh& net, size_t minimum_bin_count, size_t hash_count, std::string hash_func, size_t domain_size, size_t set_size /*, Stats& pstats*/)
        : network(net),
          bin_count(((minimum_bin_count + 63) / 64) * 64),
          hash_count(hash_count),
          hash_func(hash_func),
          domain_size(domain_size),
          set_size(set_size)
          //stats(results_filename)
           /* stats(pstats)*/
           
           {
            //stats = Stats();
            //g_stats = stats;
          }

std::vector<std::unique_ptr<Party>> ApproximateMpsi::setup_parties(size_t n_parties) {
    std::vector<std::vector<std::array<uint8_t, RAND_SECRET_SIZE>>> party_seeds(n_parties - 1,
                                                              std::vector<std::array<uint8_t, RAND_SECRET_SIZE>>(n_parties - 1));
   
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint8_t> dist(0, 255);

    for (size_t i = 1; i < n_parties; ++i) {
        for (size_t j = i + 1; j < n_parties; ++j) {
            std::array<uint8_t, RAND_SECRET_SIZE> seed{};
            for (auto& byte : seed) byte = dist(gen);

            party_seeds[i - 1][j - 1] = seed;
            party_seeds[j - 1][i - 1] = seed;
        }
    }

    party_seeds.insert(party_seeds.begin(), {});

    std::vector<std::unique_ptr<Party>> parties;
    parties.reserve(n_parties);
    for (const auto& seeds : party_seeds) {
        parties.push_back(std::make_unique<ApproximateMpsiParty>(network, seeds, bin_count, hash_count, hash_func /*, stats*/));
    }
    return parties;
}

std::vector<std::unique_ptr<Party>> ApproximateMpsi::setup_parties2(size_t n_parties, size_t seeds_sz_factor) {
    //const size_t seeds_sz_factor = 314048; /* Size of the bloom filter*/
    std::vector<std::vector<std::array<uint8_t, RAND_SECRET_SIZE>>> party_seeds(n_parties, 
                                                                std::vector<std::array<uint8_t, RAND_SECRET_SIZE>>(seeds_sz_factor));

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint8_t> dist(0, 255);

    for (size_t i = 1; i < n_parties; ++i) {
        party_seeds[i - 1].resize(seeds_sz_factor);
        for (size_t j = i + 1; j < seeds_sz_factor; ++j) {
            std::array<uint8_t, RAND_SECRET_SIZE> seed{};
            for (auto& byte : seed) byte = dist(gen);

            party_seeds[i - 1][j - 1] = seed;
            //party_seeds[j - 1][i - 1] = seed;
        }
    }

    party_seeds.insert(party_seeds.begin(), {});

    std::vector<std::unique_ptr<Party>> parties;
    parties.reserve(n_parties);
    for (const auto& seeds : party_seeds) {
        parties.push_back(std::make_unique<ApproximateMpsiParty>(network, seeds, bin_count, hash_count, hash_func /*, stats*/));
    }
    return parties;
}

std::vector<Set> ApproximateMpsi::gen_sets_with_uniform_intersection(size_t n_parties, size_t set_size, size_t domain_size) {
    std::vector<Set> sets;
    std::unordered_set<size_t> common_elements;
    std::mt19937 rng(std::random_device{}());

    // Step 1: Generate a common subset of elements
    std::cout << "Generating common subset of elements...\n";
    size_t common_size = set_size / 2; // Half the set will be common
    while (common_elements.size() < common_size) {
        common_elements.insert(rng() % domain_size);
    }

    // Step 2: Generate unique sets for each party
    std::cout << "Generating unique sets for each party...\n";
    for (size_t i = 0; i < n_parties; ++i) {
        std::unordered_set<size_t> unique_elements = common_elements;
        std::cout << "Generating unique set for party " << i <<", unique_elements size=" <<unique_elements.size()
        <<", set_size = "<<set_size << "...\n";
        while (unique_elements.size() < set_size) {
            std::cout<<"Inserting element for unique_elements size="<<unique_elements.size()<<"...\n";
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
    std::cout <<"Generating sets with uniform intersection...\n";
    auto sets = gen_sets_with_uniform_intersection(n_parties, set_size, domain_size);

    std::cout << "Emplacing back\n";
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

void ApproximateMpsi::evaluate(const std::string& experiment_name, size_t party_count,
                                const FullMesh& network_description, size_t repetitions) {
    //Stats stats(results_filename); //experiment_name + ".csv");  // Initialize Stats to log CSV output

    for (size_t i = 0; i < repetitions; ++i) {
        std::cout << "Running repetition " << (i + 1) << " of " << repetitions << "...\n";

        // Step 1: Generate inputs (randomized sets with uniform intersection)
        std::cout << "Generating inputs...\n";
        auto inputs = generate_inputs(party_count);

        // Step 2: Set up network communication (FullMesh)
        std::cout << "Setting up network...\n";
        const FullMesh& network = network_description;

        // Step 3: Set up parties
        std::cout << "Setting up parties...\n";
        auto parties = setup_parties2(party_count+1, set_size*40);

        // Step 4: Run protocol for all parties
        std::cout << "Running protocol for all parties...\n";
        //std::vector<std::optional<Set>> outputs;
        //std::vector<thread_data*> outputs;
        std::vector<std::unique_ptr<thread_data>> outputs;
        std::vector<std::optional<Set>> v_outputs;
        outputs.reserve(parties.size());

        std::vector<std::thread> threads;
        /*
        std::vector<std::promise<std::optional<Set>>> promises;
        std::vector<std::future<std::optional<Set>>> futures;

        promises.resize(party_count);
        futures.resize(party_count);
        */
        for (size_t id = 0; id < party_count; id++) {
        //for (size_t id = 0; id < party_count-1; id++) { //TBD:Is this okay??
        //for (size_t id = 0; id < party_count-2; id++) { //TBD:Is this okay??
        //for (int id = party_count-2; id >=0; id--) {
            //futures[id] = promises[id].get_future();
            //if (parties[id] == nullptr) continue;  // Skip parties that are not part of the protocol
            //auto start_time = std::chrono::steady_clock::now();
            //auto out = parties[id]->run(id, party_count, inputs[id], network.get_channels(id));
            /*threads.emplace_back([&parties, &id, &party_count, &inputs, &network, &promises]() {
                promises[id].set_value(parties[id]->run(id, party_count, inputs[id], network.get_channels(id)));
            });*/
            //thread_data *th_data = new thread_data;
            std::unique_ptr<thread_data> th_data = std::make_unique<thread_data>();
            th_data->id = id;
            th_data->is_completed = false;
            th_data->output = std::nullopt;
            //outputs.push_back(th_data);
            outputs.push_back(std::move(th_data)); // now only 'outputs.back()' owns it
            thread_data* th_data_ptr = outputs.back().get(); // raw pointer is safe here 
            //auto out = parties[id]->run(id, party_count, inputs[id], network.get_channels(id), th_data);
            std::cout<<"ApproximateMpsi::evaluate():Running (1) party id ="<<id<<", party count ="<<party_count<<"\n";
            /*threads.emplace_back([&parties, &party_count, &inputs, &network, th_data]() {
                auto p_id = th_data->id;
                std::cout<<"ApproximateMpsi::evaluate():Running (2) party id ="<<p_id<<", party count ="<<party_count<<"\n";
                parties[p_id]->run(p_id, party_count, *inputs[p_id], network.get_channels(p_id), th_data);
            });*/
            
            threads.emplace_back([&, th_data_ptr /*th_data = std::move(th_data)*/]() mutable {
                auto p_id = th_data_ptr->id;
                std::cout<<"ApproximateMpsi::evaluate():Running (2) party id ="<<p_id<<", party count ="<<party_count<<"\n";
                parties[p_id]->run(p_id, party_count, inputs[p_id], network.get_channels(p_id), th_data_ptr /*th_data.get()*/);
            });
            //auto end_time = std::chrono::steady_clock::now();
            //stats.log_duration(Stats::OPS::COMPUTE_BREAKDOWN, id, start_time, end_time);
            //outputs.push_back(std::move(out));
        }
        std::cout<<"ApproximateMpsi::evaluate():outputs size="<<outputs.size()<<"\n";
        // Wait for all parties to finish
        while (true) {
            bool all_completed = true;
            //for (size_t id = 0; id < party_count-2; id++) {
            for (size_t id = 0; id < party_count; id++) {
                if (!outputs[id]->is_completed) {
                    all_completed = false;
                    break;
                }
            }
            if (all_completed) break;
        }
        //for (size_t id = 0; id < party_count-1; id++) {
        //for (size_t id = 0; id < party_count-2; id++) {
        for (size_t id = 0; id < party_count; id++) {
            /*if (parties[id] == nullptr) continue;  // Skip parties that are not part of the protocol
            if (futures[id].valid()) {
                // Wait for the future to be ready and get the result
                std::cout << "Waiting for party " << id << " to finish...\n";
            }
            auto out = futures[id].get();*/
            auto out = outputs[id]->output;
            v_outputs.push_back(std::move(out));
        }
        for (auto& thread : threads) {
            thread.join(); // Ensure all parties complete
        }
        // Step 5: Validate outputs
        std::cout << "Validating outputs...\n";
        bool success = validate_outputs(inputs, v_outputs);
        std::cout << "Validating Results = "<<success<<"\n";
        //stats.log_experiment(i + 1, success);  // Log success/failure to CSV
    }

    //return stats;
}

/* Method Definitions for 'ApproximateMpsiParty' class */
ApproximateMpsiParty::ApproximateMpsiParty(FullMesh& net, std::vector<std::array<uint8_t, RAND_SECRET_SIZE>> seeds, size_t bin_count, size_t hash_count, std::string hash_func /*, Stats& pstats*/)
    : network(net), seeds(std::move(seeds)), bin_count(bin_count), hash_count(hash_count), hash_func(hash_func)/*, stats(pstats)*/ {}

std::vector<bool> ApproximateMpsiParty::compute_query_results(
    size_t id,
    const std::vector<std::vector<size_t>>& query_patterns, 
    const SimdBytes& aggregated_share) 
{
    std::vector<bool> results;
    
    // Convert aggregated share into individual byte chunks
    std::vector<std::array<uint8_t, SHARE_BYTE_COUNT>> shares = aggregated_share.to_byte_chunks<SHARE_BYTE_COUNT>();

    auto start_time = std::chrono::steady_clock::now();
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
    auto end_time = std::chrono::steady_clock::now();
    g_stats.log_duration(Stats::OPS::XOR_OP, id, start_time, end_time);
    //auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
 
    return results;
}

std::vector<std::vector<size_t>> ApproximateMpsiParty::generate_query_patterns(const Set& input) {
    std::vector<std::vector<size_t>> query_patterns;

    /*
    for (const auto& element : input.get_elements()) {
        query_patterns.push_back(input.bloom_filter_indices(element, bin_count, hash_count, hash_func));
    }
    return query_patterns;
    */

   return input.bloom_filter_indices(bin_count, hash_count, hash_func);
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

void ApproximateMpsiParty::run_server_approx(size_t id, size_t n_parties, Channels& channels) {
    auto start_time = std::chrono::steady_clock::now();

    // Receive all clients' shares
    std::vector<SimdBytes> received_shares;
    for (size_t i = 1; i < n_parties; ++i) {
        /*auto data = channels.receive(i);*/
        auto data = network.receive(id, i);
        if (data.size() > 0) {
            //received_shares.push_back(SimdBytes::from_bytes(channels.receive(i)));
            received_shares.push_back(SimdBytes::from_bytes(data));
        }
    }
    std::cout<<"ApproximateMpsiParty::run_server_approx():received share size="<<received_shares.size()<<"\n";
    // Aggregate shares
    SimdBytes aggregated_share;
    if (received_shares.size() > 0) {
        aggregated_share = received_shares[0];
        for (size_t i = 1; i < received_shares.size(); ++i) {
            aggregated_share ^= received_shares[i];
        }
    }

    std::cout<<"ApproximateMpsiParty::run_server_approx():aggregated share size="<<aggregated_share.to_bytes().size()<<"\n";
    // Receive query patterns from the querier (id = 1)
    std::vector<std::vector<size_t>> query_patterns;
    /* channels.receive(1, query_patterns); */
    network.receive(id, 1, query_patterns);

    std::cout<<"ApproximateMpsiParty::run_server_approx():query pattern size="<<query_patterns.size()<<"\n";
    // Compute results
    std::vector<bool> results = compute_query_results(id, query_patterns, aggregated_share);

    std::cout<<"ApproximateMpsiParty::run_server_approx():result size (compute_query_results)="<<results.size()<<"\n";
    // Send results to the querier (id = 1)
    /* channels.send(1, results); */
    network.send(id, 1, results);

    // Log execution time
    auto end_time = std::chrono::steady_clock::now();
    //stats.log_duration("Server Execution Time", start_time, end_time);
    std::cout<<"ApproximateMpsiParty::run_server_approx():"<<start_time.time_since_epoch().count()<<", "<<end_time.time_since_epoch().count()<<"\n";
}

Set ApproximateMpsiParty::run_querier_approx(size_t id, const Set& input, Channels& channels) {
    auto start_time = std::chrono::steady_clock::now();

    // Act as a client first
    run_client_approx(id, input, channels);

    // Send query patterns
    std::vector<std::vector<size_t>> query_patterns = generate_query_patterns(input);
    std::cout<<"ApproximateMpsiParty::run_querier_approx():input size = "<<input.to_vector().size()<<", query_patterns size="<<query_patterns.size()<<"\n";
    /* channels.send(0, query_patterns); */
    network.send(id, 0, query_patterns);
    
    // Receive response from server 
    std::vector<bool> results;
    /* channels.receive(0, results); */
    network.receive(id, 0, results);

    // Extract intersection
    Set output = extract_intersection(input, results);
    std::cout<<"ApproximateMpsiParty::run_querier_approx():ouput size (extracted intersection size) = "<<output.to_vector().size()<<"\n";
    // Log execution time
    auto end_time = std::chrono::steady_clock::now();
    //stats.log_duration("Querier Execution Time", start_time, end_time);
    std::cout<<"ApproximateMpsiParty::run_querier_approx():"<<start_time.time_since_epoch().count()<<", "<<end_time.time_since_epoch().count()<<"\n";
    return output;
}

void ApproximateMpsiParty::run_client_approx(size_t id, const Set& input, Channels& channels) {
    

    // Encode input into a Bloom filter
    std::vector<bool> bloom_filter;
    //Running as lambda function for bloom filter
    std::thread bloom_thread([&bloom_filter, /*&bloom_done,*/ &input, this, id]() {
        auto start_time = std::chrono::steady_clock::now();
        bloom_filter = input.to_bloom_filter(this->bin_count, this->hash_count, this->hash_func);//Xi
        std::cout<<"ApproximateMpsiParty::run_client_approx(): bloom filter size="<<bloom_filter.size()<<"\n";
        auto end_time = std::chrono::steady_clock::now();
        g_stats.log_duration(Stats::OPS::BLOOMFILTER_OP, id, start_time, end_time);
   });

    auto start_time = std::chrono::steady_clock::now();
    // Generate a zero share and corrupt it conditionally
    //SimdBytes share = create_zero_share(seeds, SHARE_BYTE_COUNT * bin_count, hash_func);//Mi
    SimdBytes share = create_zero_share_no_resize(seeds, SHARE_BYTE_COUNT * bin_count, hash_func);//Mi //g_options.set_size
    //SimdBytes share = create_zero_share_parallel(seeds, SHARE_BYTE_COUNT * bin_count, hash_func);//Mi
    //SimdBytes share = create_zero_share_parallel(seeds, g_options.set_size, hash_func);
    std::cout << "Bloom Filter Size: " << bloom_filter.size()
          << ", bin_count: " << bin_count
          << ", Seeds size: " << seeds.size()
          << ", Expected false_values.bytes.size(): " << share.to_bytes().size()
          << std::endl;
    // Wait for bloom filter to be ready
    bloom_thread.join();

    //SimdBytes corrupted_share = conditionally_corrupt_share(share, bloom_filter);//Ri
    SimdBytes corrupted_share = conditionally_corrupt_share_parallel(share, bloom_filter);
    std::cout<<"ApproximateMpsiParty::run_client_approx(): share size="<<share.to_bytes().size()<<", corrupted share size="<<corrupted_share.to_bytes().size()<<"\n";
    // Log execution time
    auto end_time = std::chrono::steady_clock::now();
    g_stats.log_duration(Stats::OPS::XOF_OP, id, start_time, end_time);

    // Send the share to the server
    /* channels.send(corrupted_share.to_bytes(), 0); */
    network.send(id, 0, corrupted_share.to_bytes());

    //stats.log_duration("Client Execution Time", start_time, end_time);
    std::cout<<"ApproximateMpsiParty::run_client_approx():"<<start_time.time_since_epoch().count()<<", "<<end_time.time_since_epoch().count()<<"\n";
}

std::optional<Set> ApproximateMpsiParty::run(size_t id, size_t n_parties, const Input& input, Channels& channels, thread_data* t_data) {
    Set output;
    auto start_time = std::chrono::steady_clock::now();
    switch(id) {
        case 0:
            run_server_approx(id, n_parties, channels);
            break;

        case 1:
            output = run_querier_approx(id, *input, channels);
            break;

        default:
            run_client_approx(id, *input, channels);
            break;
        
    }
    auto end_time = std::chrono::steady_clock::now();
    g_stats.log_duration(Stats::OPS::COMPUTE_BREAKDOWN, id, start_time, end_time);
    t_data->id = id;
    t_data->output = output;
    t_data->is_completed = true;
    return output;
}




