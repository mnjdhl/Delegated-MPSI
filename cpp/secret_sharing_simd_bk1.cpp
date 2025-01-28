#include <vector>
#include <array>
#include <cstdint>
#include <immintrin.h> // For AVX/AVX2/AVX512 intrinsics
#include <cstring>     // For memcpy

// A struct representing a collection of 64-byte SIMD vectors
struct SimdBytes {
    std::vector<__m512i> bytes; // Using 512-bit registers (64 bytes each)

    // Constructor to create SimdBytes from a raw byte array
    static SimdBytes from_bytes(const std::vector<uint8_t>& input_bytes) {
        SimdBytes result;
        size_t chunks = input_bytes.size() / 64;

        for (size_t i = 0; i < chunks; ++i) {
            // Load 64 bytes into a __m512i SIMD register
            __m512i chunk = _mm512_loadu_si512(input_bytes.data() + i * 64);
            result.bytes.push_back(chunk);
        }

        return result;
    }

    // SIMD-optimized select function
    static SimdBytes select(
        const std::vector<__m512i>& masks,
        const SimdBytes& true_values,
        const SimdBytes& false_values
    ) {
        SimdBytes result;

        for (size_t i = 0; i < masks.size(); ++i) {
            // Perform a SIMD blend based on the mask
            __m512i selected = _mm512_mask_blend_epi8(
                _mm512_movepi8_mask(masks[i]), // Mask
                false_values.bytes[i],         // False values
                true_values.bytes[i]           // True values
            );
            result.bytes.push_back(selected);
        }

        return result;
    }

    // Convert the internal representation back to a flat byte array
    std::vector<uint8_t> to_bytes() const {
        std::vector<uint8_t> flat_bytes;

        for (const auto& chunk : bytes) {
            // Store the 64 bytes from the SIMD register into a temporary array
            std::array<uint8_t, 64> temp;
            _mm512_storeu_si512(temp.data(), chunk);

            flat_bytes.insert(flat_bytes.end(), temp.begin(), temp.end());
        }

        return flat_bytes;
    }
};
