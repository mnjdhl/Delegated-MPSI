#include <vector>
#include <array>
#include <random>
#include <cstdint>
#include <cstring> // For memcpy
#include <x86intrin.h> // Example for SIMD intrinsics (AVX2/AVX512)

// Constant for SHARE_BYTE_COUNT (add the value or import from elsewhere)
constexpr size_t SHARE_BYTE_COUNT = 64; 

// A struct representing a 64-byte SIMD vector
struct SimdBytes {
    std::vector<std::array<uint8_t, SHARE_BYTE_COUNT>> bytes;

    
    // Constructor to create SimdBytes from a raw byte array
    static SimdBytes from_bytes(const std::vector<uint8_t>& input_bytes) {
        SimdBytes result;

        size_t chunks = input_bytes.size() / 64;
        for (size_t i = 0; i < chunks; ++i) {
            std::array<uint8_t, 64> chunk;
            std::memcpy(chunk.data(), input_bytes.data() + i * 64, 64);
            result.bytes.push_back(chunk);
        }

        return result;
    }

    // Select function based on masks
    static SimdBytes select(
        const std::vector<std::array<uint8_t, 64>>& masks,
        const SimdBytes& true_values,
        const SimdBytes& false_values
    ) {
        SimdBytes result;

        for (size_t i = 0; i < masks.size(); ++i) {
            std::array<uint8_t, 64> selected_chunk;
            const auto& mask = masks[i];
            const auto& true_chunk = true_values.bytes[i];
            const auto& false_chunk = false_values.bytes[i];

            for (size_t j = 0; j < 64; ++j) {
                selected_chunk[j] = mask[j] ? true_chunk[j] : false_chunk[j];
            }

            result.bytes.push_back(selected_chunk);
        }

        return result;
    }

    // Convert the internal representation back to a flat byte array
    std::vector<uint8_t> to_bytes() const {
        std::vector<uint8_t> flat_bytes;

        for (const auto& chunk : bytes) {
            flat_bytes.insert(flat_bytes.end(), chunk.begin(), chunk.end());
        }

        return flat_bytes;
    }
};
