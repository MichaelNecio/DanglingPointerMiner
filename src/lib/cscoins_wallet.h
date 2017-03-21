#ifndef CSCOINS_WALLET_H
#define CSCOINS_WALLET_H

#include <cstdio>
#include <cstring>
#include <fstream>
#include <memory>
#include <random>
#include <streambuf>
#include <string>
#include <vector>

#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>

namespace cscoins_wallet {
namespace detail {

template <typename T>
void CHECK(const T& b, const std::string& message) {
  if (!b) {
    perror("Check fail: ");
    std::cerr << message << std::endl;
    abort();
  }
}

struct RSADeleter {
  void operator()(RSA* rsa) const { RSA_free(rsa); }
};

struct FileDeleter {
  void operator()(FILE* f) const { fclose(f); }
};

using RSAPtr = std::unique_ptr<RSA, detail::RSADeleter>;
using FilePtr = std::unique_ptr<FILE, detail::FileDeleter>;

void seed_openssl_RAND() {
  if (RAND_status()) {
    return;
  }
  std::random_device rd;
  auto seed = rd();
  RAND_seed(&seed, sizeof(seed));
}

bool keys_are_pair(const RSAPtr& public_key, const RSAPtr& private_key) {
  seed_openssl_RAND();  // Needed for encrypting

  const std::string m =
      "Files might be modified, must be checked every reading";
  std::vector<uint8_t> msg(RSA_size(private_key.get()), 0);
  std::copy(m.begin(), m.end(), msg.begin());

  std::vector<uint8_t> ciphertext(RSA_size(private_key.get()), 0);
  const auto nbytes_encrypted =
      RSA_public_encrypt(msg.size(), msg.data(), ciphertext.data(),
                         public_key.get(), RSA_NO_PADDING);
  CHECK(nbytes_encrypted > 0, "error encrypting with public key");
  std::vector<uint8_t> decrypted(nbytes_encrypted, '\0');
  RSA_private_decrypt(nbytes_encrypted, ciphertext.data(), decrypted.data(),
                      private_key.get(), RSA_NO_PADDING);
  return msg == decrypted;
}

}  // namespace detail

class CSCoinsWallet {
 public:
  // TODO: Method to sign strings
  // TODO: Getter method for wallet id

  CSCoinsWallet(const std::string& public_key_file,
                const std::string& private_key_file,
                const std::string& der_file, const std::string& team_name) {
    load_keys_from_file(public_key_file, private_key_file);
    generate_wallet_id(der_file);
  }

  const std::string& wallet_id() const { return wallet_id_; }
  const std::string& public_key() const { return public_key_str_; }

  std::string stringify(const unsigned char* digest,
                        const unsigned int len) const {
    const auto f = [](char c) { return c < 10 ? '0' + c : 'a' + (c - 10); };
    std::string stringified;
    for (unsigned i = 0; i < len; ++i) {
      stringified.push_back(f(digest[i] >> 4));
      stringified.push_back(f(digest[i] & 0x0F));
    }
    return stringified;
  }

  std::string sign_digest(
      const unsigned char digest[SHA256_DIGEST_LENGTH]) const {
    std::vector<unsigned char> signature(RSA_size(private_key_.get()), 0);
    unsigned int siglen = 0;
    RSA_sign(NID_sha256, digest, SHA256_DIGEST_LENGTH, signature.data(),
             &siglen, private_key_.get());
    return stringify(signature.data(), siglen);
  }

  std::string sign_str(const std::string& msg) const {
    std::vector<unsigned char> raw(msg.begin(), msg.end());
    const auto* digest = SHA256(raw.data(), raw.size(), nullptr);
    return sign_digest(digest);
  }

  std::string register_sig;

 private:
  detail::RSAPtr public_key_;
  detail::RSAPtr private_key_;
  std::string wallet_id_;
  std::string public_key_str_;

  void generate_wallet_id(const std::string& public_der_path) {
    std::ifstream public_der_file(public_der_path);
    unsigned char hash[SHA256_DIGEST_LENGTH];
    char buffer[1000];

    SHA256_CTX wallet_id_ctx;
    SHA256_Init(&wallet_id_ctx);

    while (public_der_file) {
      public_der_file.read(buffer, 1000);
      SHA256_Update(&wallet_id_ctx, buffer, public_der_file.gcount());
    }

    SHA256_Final(hash, &wallet_id_ctx);

    register_sig = sign_digest(hash);

    const auto f = [](char c) { return c < 10 ? '0' + c : 'a' + (c - 10); };
    for (unsigned i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
      wallet_id_.push_back(f(hash[i] >> 4));
      wallet_id_.push_back(f(hash[i] & 0x0F));
    }
  }

  void load_keys_from_file(const std::string& public_path,
                           const std::string& private_path) {
    using detail::CHECK;
    detail::FilePtr public_key_file{fopen(public_path.c_str(), "r")};
    CHECK(public_key_file, "Error opening public key");

    public_key_.reset(
        PEM_read_RSA_PUBKEY(public_key_file.get(), nullptr, nullptr, nullptr));
    CHECK(public_key_, "Error reading public key file");

    CHECK(!(public_key_->d || public_key_->p || public_key_->q),
          "Thats a private key");

    detail::FilePtr private_key_file{fopen(private_path.c_str(), "r")};
    CHECK(private_key_file, "Error opening private key file.");

    private_key_.reset(PEM_read_RSAPrivateKey(private_key_file.get(), nullptr,
                                              nullptr, nullptr));
    CHECK(private_key_, "Error reading private key file.");

    if (RSA_check_key(private_key_.get()) <= 0) {
      CHECK(false, "Bad private key");
    }

    CHECK(keys_are_pair(public_key_, private_key_),
          "These public/private keys aren't a pair.");

    std::fstream s(public_path);
    std::noskipws(s);
    public_key_str_ = std::string(std::istream_iterator<char>(s),
                                  std::istream_iterator<char>());
  }
};

}  // namespace cscoins_wallet
#endif
