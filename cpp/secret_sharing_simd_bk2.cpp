#include <algorithm>
#include <vector>
#include <array>
#include <cstdint>
#include <cstring>
#include <immintrin.h> // For AVX/AVX2/AVX512 intrinsics
#include <random>
//#include <blake3.h> // Include BLAKE3 library for hashing
#include "blake3.h" // Include BLAKE3 library for hashing

// Define the SHARE_BYTE_COUNT (64 in this context)
constexpr size_t SHARE_BYTE_COUNT = 64;

// SimdBytes struct
struct SimdBytes {
    std::vector<__m512i> bytes;

    // Constructor to create SimdBytes from raw bytes
    static SimdBytes from_bytes(const std::vector<uint8_t>& input_bytes) {
        SimdBytes result;
        size_t chunks = input_bytes.size() / 64;

        for (size_t i = 0; i < chunks; ++i) {
            __m512i chunk = _mm512_loadu_si512(input_bytes.data() + i * 64);
            result.bytes.push_back(chunk);
        }

        return result;
    }

    // Convert SimdBytes back to a flat byte array
    std::vector<uint8_t> to_bytes() const {
        std::vector<uint8_t> flat_bytes;

        for (const auto& chunk : bytes) {
            std::array<uint8_t, 64> temp;
            _mm512_storeu_si512(temp.data(), chunk);
            flat_bytes.insert(flat_bytes.end(), temp.begin(), temp.end());
        }

        return flat_bytes;
    }

    // SIMD-optimized select function
    static SimdBytes select(
        const std::vector<__m512i>& masks,
        const SimdBytes& true_values,
        const SimdBytes& false_values
    ) {
        SimdBytes result;

        for (size_t i = 0; i < masks.size(); ++i) {
            __m512i selected = _mm512_mask_blend_epi8(
                _mm512_movepi8_mask(masks[i]), // Mask
                false_values.bytes[i],         // False values
                true_values.bytes[i]           // True values
            );
            result.bytes.push_back(selected);
        }

        return result;
    }

#ifdef AVX_512
    // Overload ^= (BitXorAssign) for SimdBytes
    SimdBytes& operator^=(const SimdBytes& rhs) {
        for (size_t i = 0; i < bytes.size(); ++i) {
            bytes[i] = _mm512_xor_si512(bytes[i], rhs.bytes[i]);
        }
        return *this;
    }
#else 
#ifdef AVX2
    SimdBytes& operator^=(const SimdBytes& rhs) {
        for (size_t i = 0; i < bytes.size(); ++i) {
            // Use AVX2 XOR instead
            __m256i* lhs_ptr = reinterpret_cast<__m256i*>(&bytes[i]);
            const __m256i* rhs_ptr = reinterpret_cast<const __m256i*>(&rhs.bytes[i]);

            // Process 256-bit chunks
            lhs_ptr[0] = _mm256_xor_si256(lhs_ptr[0], rhs_ptr[0]);
            lhs_ptr[1] = _mm256_xor_si256(lhs_ptr[1], rhs_ptr[1]);
        }
        return *this;
    }
#else
    SimdBytes& operator^=(const SimdBytes& rhs) {
        for (size_t i = 0; i < bytes.size(); ++i) {
            // Use SSE2 XOR
            __m128i* lhs_ptr = reinterpret_cast<__m128i*>(&bytes[i]);
            const __m128i* rhs_ptr = reinterpret_cast<const __m128i*>(&rhs.bytes[i]);

            // Process 128-bit chunks (4 chunks per 512-bit SIMD register)
            lhs_ptr[0] = _mm_xor_si128(lhs_ptr[0], rhs_ptr[0]);
            lhs_ptr[1] = _mm_xor_si128(lhs_ptr[1], rhs_ptr[1]);
            lhs_ptr[2] = _mm_xor_si128(lhs_ptr[2], rhs_ptr[2]);
            lhs_ptr[3] = _mm_xor_si128(lhs_ptr[3], rhs_ptr[3]);
        }
        return *this;
    }
#endif
#endif
};

// Function for XOF (Extendable Output Function) using BLAKE3
SimdBytes xof(const std::vector<uint8_t>& seed, size_t byte_count) {
    // Initialize the BLAKE3 hasher and compute XOF
    blake3_hasher hasher;
    blake3_hasher_init(&hasher);
    blake3_hasher_update(&hasher, seed.data(), seed.size());

    // Finalize the output
    std::vector<uint8_t> expanded_bytes(byte_count, 0);
    blake3_hasher_finalize(&hasher, expanded_bytes.data(), byte_count);

    return SimdBytes::from_bytes(expanded_bytes);
}

// Create a zero share by XORing multiple seeds
SimdBytes create_zero_share(const std::vector<std::array<uint8_t, 16>>& seeds, size_t byte_count) {
    auto it = seeds.begin();

    // Create the initial share from the first seed
    SimdBytes share = xof(std::vector<uint8_t>(it->begin(), it->end()), byte_count);
    ++it;

    // XOR with subsequent XOFs
    for (; it != seeds.end(); ++it) {
        SimdBytes next_share = xof(std::vector<uint8_t>(it->begin(), it->end()), byte_count);
        share ^= next_share;
    }

    return share;
}

// Conditionally corrupt a share based on conditions
SimdBytes conditionally_corrupt_share( const SimdBytes& share, const std::vector<bool>& conditions) {
    // Expand conditions into a mask vector
    std::vector<__m512i> masks;
    for (size_t i = 0; i < conditions.size(); ++i) {
        std::array<uint8_t, 64> mask_chunk = {};
        for (size_t j = 0; j < SHARE_BYTE_COUNT; ++j) {
            mask_chunk[j] = conditions[i] ? 0xFF : 0x00; // Expand to full chunk
        }
        masks.push_back(_mm512_loadu_si512(mask_chunk.data()));
    }

    // Generate randomness using a cryptographic RNG
    std::vector<uint8_t> randomness(SHARE_BYTE_COUNT * conditions.size());
    std::random_device rd;
    //std::generate(randomness.begin(), randomness.end(), rd);
    std::generate(randomness.begin(), randomness.end(), [&rd]() {
        return static_cast<uint8_t>(rd() & 0xFF); // Take only the lowest byte
    });

    SimdBytes randomness_simd = SimdBytes::from_bytes(randomness);

    // Use SIMD select to conditionally corrupt the share
    return SimdBytes::select(masks, randomness_simd, share);
}

