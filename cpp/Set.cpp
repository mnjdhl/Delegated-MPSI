#include <iostream>
#include "Set.hpp"
#include "hash_funcs.hpp"
#include "common.hpp"
#include "secret_sharing_simd.hpp"

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
Set::Set() {
    epsilon = 0.1;
}

Set::Set(std::initializer_list<size_t> init_list) : elements(init_list) {
    epsilon = 0.1;
}

Set::Set(std::unordered_set<size_t> elems) : elements(std::move(elems)) {
    epsilon = 0.1;
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

/*std::size_t Set::compute_optimal_bit_size(std::size_t n, std::size_t bin_count) const {
    std::size_t base_size = static_cast<std::size_t>(std::ceil(-(n * std::log(epsilon)) / (std::log(2) * std::log(2))));
    std::cout<<"Set::compute_optimal_bit_size: base_size = " <<base_size<<", returns="<<base_size * bin_count<<"\n";
    return base_size * bin_count; // Scale bit array by bin count
}*/

std::size_t Set::compute_optimal_bit_size(std::size_t n, std::size_t bin_count) const {
    double bits_per_element = 14.3779296875; // -(std::log(epsilon) / (std::log(2) * std::log(2))); 
    std::size_t bit_array_size = static_cast<std::size_t>(std::ceil(n * bits_per_element));

    std::cout<<"Set::compute_optimal_bit_size: elements size = "<<n<<", bits_per_element = " <<bits_per_element<<", bit_array_size="<<bit_array_size<<"\n";
  
    // Normalize the bit array size based on bin count
    //bit_array_size = (bit_array_size / bin_count) * bin_count;
  
    bit_array_size = bit_array_size * 40;
    // Set an upper limit to avoid excessive memory usage
    std::size_t MAX_BITS = 100'000'000; // g_options.set_size*40;  //1'000'000;//10'000'000; // Example cap: 10 million bits
    auto retvalue = std::min(bit_array_size, MAX_BITS);

    std::cout<<"Set::compute_optimal_bit_size: Final bit_array_size="<<bit_array_size<<", retvalue="<<retvalue<<"\n";
    return retvalue;
}

std::size_t Set::compute_optimal_hash_count(std::size_t n, std::size_t m) const {
        return static_cast<std::size_t>(std::ceil((m / n) * std::log(2)));
}

std::size_t Set::extract_hash_value(const std::vector<uint8_t>& hash_result) const {
    std::size_t hash_value = 0;
    std::memcpy(&hash_value, hash_result.data(), std::min(sizeof(hash_value), hash_result.size()));
    return hash_value;
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

/* It uses generic hash function */
std::vector<size_t> Set::bloom_filter_indices(const size_t element, 
    size_t bin_count, size_t hash_count, const std::string hash_func) const {
    std::vector<size_t> indices;

    std::vector<uint8_t> element_bytes(sizeof(element));
    std::memcpy(element_bytes.data(), &element, sizeof(element));  // Convert element to bytes

    for (size_t i = 0; i < hash_count; i++) {
        auto hash_out = generic_hash_func(hash_func, element_bytes.data(), element_bytes.size() + i);
        size_t hash = extract_hash_value(hash_out) % bin_count;
        indices.push_back(hash);
    }

    return indices;
}

void Set::insert(size_t element) {
    elements.insert(element);
}

// Convert the set into a Bloom filter representation
std::vector<bool> Set::to_bloom_filter2(size_t bin_count, size_t hash_count, std::string hash_func) const {
    std::vector<bool> bloom_filter(bin_count, false);

    for (size_t element : elements) {
        auto indices = bloom_filter_indices(element, bin_count, hash_count, hash_func);
        for (size_t idx : indices) {
            bloom_filter[idx] = true;
        }
    }

    return bloom_filter;
}


std::vector<std::vector<size_t>> Set::bloom_filter_indices(size_t bin_count, size_t hash_count, const std::string hash_func) const {
    std::vector<std::vector<size_t>> indices;
    std::size_t bit_array_size = compute_optimal_bit_size(elements.size(), bin_count);

    for (const auto& element : elements) {
        std::vector<size_t> indic;
        std::vector<uint8_t> element_bytes(sizeof(element));
        std::memcpy(element_bytes.data(), &element, sizeof(element));  // Convert element to bytes

        for (std::size_t i = 0; i < hash_count; ++i) {
            std::vector<uint8_t> hash_result = generic_hash_func(hash_func, element_bytes.data(), element_bytes.size() + i);
            std::size_t hash_value = extract_hash_value(hash_result) % bit_array_size;
            indic.push_back(hash_value % bin_count); // Map indices to bin count
        }
        indices.push_back(indic);
    }
    return indices;
}

std::vector<bool> Set::to_bloom_filter(size_t bin_count, size_t hash_count, std::string hash_func) const {
    std::size_t bit_array_size = compute_optimal_bit_size(elements.size(), bin_count);
    if (bit_array_size% SHARE_BYTE_COUNT != 0) {
        bit_array_size += (SHARE_BYTE_COUNT - (bit_array_size % SHARE_BYTE_COUNT)); // Align to SHARE_BYTE_COUNT
    }
 
    std::vector<bool> bit_array(bit_array_size, false);  // Bit array initialized with false

    for (const auto& element : elements) {
        std::vector<uint8_t> element_bytes(sizeof(element));
        std::memcpy(element_bytes.data(), &element, sizeof(element));  // Convert element to bytes
        for (std::size_t i = 0; i < hash_count; ++i) {
            auto hash_out = generic_hash_func(hash_func, element_bytes.data(), element_bytes.size() + i);
            size_t hash_value = extract_hash_value(hash_out) % bit_array_size;
            bit_array[hash_value] = true;
        }
    }
    return bit_array;
}