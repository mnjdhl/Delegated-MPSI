#include <vector>
#include <array>
#include <cstdint>
#include <random>
#include <chrono>
#include <assert.h>
#include <future>
#include <numeric>
#include <execution>  // C++17 Parallel STL (if using it optionally)
#include <thread>

#include "blake3.h" // Include BLAKE3 library for hashing
#include "secret_sharing_simd.hpp"
#include "hash_funcs.hpp"
#include "ThreadPool.h"

// Constants
//constexpr size_t SHARE_BYTE_COUNT = 64;

#if 0
void SimdBytes::resize(size_t byte_count) {
    size_t simd_blocks = byte_count / sizeof(__m128i);  // Align to SIMD register size
    bytes.resize(simd_blocks);
}

// Helper Struct for SIMD Bytes
SimdBytes SimdBytes::from_bytes(const std::vector<uint8_t>& data) {
    SimdBytes simdBytes;
    size_t chunk_count = data.size() / SHARE_BYTE_COUNT;

    simdBytes.bytes.resize(chunk_count);
    for (size_t i = 0; i < chunk_count; ++i) {
        for (size_t j = 0; j < 4; ++j) {
            simdBytes.bytes[i][j] = _mm_loadu_si128(
                reinterpret_cast<const __m128i*>(&data[i * SHARE_BYTE_COUNT + j * 16]));
        }
    }
    return simdBytes;
}

// Convert SimdBytes back to raw bytes
std::vector<uint8_t> SimdBytes::to_bytes() const {
    std::vector<uint8_t> result(bytes.size() * SHARE_BYTE_COUNT);
    for (size_t i = 0; i < bytes.size(); ++i) {
        for (size_t j = 0; j < 4; ++j) {
            _mm_storeu_si128(
                reinterpret_cast<__m128i*>(&result[i * SHARE_BYTE_COUNT + j * 16]),
                bytes[i][j]);
        }
    }
    return result;
}

// XOR operation for SimdBytes
SimdBytes& SimdBytes::operator^=(const SimdBytes& rhs) {
    for (size_t i = 0; i < bytes.size(); ++i) {
        for (size_t j = 0; j < 4; ++j) {
            bytes[i][j] = _mm_xor_si128(bytes[i][j], rhs.bytes[i][j]);
        }
    }
    return *this;
}

// Conditional selection
SimdBytes SimdBytes::select( const std::vector<std::array<__m128i, 4>>& masks, const SimdBytes& true_values, const SimdBytes& false_values) {
    SimdBytes result;
    auto rsize = masks.size();
    std::cout<<"SimdBytes::select(): rsize = "<<rsize<<"\n";
    result.bytes.resize(rsize);

    //std::cout<<"SimdBytes::select()(1):  result size="<<result.to_bytes().size()<<", mask size="<<masks.size()<<", true values size="<<true_values.to_bytes().size()<<", false values size="<<false_values.to_bytes().size()<<"\n";
    std::cout << "false_values.bytes.size(): " << false_values.bytes.size() << "\n";
    std::cout << "true_values.bytes.size(): " << true_values.bytes.size() << "\n";
    std::cout << "masks.size(): " << masks.size() << "\n";
    std::cout << "result.bytes.size(): " << result.bytes.size() << "\n";
    for (size_t i = 0; i < masks.size(); ++i) {
        for (size_t j = 0; j < 4; ++j) {
            //std::cout << "i: " << i << "\n";
            //std::cout << "j: " << j << "\n";
            /* Check the Out-of-Bounds Accesses */
            assert(i < result.bytes.size());
            assert(j < result.bytes[i].size());
            assert(i < false_values.bytes.size());
            assert(j < false_values.bytes[i].size());
            assert(i < true_values.bytes.size());
            assert(j < true_values.bytes[i].size());
            assert(i < masks.size());
            assert(j < masks[i].size());

            result.bytes[i][j] = _mm_blendv_epi8(
                false_values.bytes[i][j], true_values.bytes[i][j], masks[i][j]);
        }
    }
    std::cout<<"SimdBytes::select()(2):  result size="<<result.to_bytes().size()<<", mask size="<<masks.size()<<", true values size="<<true_values.to_bytes().size()<<", false values size="<<false_values.to_bytes().size()<<"\n";

    return result;
}
#endif


 SimdBytes::SimdBytes(size_t byte_count) {
    bytes.resize(byte_count, 0);
}
SimdBytes::SimdBytes(size_t byte_count, uint8_t value) {
    bytes.resize(byte_count, value);
}

void SimdBytes::resize(size_t byte_count) {
    bytes.resize(byte_count, 0);
}

size_t SimdBytes::size() const {
    return bytes.size();
}

void SimdBytes::fill(uint8_t value) {
    std::fill(bytes.begin(), bytes.end(), value);
}

std::vector<uint8_t> SimdBytes::to_bytes() const {
    return bytes;
}
SimdBytes SimdBytes::from_bytes(const std::vector<uint8_t>& data) {
    SimdBytes simdBytes(data.size());
    simdBytes.bytes = data;
    return simdBytes;
}

SimdBytes& SimdBytes::operator^=(const SimdBytes& other) {
    assert(bytes.size() == other.bytes.size());
    for (size_t i = 0; i < bytes.size(); ++i) {
        bytes[i] ^= other.bytes[i];
    }
    return *this;
}

SimdBytes SimdBytes::operator^(const SimdBytes& other) const {
    SimdBytes result = *this;
    result ^= other;
    return result;
}

bool SimdBytes::operator==(const SimdBytes& other) const {
    return bytes == other.bytes;
}

SimdBytes SimdBytes::select(const std::vector<uint8_t>& mask, const SimdBytes& true_values, const SimdBytes& false_values) {
    assert(mask.size() == true_values.bytes.size());
    assert(true_values.bytes.size() == false_values.bytes.size());

    SimdBytes result(mask.size());
    for (size_t i = 0; i < mask.size(); ++i) {
        result.bytes[i] = mask[i] ? true_values.bytes[i] : false_values.bytes[i];
    }
    return result;
}

// Convert SimdBytes into byte chunks
/*template <const size_t ChunkSize>
    std::vector<std::array<uint8_t, ChunkSize>> SimdBytes::to_byte_chunks() const {
    std::vector<std::array<uint8_t, ChunkSize>> result;
    result.reserve(bytes.size());

    for (const auto& chunk : bytes) {
        std::array<uint8_t, ChunkSize> byte_chunk;
        for (size_t i = 0; i < 4; ++i) {
            _mm_storeu_si128(reinterpret_cast<__m128i*>(&byte_chunk[i * 16]), chunk[i]);
        }
        result.push_back(byte_chunk);
    }

    return result;
}*/

// Blake3-like Hash Expansion (placeholder for actual hashing)
//SimdBytes xof(const std::vector<uint8_t>& seed, size_t byte_count) {
SimdBytes xof(const std::array<uint8_t, RAND_SECRET_SIZE>& seed, size_t byte_count) {
    std::vector<uint8_t> expanded_bytes(byte_count);
    std::random_device rd;
    std::generate(expanded_bytes.begin(), expanded_bytes.end(), [&rd]() {
        return static_cast<uint8_t>(rd() & 0xFF);
    });
    return SimdBytes::from_bytes(expanded_bytes);
}

// Function for XOF (Extendable Output Function) using BLAKE3
SimdBytes blake3_xof(const std::array<uint8_t, RAND_SECRET_SIZE>& seed, size_t byte_count) {
    //auto start_time = std::chrono::steady_clock::now();
    // Initialize the BLAKE3 hasher and compute XOF
    blake3_hasher hasher;
    blake3_hasher_init(&hasher);
    blake3_hasher_update(&hasher, seed.data(), seed.size());

    // Finalize the output
    std::vector<uint8_t> expanded_bytes(byte_count, 0);
    blake3_hasher_finalize(&hasher, expanded_bytes.data(), byte_count);
    //auto end_time = std::chrono::steady_clock::now();
    return SimdBytes::from_bytes(expanded_bytes);
}

// Function for XOF (Extendable Output Function) using BLAKE3
SimdBytes do_generic_hash(const std::array<uint8_t, RAND_SECRET_SIZE>& seed, size_t byte_count, std::string hash_func) {

    //auto expanded_bytes = generic_hash_func(hash_func, seed.data(), seed.size());
    auto expanded_bytes = generic_hash_func(hash_func, seed.data(), byte_count);
    return SimdBytes::from_bytes(expanded_bytes);
}

// Create Zero Share
SimdBytes create_zero_share2(const std::vector<std::array<uint8_t, RAND_SECRET_SIZE>>& seeds, size_t byte_count, std::string hash_func) {
    auto seeds_iterator = seeds.begin();
    SimdBytes share = do_generic_hash(*seeds_iterator, byte_count, hash_func); 

    for (++seeds_iterator; seeds_iterator != seeds.end(); ++seeds_iterator) {
        share ^= do_generic_hash(*seeds_iterator, byte_count, hash_func); 
    }
    return share;
}   

SimdBytes create_zero_share_no_resize(const std::vector<std::array<uint8_t, RAND_SECRET_SIZE>>& seeds, size_t byte_count, std::string hash_func) {
    auto sresize = seeds.size();

    auto seeds_iterator = seeds.begin();
    SimdBytes share = do_generic_hash(*seeds_iterator, byte_count, hash_func);


    for (++seeds_iterator; seeds_iterator != seeds.end(); ++seeds_iterator) {
        SimdBytes hash_result = do_generic_hash(*seeds_iterator, byte_count, hash_func);
        
        share ^= hash_result;  // Now XOR will be safe
    }

    std::cout << "Created zero share with " << share.to_bytes().size() << " elements (Expected: " << sresize / 16 << ")" << std::endl;

    return share;
}

SimdBytes create_zero_share(const std::vector<std::array<uint8_t, RAND_SECRET_SIZE>>& seeds, size_t byte_count, std::string hash_func) {
    auto sresize = seeds.size();

    auto seeds_iterator = seeds.begin();
    SimdBytes share = do_generic_hash(*seeds_iterator, byte_count, hash_func);

    /* Increases performance if we don't resize here */
    share.resize(sresize);  // 🔹 Resize to match 'share'

    for (++seeds_iterator; seeds_iterator != seeds.end(); ++seeds_iterator) {
        SimdBytes hash_result = do_generic_hash(*seeds_iterator, byte_count, hash_func);
        
        /* Increases performance if we don't resize here */
        hash_result.resize(sresize);  // 🔹 Resize hash result to match 'share'
        
        share ^= hash_result;  // Now XOR will be safe
    }

    std::cout << "Created zero share with " << share.to_bytes().size() << " elements (Expected: " << sresize / 16 << ")" << std::endl;

    return share;
}

SimdBytes create_zero_share_parallel_no_threadpool(
    const std::vector<std::array<uint8_t, RAND_SECRET_SIZE>>& seeds,
    size_t byte_count,
    std::string hash_func
) {
    const size_t seed_count = seeds.size();
    SimdBytes share;
    share.resize(seed_count);  // Prepare final share storage

    // Step 1: Launch parallel hash computations
    std::vector<std::future<SimdBytes>> futures;
    futures.reserve(seed_count);
    for (auto seeds_iterator = seeds.begin(); seeds_iterator != seeds.end(); ++seeds_iterator) {
        futures.emplace_back(std::async(std::launch::async, [=]() {
            try {
            SimdBytes hash = do_generic_hash(*seeds_iterator, byte_count, hash_func);
            hash.resize(seed_count);
            return hash;
            } catch (const std::exception& e) {
                std::cerr << "Error in hash computation: " << e.what() << std::endl;
                return SimdBytes();  // Return empty SimdBytes on error
            }
        }));
    }

    // Step 2: XOR reduce the results
    SimdBytes result;
    result.resize(seed_count);
    bool first = true;

    for (auto& future : futures) {
        SimdBytes hash_result = future.get();

        if (first) {
            result = hash_result;
            first = false;
        } else {
            result ^= hash_result;
        }
    }

    std::cout << "Created zero share with " << result.to_bytes().size() << " elements (Expected: " << seed_count / 16 << ")" << std::endl;

    return result;
}

SimdBytes create_zero_share_parallel(
    const std::vector<std::array<uint8_t, RAND_SECRET_SIZE>>& seeds,
    size_t byte_count,
    std::string hash_func
) {
    const size_t seed_count = seeds.size();
    SimdBytes result;
    result.resize(byte_count);  // XOR result should match byte_count
    std::vector<std::future<SimdBytes>> futures;

    ThreadPool pool(std::thread::hardware_concurrency());

    for (size_t i = 0; i < seed_count; ++i) {
        futures.emplace_back(
            pool.enqueue([&, i] {
                try {
                    SimdBytes hash = do_generic_hash(seeds[i], byte_count, hash_func);
                    return hash;
                } catch (const std::exception& e) {
                    std::cerr << "Hash error at i=" << i << ": " << e.what() << "\n";
                    return SimdBytes(byte_count, 0);  // Return zeroed hash on error
                }
            })
        );
    }

    bool first = true;
    for (auto& future : futures) {
        SimdBytes hash = future.get();
        if (first) {
            result = hash;
            first = false;
        } else {
            result ^= hash;
        }
    }

    std::cout << "Created zero share with " << result.to_bytes().size()
              << " bytes (Expected: " << byte_count << ")" << std::endl;

    return result;
}

#if 0
// Conditionally Corrupt Share
SimdBytes conditionally_corrupt_share( const SimdBytes& share, const std::vector<bool>& conditions) {
    // Expand conditions into a mask vector
    std::vector<std::array<__m128i, 4>> masks;
    size_t chunk_count = conditions.size();

    for (size_t i = 0; i < chunk_count; ++i) {
        std::array<uint8_t, SHARE_BYTE_COUNT> mask_chunk = {};
        for (size_t j = 0; j < SHARE_BYTE_COUNT; ++j) {
            mask_chunk[j] = conditions[i] ? 0xFF : 0x00; // Expand condition to mask
        }

        std::array<__m128i, 4> mask_sse;
        for (size_t j = 0; j < 4; ++j) {
            mask_sse[j] = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&mask_chunk[j * 16]));
        }
        masks.push_back(mask_sse);
    }
    std::cout<<"SimdBytes::conditionally_corrupt_share(): chunk count="<<chunk_count<<", mask size="<<masks.size()<<"\n";
 
    // Generate randomness
    auto rsize = SHARE_BYTE_COUNT * chunk_count;
    //auto rsize =  chunk_count;
    std::vector<uint8_t> randomness(rsize);
    std::random_device rd;
    std::generate(randomness.begin(), randomness.end(), [&rd]() {
        return static_cast<uint8_t>(rd() & 0xFF);
    });
    std::cout<<"SimdBytes::conditionally_corrupt_share():  randomness size="<<randomness.size()<<", rsize = "<<rsize<<"\n";
 
    SimdBytes randomness_simd = SimdBytes::from_bytes(randomness);
   
    auto ret_val = SimdBytes::select(masks, randomness_simd, share);

    std::cout<<"SimdBytes::conditionally_corrupt_share():  randomness simd size="<<randomness_simd.to_bytes().size()<<", return value size = "<<ret_val.to_bytes().size()<<"\n";
 
    // Use SIMD select to conditionally corrupt the share
    return ret_val;
}
#endif

SimdBytes conditionally_corrupt_share(
    const SimdBytes& share,
    const std::vector<bool>& conditions,
    size_t chunk_size  // Usually SHARE_BYTE_COUNT
) {
    size_t num_chunks = conditions.size();
    size_t total_size = share.bytes.size(); // num_chunks;// * chunk_size;

    //assert(share.bytes.size() == total_size && "Share size mismatch with condition mask");

    // Generate randomness
    std::vector<uint8_t> random_bytes(total_size);
    std::random_device rd;
    std::generate(random_bytes.begin(), random_bytes.end(), [&rd](){
        return static_cast<uint8_t>(rd() & 0xFF);
    });

    // Create result share with corruption applied
    SimdBytes corrupted(total_size);
    for (size_t i = 0; i < total_size; ++i) {
        size_t condition_idx = i / chunk_size;
        corrupted.bytes[i] = (condition_idx < conditions.size() && conditions[condition_idx])
            ? random_bytes[i]
            : share.bytes[i];
    }

    return corrupted;
}

SimdBytes conditionally_corrupt_share_parallel(
    const SimdBytes& share,
    const std::vector<bool>& conditions,
    size_t chunk_size  // Usually SHARE_BYTE_COUNT
) {
    const size_t total_size = share.bytes.size();
    const size_t num_chunks = conditions.size();
    //assert(total_size >= chunk_size * num_chunks);//Wronly put assertion

    // Step 1: Generate randomness in parallel
    std::vector<uint8_t> random_bytes(num_chunks); //total_size);
    {
        std::random_device rd;
        std::generate(random_bytes.begin(), random_bytes.end(), [&rd]() {
            return static_cast<uint8_t>(rd() & 0xFF);
        });
    }

    // Step 2: Define worker for a range of bytes
    auto corrupt_worker = [&](size_t start, size_t end) -> std::vector<uint8_t> {
        std::vector<uint8_t> segment(end - start);

        for (size_t i = start; i < end; ++i) {
            /*
            size_t condition_idx = i / chunk_size;
            bool corrupt = (condition_idx < num_chunks && conditions[condition_idx]);
            segment[i - start] = corrupt ? random_bytes[i] : share.bytes[i];
            */
            if (conditions[i]) {
                segment[i - start] = 0;
            } else {
                segment[i - start] = random_bytes[i];
            }
            segment[i - start] ^= share.bytes[i%total_size];
        }

        return segment;
    };

    // Step 3: Launch parallel workers
    const size_t num_threads = std::thread::hardware_concurrency();
    const size_t chunk = (num_chunks + num_threads - 1) / num_threads; //(total_size + num_threads - 1) / num_threads;

    std::vector<std::future<std::vector<uint8_t>>> futures;
    for (size_t t = 0; t < num_threads; ++t) {
        size_t start = t * chunk;
        size_t end = std::min(start + chunk, num_chunks); //total_size);
        if (start >= end) break;  // Avoid launching empty tasks
        futures.emplace_back(std::async(std::launch::async, corrupt_worker, start, end));
    }

    // Step 4: Combine all results
    SimdBytes corrupted;
    corrupted.bytes.reserve(num_chunks); //total_size);

    for (auto& fut : futures) {
        std::vector<uint8_t> part = fut.get();
        corrupted.bytes.insert(corrupted.bytes.end(), part.begin(), part.end());
    }

    return corrupted;
}