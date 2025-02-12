#ifndef SECRET_SHARING_SIMD_HPP
#define SECRET_SHARING_SIMD_HPP

#include <emmintrin.h> // SSE2 intrinsics
#include <smmintrin.h> // SSE4.1 intrinsics (for blendv)
#include <vector>
#include <array>
#include <cstdint>

// Helper class for SIMD-like operations
/*class SimdBytes {
public:
    std::vector<uint8_t> bytes;

    explicit SimdBytes(size_t size) : bytes(size, 0) {}

    explicit SimdBytes(const std::vector<uint8_t>& data) : bytes(data) {}

    std::vector<uint8_t> to_bytes() const {
        return bytes;
    }

    SimdBytes& operator^=(const SimdBytes& other) {
        for (size_t i = 0; i < bytes.size(); ++i) {
            bytes[i] ^= other.bytes[i];
        }
        return *this;
    }

    static SimdBytes from_bytes(const std::vector<uint8_t>& data) {
        return SimdBytes(data);
    }
};*/

class SimdBytes {
    std::vector<std::array<__m128i, 4>> bytes; // Each 512-bit chunk is split into 4x128-bit chunks
    //std::string hash_func;

    public:
    // Default constructor
    /*SimdBytes() {
        hash_func = "blake3_xof";
    }
    SimdBytes(std::string hash_func) : hash_func(hash_func) {}
    */
    // Convert raw bytes into SimdBytes
    static SimdBytes from_bytes(const std::vector<uint8_t>& data);

    // Convert SimdBytes back to raw bytes
    std::vector<uint8_t> to_bytes() const;

    // XOR operation for SimdBytes
    SimdBytes& operator^=(const SimdBytes& rhs);

    // Conditional selection
    static SimdBytes select( const std::vector<std::array<__m128i, 4>>& masks, const SimdBytes& true_values, const SimdBytes& false_values);

    // Convert SimdBytes into byte chunks
    /*
    template <size_t ChunkSize>
     std::vector<std::array<uint8_t, ChunkSize>> to_byte_chunks() const;
     */

     // Convert SimdBytes into byte chunks
template <const size_t ChunkSize>
    std::vector<std::array<uint8_t, ChunkSize>> to_byte_chunks() const {
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
}
};

SimdBytes blake3_xof(const std::array<uint8_t, 16>& seed, size_t byte_count);
SimdBytes do_generic_hash(const std::array<uint8_t, 16>& seed, size_t byte_count);
SimdBytes conditionally_corrupt_share( const SimdBytes& share, const std::vector<bool>& conditions);
SimdBytes create_zero_share(const std::vector<std::array<uint8_t, 16>>& seeds, size_t byte_count, std::string hash_func);

#endif // SECRET_SHARING_SIMD_HPP