#include <vector>
#include <array>
#include <cstdint>
#include <random>
#include <algorithm>
#include "blake3.h" // Include BLAKE3 library for hashing
#include "secret_sharing_simd.hpp"

// Constants
constexpr size_t SHARE_BYTE_COUNT = 64;

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
    result.bytes.resize(masks.size());

    for (size_t i = 0; i < masks.size(); ++i) {
        for (size_t j = 0; j < 4; ++j) {
            result.bytes[i][j] = _mm_blendv_epi8(
                false_values.bytes[i][j], true_values.bytes[i][j], masks[i][j]);
        }
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
SimdBytes xof(const std::array<uint8_t, 16>& seed, size_t byte_count) {
    std::vector<uint8_t> expanded_bytes(byte_count);
    std::random_device rd;
    std::generate(expanded_bytes.begin(), expanded_bytes.end(), [&rd]() {
        return static_cast<uint8_t>(rd() & 0xFF);
    });
    return SimdBytes::from_bytes(expanded_bytes);
}

// Function for XOF (Extendable Output Function) using BLAKE3
//SimdBytes xof(const std::vector<uint8_t>& seed, size_t byte_count) {
SimdBytes blake3_xof(const std::array<uint8_t, 16>& seed, size_t byte_count) {
    // Initialize the BLAKE3 hasher and compute XOF
    blake3_hasher hasher;
    blake3_hasher_init(&hasher);
    blake3_hasher_update(&hasher, seed.data(), seed.size());

    // Finalize the output
    std::vector<uint8_t> expanded_bytes(byte_count, 0);
    blake3_hasher_finalize(&hasher, expanded_bytes.data(), byte_count);

    return SimdBytes::from_bytes(expanded_bytes);
}

// Create Zero Share
SimdBytes create_zero_share(const std::vector<std::array<uint8_t, 16>>& seeds, size_t byte_count) {
    auto seeds_iterator = seeds.begin();
    //SimdBytes share = xof(*seeds_iterator, byte_count);
    SimdBytes share = blake3_xof(*seeds_iterator, byte_count);

    for (++seeds_iterator; seeds_iterator != seeds.end(); ++seeds_iterator) {
        share ^= blake3_xof(*seeds_iterator, byte_count);
    }
    return share;
}

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

    // Generate randomness
    std::vector<uint8_t> randomness(SHARE_BYTE_COUNT * chunk_count);
    std::random_device rd;
    std::generate(randomness.begin(), randomness.end(), [&rd]() {
        return static_cast<uint8_t>(rd() & 0xFF);
    });

    SimdBytes randomness_simd = SimdBytes::from_bytes(randomness);

    // Use SIMD select to conditionally corrupt the share
    return SimdBytes::select(masks, randomness_simd, share);
}
