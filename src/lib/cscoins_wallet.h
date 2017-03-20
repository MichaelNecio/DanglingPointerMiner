#ifndef CSCOINS_WALLET_H
#define CSCOINS_WALLET_H

#include <cstdio>
#include <cstring>
#include <memory>
#include <random>
#include <string>
#include <vector>
#include <fstream>
#include <streambuf>

#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>

namespace cscoins_wallet {
namespace detail {

template <typename T>
void CHECK(const T& b, const std::string& message) {
  if (!b) {
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
  std::vector<uint8_t> msg(m.begin(), m.end());

  std::vector<uint8_t> ciphertext(RSA_size(private_key.get()), '\0');
  const auto nbytes_encrypted =
      RSA_public_encrypt(msg.size(), msg.data(), ciphertext.data(),
                         public_key.get(), RSA_NO_PADDING);
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
                const std::string& team_name) {
    load_keys_from_file(public_key_file, private_key_file);
    generate_wallet_id(public_key_file, team_name);
  }

  std::string sign_str(const std::string& msg) {
    std::string signed_str(RSA_size(private_key_.get(), RSA_PKCS1_PADDING), '\0');
    //TODO: Figure out what format they want for this shit
  }

 private:
  detail::RSAPtr public_key_;
  detail::RSAPtr private_key_;
  std::string wallet_id_;

  void generate_wallet_id(const std::string& public_path, const std::string& team_name) {
    std::ifstream public_key_file(public_path);
    std::string public_key_str((std::istreambuf_iterator<char>(public_key_file)),
                                std::istreambuf_iterator<char>());
    std::string temp;
    temp = team_name + "," + public_key_str;
    //TODO: Sign this message and save the message as the wallet_id_
  }

  void load_keys_from_file(const std::string& public_path,
                           const std::string& private_path) {
    using detail::CHECK;
    detail::FilePtr public_key_file{fopen(public_path.c_str(), "r")};
    CHECK(public_key_file, "Error opening public key");

    public_key_.reset(PEM_read_RSAPublicKey(public_key_file.get(), nullptr,
                                            nullptr, nullptr));
    CHECK(public_key_, "Error reading public key file");

    CHECK(public_key_->d || public_key_->p || public_key_->q,
          "Thats a private key");

    detail::FilePtr private_key_file{fopen(private_path.c_str(), "r")};
    CHECK(private_key_file, "Error opening private key file.");

    private_key_.reset(PEM_read_RSAPrivateKey(private_key_file.get(), nullptr,
                                              nullptr, nullptr));
    CHECK(private_key_, "Error reading private key file.");

    if (RSA_check_key(private_key_.get())) {
      CHECK(false, "Bad private key");
    }

    CHECK(keys_are_pair(public_key_, private_key_),
          "These public/private keys aren't a pair.");
  }
};

}  // namespace cscoins_wallet
#endif
