#include <iostream>
#include "Set.hpp"
#include "hash_funcs.hpp"

/* Method Definitions for 'Set' class */
#if USE_BLOOM_FILTER_LIB
Set::Set() {
    init();
}

Set::Set(std::initializer_list<size_t> init_list) : elements(init_list) {

    init();
}

Set::Set(std::unordered_set<size_t> elems) : elements(std::move(elems)) {
    init();
}
void Set::bloom_init(unsigned long long element_count, double false_positive_prob, unsigned long long rand_seed) {
       // How many elements roughly do we expect to insert?
   bl_parameters.projected_element_count = element_count; //1000

   // Maximum tolerable false positive probability? (0,1)
   bl_parameters.false_positive_probability = false_positive_prob; //0.0001; // 1 in 10000

   // Simple randomizer (optional)
   bl_parameters.random_seed = rand_seed;

   if (!bl_parameters)
   {
      std::cout << "bloom_init:Error - Invalid set of bloom filter parameters!" << std::endl;
      return;
   }

   bl_parameters.compute_optimal_parameters();

   //Instantiate Bloom Filter
    bl_filter = bloom_filter(bl_parameters);
;
}

void Set::init() {
    elements.clear();
    bloom_init(elements.size(), FALSE_POSITIVE_PROBABILITY, RANDOM_SEED);
}
#else
Set::Set() = default;

Set::Set(std::initializer_list<size_t> init_list) : elements(init_list) {
}

Set::Set(std::unordered_set<size_t> elems) : elements(std::move(elems)) {
}
#endif

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

std::vector<size_t> Set::bloom_filter_indices_std_hash(const size_t element, 
    size_t bin_count, size_t hash_count) {
    std::vector<size_t> indices;
    std::hash<size_t> hasher;

    for (size_t i = 0; i < hash_count; i++) {
        indices.push_back((hasher(element + i) % bin_count));
    }

    return indices;
}

std::vector<size_t> Set::bloom_filter_indices_boost_hash(const size_t element, 
    size_t bin_count, size_t hash_count) {
    std::vector<size_t> indices;
    boost::hash<size_t> boost_hasher;

    for (size_t i = 0; i < hash_count; i++) {
        indices.push_back((boost_hasher(element + i) % bin_count));
    }

    return indices;
}

/* It uses blake3_xof() as hash function */
std::vector<size_t> Set::bloom_filter_indices(const size_t element, 
    size_t bin_count, size_t hash_count, const std::string hash_func) {
    std::vector<size_t> indices;
    boost::hash<size_t> boost_hasher;
    size_t tmp_element;
    for (size_t i = 0; i < hash_count; i++) {
        tmp_element = element + i;
        uint8_t *seed = reinterpret_cast<uint8_t*>(&tmp_element);
        auto hash_out = generic_hash_func(hash_func, seed, sizeof(size_t)); //blake3_xof(seed, sizeof(size_t));
        size_t hash = *reinterpret_cast<size_t*>(hash_out.data());
        indices.push_back(hash % bin_count);
    }

    return indices;
}

void Set::insert(size_t element) {
    elements.insert(element);
}

// Convert the set into a Bloom filter representation
std::vector<bool> Set::to_bloom_filter(size_t bin_count, size_t hash_count, std::string hash_func) const {
    std::vector<bool> bloom_filter(bin_count, false);

    for (size_t element : elements) {
        auto indices = Set::bloom_filter_indices(element, bin_count, hash_count, hash_func);
        for (size_t idx : indices) {
            bloom_filter[idx] = true;
        }
    }

    return bloom_filter;
}