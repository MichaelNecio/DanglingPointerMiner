#ifndef __DANGMINER_SORTED_LIST__
#define __DANGMINER_SORTED_LIST__

#include <openssl/sha.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <random>
#include <string>

#define TO_HEX_CHAR(c) ((c) < 10 ? '0' + (c) : 'a' + (c)-10)

uint64_t generate_seed(const uint64_t nonce,
                       const std::string& last_solution_hash,
                       std::string& buffer,
                       unsigned char hash[SHA256_DIGEST_LENGTH]) {
  buffer = std::to_string(nonce);
  SHA256_CTX nonce_ctx;
  SHA256_Init(&nonce_ctx);
  SHA256_Update(&nonce_ctx, last_solution_hash.data(),
                last_solution_hash.size());
  SHA256_Update(&nonce_ctx, buffer.data(), buffer.size());
  SHA256_Final(hash, &nonce_ctx);

  uint64_t new_seed = 0;
  std::memcpy(&new_seed, hash, 8);
  return new_seed;
}

uint64_t solve_sorted_list(const std::string& last_solution_hash,
                           const std::string& hash_prefix,
                           const int n_elements) {
  std::cout << last_solution_hash << " " << hash_prefix << " " << n_elements
            << std::endl;
  std::string buffer;
  unsigned char hash[SHA256_DIGEST_LENGTH];
  uint64_t last_nonce = rand();
  const auto initial_seed =
      generate_seed(last_nonce, last_solution_hash, buffer, hash);

  std::mt19937_64 rng(initial_seed);
  std::vector<std::uint64_t> list(n_elements);

  while (true) {
    for (int i = 0; i < n_elements; ++i) {
      list[i] = rng();
    }

    std::sort(list.begin(), list.end());

    SHA256_CTX solution_ctx;
    SHA256_Init(&solution_ctx);

    // Generate the solution hash on the fly to avoid continual string
    // concatenation.
    for (int i = 0; i < n_elements; ++i) {
      // Can we make this faster?  Home made to_string directly into the
      // buffer?
      buffer = std::to_string(list[i]);
      SHA256_Update(&solution_ctx, buffer.data(), buffer.size());
    }

    SHA256_Final(hash, &solution_ctx);

    buffer.clear();
    // TODO: Is the hash_prefix always the same size?
    for (unsigned i = 0; i < 2; ++i) {
      buffer.push_back(TO_HEX_CHAR(hash[i] >> 4));
      buffer.push_back(TO_HEX_CHAR(hash[i] & 0x0F));
    }

    if (buffer == hash_prefix) {
      std::cout << "Found: " << last_nonce << " " << buffer << std::endl;
      return last_nonce;
    }

    // Generate the new seed.
    last_nonce = rng();
    const auto new_seed =
        generate_seed(last_nonce, last_solution_hash, buffer, hash);
    rng.seed(new_seed);
  }
}

#endif
