#ifndef HASH_FUNCS_HPP
#define HASH_FUNCS_HPP

#include <openssl/sha.h>
#include <openssl/evp.h>
#include <sodium.h>
#include "blake3.h" // Include BLAKE3 library for hashing
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>


std::string sha256(const std::string& input);
std::vector<uint8_t> sha256(const uint8_t* seed, size_t byte_count);
std::string sha3_256(const std::string& input);
std::vector<uint8_t> sha3_256(const uint8_t* seed, size_t byte_count);
std::string argon2_hash(const std::string& password);
bool verify_argon2(const std::string& password, const std::string& hash);
std::vector<uint8_t> blake3_xof(const uint8_t * seed, size_t byte_count);

typedef std::vector<uint8_t> (*hash_function_t)(const uint8_t* seed, size_t byte_count);

//extern std::map<std::string, hash_function_t> hash_functions;

std::vector<uint8_t> generic_hash_func(std::string, const uint8_t * seed, size_t byte_count);
bool find_hash_func(std::string hf_name);

#endif // HASH_FUNCS_HPP