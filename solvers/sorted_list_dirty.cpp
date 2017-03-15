#include <openssl/sha.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <random>
#include <string>

#define LAST_SOLUTION_HASH \
  "0000000000000000000000000000000000000000000000000000000000000000"
#define HASH_PREFIX "9098"
#define NB_ELEMENTS 20
#define TO_HEX_CHAR(c) ((c) < 10 ? '0' + (c) : 'a' + (c)-10)

int main() {
  std::ios_base::sync_with_stdio(false);

  // TODO Consider making command line args.
  const std::string last_solution_hash = LAST_SOLUTION_HASH;
  const std::string hash_prefix = HASH_PREFIX;

  // TODO: change this seed.
  std::mt19937_64 nonce_generator(0x170924960fef31e5);
  std::uniform_int_distribution<std::uint64_t> rng;

  std::array<std::uint64_t, NB_ELEMENTS> list;
  std::string buffer;
  unsigned char hash[SHA256_DIGEST_LENGTH];

  uint64_t last_nonce = 0;

  while (true) {
    for (int i = 0; i < NB_ELEMENTS; ++i) {
      list[i] = rng(nonce_generator);
    }

    std::sort(list.begin(), list.end());

    SHA256_CTX solution_ctx;
    SHA256_Init(&solution_ctx);

    // Generate the solution hash on the fly to avoid continual string
    // concatenation.
    for (int i = 0; i < NB_ELEMENTS; ++i) {
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
      std::cout << "Got it! " << last_nonce << std::endl;
      break;
    }

    // Generate the new seed.
    last_nonce = rng(nonce_generator);
    buffer = std::to_string(last_nonce);
    SHA256_CTX nonce_ctx;
    SHA256_Init(&nonce_ctx);
    SHA256_Update(&nonce_ctx, last_solution_hash.data(),
                  last_solution_hash.size());
    SHA256_Update(&nonce_ctx, buffer.data(), buffer.size());
    SHA256_Final(hash, &nonce_ctx);

    uint64_t new_seed = 0;
    std::memcpy(&new_seed, hash, 8);

    nonce_generator.seed(new_seed);
  }
}
