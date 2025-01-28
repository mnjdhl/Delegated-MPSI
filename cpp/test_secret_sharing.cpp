#include <gtest/gtest.h>
#include <vector>
#include <array>
#include <algorithm>
#include "secret_sharing_simd.hpp" // Include your SSE implementation here

TEST(SecretSharingTest, TestSecretShares) {
    // Create zero shares
    SimdBytes share_1 = create_zero_share({{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, 
                                           {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2}}, 
                                           128);
    SimdBytes share_2 = create_zero_share({{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, 
                                           {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3}}, 
                                           128);
    SimdBytes share_3 = create_zero_share({{2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2}, 
                                           {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3}}, 
                                           128);

    // Assertions for zero shares
    std::vector<uint8_t> zero_bytes(128, 0);
    ASSERT_NE(share_1.to_bytes(), zero_bytes);
    ASSERT_EQ(share_1.to_bytes().size(), 128);
    ASSERT_NE(share_2.to_bytes(), zero_bytes);
    ASSERT_EQ(share_2.to_bytes().size(), 128);
    ASSERT_NE(share_3.to_bytes(), zero_bytes);
    ASSERT_EQ(share_3.to_bytes().size(), 128);

    // XOR the shares together
    SimdBytes aggregated = share_1;
    aggregated ^= share_2;
    aggregated ^= share_3;

    // Final result should be zero
    ASSERT_EQ(aggregated.to_bytes(), zero_bytes);
}

TEST(SecretSharingTest, TestCorruptShares) {
    // Create a share with zero bytes
    SimdBytes share = SimdBytes::from_bytes(std::vector<uint8_t>(320, 0));

    // Define conditions
    std::vector<bool> conditions(64, false);
    conditions[1] = true;
    conditions[4] = true;
    conditions[30] = true;
    conditions[31] = true;

    // Corrupt the share based on conditions
    SimdBytes corrupted = conditionally_corrupt_share(share, conditions);

    // Assertions for corruption
    auto share_bytes = share.to_bytes();
    auto corrupted_bytes = corrupted.to_bytes();

    // Check specific ranges
    ASSERT_EQ(std::vector<uint8_t>(share_bytes.begin(), share_bytes.begin() + 5), 
              std::vector<uint8_t>(corrupted_bytes.begin(), corrupted_bytes.begin() + 5));
    ASSERT_NE(std::vector<uint8_t>(share_bytes.begin() + 5, share_bytes.begin() + 10), 
              std::vector<uint8_t>(corrupted_bytes.begin() + 5, corrupted_bytes.begin() + 10));
    ASSERT_EQ(std::vector<uint8_t>(share_bytes.begin() + 10, share_bytes.begin() + 15), 
              std::vector<uint8_t>(corrupted_bytes.begin() + 10, corrupted_bytes.begin() + 15));
    ASSERT_EQ(std::vector<uint8_t>(share_bytes.begin() + 15, share_bytes.begin() + 20), 
              std::vector<uint8_t>(corrupted_bytes.begin() + 15, corrupted_bytes.begin() + 20));
    ASSERT_NE(std::vector<uint8_t>(share_bytes.begin() + 20, share_bytes.begin() + 25), 
              std::vector<uint8_t>(corrupted_bytes.begin() + 20, corrupted_bytes.begin() + 25));
    ASSERT_EQ(std::vector<uint8_t>(share_bytes.begin() + 25, share_bytes.begin() + 30), 
              std::vector<uint8_t>(corrupted_bytes.begin() + 25, corrupted_bytes.begin() + 30));
    ASSERT_EQ(std::vector<uint8_t>(share_bytes.begin() + 30, share_bytes.begin() + 35), 
              std::vector<uint8_t>(corrupted_bytes.begin() + 30, corrupted_bytes.begin() + 35));
    ASSERT_EQ(std::vector<uint8_t>(share_bytes.begin() + 35, share_bytes.begin() + 40), 
              std::vector<uint8_t>(corrupted_bytes.begin() + 35, corrupted_bytes.begin() + 40));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
