#ifndef __DANGMINER_SORTED_LIST__
#define __DANGMINER_SORTED_LIST__

#include <openssl/sha.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>
#include <iostream>
#include <random>
#include <string>
#include <thread>

#include "guarded_value.h"

#define TO_HEX_CHAR(c) ((c) < 10 ? '0' + (c) : 'a' + (c)-10)

void custom_to_string(uint64_t n, std::string& buffer) {
  if (n == 0) {
    buffer = "0";
    return;
  }

  buffer.clear();
  while (n != 0) {
    buffer.push_back('0' + (n % 10));
    n /= 10;
  }
  std::reverse(buffer.begin(), buffer.end());
}

uint64_t generate_seed(const uint64_t nonce,
                       const std::string& last_solution_hash,
                       std::string& buffer,
                       unsigned char hash[SHA256_DIGEST_LENGTH]) {
  // buffer = std::to_string(nonce);
  custom_to_string(nonce, buffer);
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

template <typename Comparator>
void solve_sorted_list(const std::string& last_solution_hash,
                       const std::string& hash_prefix, const int n_elements,
                       const std::atomic<bool>& stopped,
                       GuardedValue<uint64_t>& nonce,
                       const uint64_t initial_nonce) {
  std::cout << "Mining from " << std::this_thread::get_id() << std::endl;

  std::string buffer;
  Comparator cmp;
  unsigned char hash[SHA256_DIGEST_LENGTH];
  uint64_t last_nonce = initial_nonce;
  const auto initial_seed =
      generate_seed(last_nonce, last_solution_hash, buffer, hash);

  std::mt19937_64 rng(initial_seed);
  std::vector<std::uint64_t> list(n_elements);

  while (!stopped) {
    for (auto& i : list) {
      i = rng();
    }

    std::sort(list.begin(), list.end(), cmp);

    SHA256_CTX solution_ctx;
    SHA256_Init(&solution_ctx);

    // Generate the solution hash on the fly to avoid continual string
    // concatenation.
    for (int i = 0; i < n_elements; ++i) {
      custom_to_string(list[i], buffer);
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
      nonce.hold();
      nonce.set(last_nonce);
      nonce.drop();
      break;
    }

    // Generate the new seed.
    last_nonce = rng();
    const auto new_seed =
        generate_seed(last_nonce, last_solution_hash, buffer, hash);
    rng.seed(new_seed);
  }
}

#endif
