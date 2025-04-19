#ifndef SECRET_SHARING_SIMD_HPP
#define SECRET_SHARING_SIMD_HPP

#include <emmintrin.h> // SSE2 intrinsics
#include <smmintrin.h> // SSE4.1 intrinsics (for blendv)
#include <vector>
#include <array>
#include <cstdint>
#include <algorithm>

// Constants
constexpr size_t SHARE_BYTE_COUNT = 40; // 64;
constexpr size_t RAND_SECRET_SIZE = 16; //SHARE_BYTE_COUNT ~5; // original- 16;

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
    //std::vector<std::array<__m128i, 4>> bytes; // Each 512-bit chunk is split into 4x128-bit chunks
    //std::string hash_func;
    public:
    std::vector<uint8_t> bytes;

    #if 0
    // Default constructor
    /*SimdBytes() {
        hash_func = "blake3_xof";
    }
    SimdBytes(std::string hash_func) : hash_func(hash_func) {}
    */
   void resize(size_t byte_count);
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
    #endif

    SimdBytes() = default;

    SimdBytes(size_t byte_count);
    SimdBytes(size_t byte_count, uint8_t value);

    void resize(size_t byte_count);

    size_t size() const;

    void fill(uint8_t value);

    // Convert raw bytes into SimdBytes
    static SimdBytes from_bytes(const std::vector<uint8_t>& data);
    // Convert SimdBytes back to raw bytes
    std::vector<uint8_t> to_bytes() const;

    // Convert SimdBytes into byte chunks
    template <const size_t ChunkSize>
    std::vector<std::array<uint8_t, ChunkSize>> to_byte_chunks() const {
        std::vector<std::array<uint8_t, ChunkSize>> result;

        // Ensure bytes.size() is a multiple of ChunkSize
        assert(bytes.size() % ChunkSize == 0 && "Bytes size must be divisible by chunk size");

        size_t num_chunks = bytes.size() / ChunkSize;
        result.reserve(num_chunks);

        for (size_t i = 0; i < num_chunks; ++i) {
            std::array<uint8_t, ChunkSize> chunk{};
            std::copy_n(bytes.begin() + i * ChunkSize, ChunkSize, chunk.begin());
            result.push_back(chunk);
        }

        return result;
    }
    // XOR operation for SimdBytes
    SimdBytes& operator^=(const SimdBytes& other);

    SimdBytes operator^(const SimdBytes& other) const;

    bool operator==(const SimdBytes& other) const;

    SimdBytes select(const std::vector<uint8_t>& mask, const SimdBytes& true_values, const SimdBytes& false_values);
};

SimdBytes blake3_xof(const std::array<uint8_t, RAND_SECRET_SIZE>& seed, size_t byte_count);
SimdBytes do_generic_hash(const std::array<uint8_t, RAND_SECRET_SIZE>& seed, size_t byte_count);
//SimdBytes conditionally_corrupt_share( const SimdBytes& share, const std::vector<bool>& conditions);

SimdBytes create_zero_share_no_resize(const std::vector<std::array<uint8_t, RAND_SECRET_SIZE>>& seeds, size_t byte_count, std::string hash_func);
SimdBytes create_zero_share(const std::vector<std::array<uint8_t, RAND_SECRET_SIZE>>& seeds, size_t byte_count, std::string hash_func);

SimdBytes create_zero_share_parallel(
    const std::vector<std::array<uint8_t, RAND_SECRET_SIZE>>& seeds,
    size_t byte_count,
    std::string hash_func
);

SimdBytes conditionally_corrupt_share(
    const SimdBytes& share,
    const std::vector<bool>& conditions,
    size_t chunk_size=SHARE_BYTE_COUNT  // Usually SHARE_BYTE_COUNT
);

SimdBytes conditionally_corrupt_share_parallel(
    const SimdBytes& share,
    const std::vector<bool>& conditions,
    size_t chunk_size=SHARE_BYTE_COUNT  // Usually SHARE_BYTE_COUNT
);
#endif // SECRET_SHARING_SIMD_HPP