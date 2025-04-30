#include <map>
#include "hash_funcs.hpp"

std::string sha256(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), input.size(), hash);

    std::ostringstream hexStream;
    for (unsigned char byte : hash) {
        hexStream << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
    }
    return hexStream.str();
}

std::vector<uint8_t> sha256(const uint8_t* seed, size_t byte_count) {
    std::vector<uint8_t> hash(SHA256_DIGEST_LENGTH);
    SHA256(seed, byte_count, hash.data());
    return hash;
}

std::vector<uint8_t> sha512(const uint8_t* seed, size_t byte_count) {
    std::vector<uint8_t> hash(SHA512_DIGEST_LENGTH);
    SHA512(seed, byte_count, hash.data());
    return hash;
}

std::string sha3_256(const std::string& input) {
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int length = 0;

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha3_256(), nullptr);
    EVP_DigestUpdate(ctx, input.c_str(), input.size());
    EVP_DigestFinal_ex(ctx, hash, &length);
    EVP_MD_CTX_free(ctx);

    std::ostringstream hexStream;
    for (unsigned int i = 0; i < length; i++) {
        hexStream << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return hexStream.str();
}

std::vector<uint8_t> sha3_256(const uint8_t* seed, size_t byte_count) {
    std::vector<uint8_t> hash(SHA256_DIGEST_LENGTH); // SHA3-256 also has 32 bytes
    EVP_Digest(seed, byte_count, hash.data(), nullptr, EVP_sha3_256(), nullptr);
    return hash;
}

std::vector<uint8_t> sha3_512(const uint8_t* seed, size_t byte_count) {
    std::vector<uint8_t> hash(SHA512_DIGEST_LENGTH);
    EVP_Digest(seed, byte_count, hash.data(), nullptr, EVP_sha3_512(), nullptr);
    return hash;
}

std::vector<uint8_t> blake2b_512(const uint8_t* seed, size_t byte_count) {
    std::vector<uint8_t> hash(EVP_MAX_MD_SIZE);
    EVP_Digest(seed, byte_count, hash.data(), nullptr, EVP_blake2b512(), nullptr);
    return hash;
}

/* SHAKE256 (Extendable-Output Function) provides:
- Arbitrary-length hashes (e.g., 64 bytes, 128 bytes, etc.)
- More flexible than fixed-length SHA-3.
*/
std::vector<uint8_t> shake128_xof_varlen(const uint8_t* seed, size_t byte_count, size_t output_len = 64) {
    std::vector<uint8_t> hash(output_len);
    EVP_Digest(seed, byte_count, hash.data(), nullptr, EVP_shake128(), nullptr);
    return hash;
}

std::vector<uint8_t> shake128_xof_len64(const uint8_t* seed, size_t byte_count) {
    return shake128_xof_varlen(seed, byte_count);
}

std::vector<uint8_t> shake256_xof_varlen(const uint8_t* seed, size_t byte_count, size_t output_len = 64) {
    std::vector<uint8_t> hash(output_len);
    EVP_Digest(seed, byte_count, hash.data(), nullptr, EVP_shake256(), nullptr);
    return hash;
}

std::vector<uint8_t> shake256_xof_len64(const uint8_t* seed, size_t byte_count) {
    return shake256_xof_varlen(seed, byte_count);
}

std::string argon2_hash(const std::string& password) {
    unsigned char hash[crypto_pwhash_STRBYTES];
    if (crypto_pwhash_str(reinterpret_cast<char*>(hash), password.c_str(), password.length(),
                          crypto_pwhash_OPSLIMIT_MODERATE, crypto_pwhash_MEMLIMIT_MODERATE) != 0) {
        throw std::runtime_error("Argon2 hashing failed");
    }
    return std::string(reinterpret_cast<char*>(hash));
}

bool verify_argon2(const std::string& password, const std::string& hash) {
    return crypto_pwhash_str_verify(hash.c_str(), password.c_str(), password.length()) == 0;
}

std::vector<uint8_t> argon2_hash(const uint8_t* seed, size_t byte_count) {
    if (!seed || byte_count == 0) {
        throw std::invalid_argument("Invalid input seed");
    }

    // Define salt with correct size
    unsigned char salt[crypto_pwhash_SALTBYTES];
    randombytes_buf(salt, sizeof(salt));  // Generate random salt

    // Output buffer for the hash
    std::vector<uint8_t> hash(crypto_pwhash_BYTES_MAX); //crypto_pwhash_BYTES);
    //crypto_pwhash_SALTBYTES);
    /*
        Keep in mind that to produce the same key from the same password, the same algorithm, the same salt, 
        and the same values for opslimit and memlimit must be used. Therefore, these parameters must be 
        stored for each user.
    */
    // Compute the Argon2 hash
    if (crypto_pwhash(hash.data(), hash.size(),
                      reinterpret_cast<const char*>(seed), byte_count,
                      salt, 
                      crypto_pwhash_OPSLIMIT_INTERACTIVE, 
                      crypto_pwhash_MEMLIMIT_INTERACTIVE, 
                      crypto_pwhash_ALG_DEFAULT) != 0) {
        throw std::runtime_error("Argon2 hashing failed");
    }

    return hash;
}

// Function for XOF (Extendable Output Function) using BLAKE3
std::vector<uint8_t> blake3_xof(const uint8_t * seed, size_t byte_count) {
    //auto start_time = std::chrono::steady_clock::now();
    // Initialize the BLAKE3 hasher and compute XOF
    blake3_hasher hasher;
    blake3_hasher_init(&hasher);
    blake3_hasher_update(&hasher, seed, byte_count); //size);

    // Finalize the output
    std::vector<uint8_t> expanded_bytes(byte_count, 0);
    blake3_hasher_finalize(&hasher, expanded_bytes.data(), byte_count);
    //auto end_time = std::chrono::steady_clock::now();
    return expanded_bytes;
}

std::map<std::string, hash_function_t> hash_functions = {
    /*{"sha256", sha256},
    {"sha3_256", sha3_256},
    {"argon2", argon2_hash},*/
    {"sha512", sha512}, //Use this
    {"sha3_512", sha3_512}, //Use this
    {"blake2b_512", blake2b_512}, //Use this
    {"shake128_xof", shake128_xof_len64}, //Use this
    {"shake256_xof", shake256_xof_len64}, //Use this
    {"blake3_xof", blake3_xof} //Use this
};

std::vector<uint8_t> generic_hash_func(std::string hf_name, const uint8_t * seed, size_t byte_count) {
    return hash_functions[hf_name](seed, byte_count);
}

bool find_hash_func(std::string hf_name) {
    return hash_functions.find(hf_name) != hash_functions.end();
}
