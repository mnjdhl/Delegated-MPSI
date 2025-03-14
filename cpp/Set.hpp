#ifndef SET_HPP
#define SET_HPP

#include <unordered_set>
#include <vector>
#include <optional>
#include <boost/container_hash/hash.hpp>
//#include "secret_sharing_simd.hpp"
#if USE_BLOOM_FILTER_LIB
#include "bloom_filter.hpp"
#endif

class Set {
public:
    Set();
    Set(std::unordered_set<size_t> elems);
    explicit Set(std::initializer_list<size_t> init);
    void insert(size_t element);
    static Set intersection(const std::vector<Set>& sets);
    std::vector<size_t> to_vector() const;
    bool operator==(const Set& other) const;
    std::unordered_set<size_t> get_elements() const;
    std::vector<size_t> bloom_filter_indices_std_hash(const size_t element, 
        size_t bin_count, size_t hash_count);
    std::vector<size_t> bloom_filter_indices_boost_hash(const size_t element, 
            size_t bin_count, size_t hash_count);

    std::vector<bool> to_bloom_filter(size_t bin_count, size_t hash_count, const std::string hash_function) const;
    std::vector<bool> to_bloom_filter2(size_t bin_count, size_t hash_count, const std::string hash_function) const;

    std::vector<size_t> bloom_filter_indices(const size_t element, 
        size_t bin_count, size_t hash_count, const std::string hash_function) const;
    
    std::vector<std::vector<size_t>>  bloom_filter_indices(size_t bin_count, size_t hash_count, const std::string hash_func) const;    
    

private:
    double epsilon; /* False positive probability */
    std::unordered_set<size_t> elements;
#if USE_BLOOM_FILTER_LIB
    bloom_parameters bl_parameters;
    bloom_filter bl_filter;
    void bloom_init(unsigned long long element_count, double false_positive_prob, unsigned long long rand_seed);
    void init();
#endif

    std::size_t compute_optimal_bit_size(std::size_t n, std::size_t bin_count) const;
    std::size_t compute_optimal_hash_count(std::size_t n, std::size_t m) const ;
    std::size_t extract_hash_value(const std::vector<uint8_t>& hash_result) const ;
};

using Input = std::optional<Set>;
const unsigned long long RANDOM_SEED = 0xA5A5A5A5;
const double FALSE_POSITIVE_PROBABILITY = 0.0001;                          
#endif // SET_HPP