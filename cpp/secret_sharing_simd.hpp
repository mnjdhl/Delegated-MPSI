// Helper class for SIMD-like operations
class SimdBytes {
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
};

SimdBytes conditionally_corrupt_share( const SimdBytes& share, const std::vector<bool>& conditions);
SimdBytes create_zero_share(const std::vector<std::array<uint8_t, 16>>& seeds, size_t byte_count);
