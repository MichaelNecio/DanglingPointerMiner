#ifndef CSCOINS_WALLET_H
#define CSCOINS_WALLET_H

#include <cstdio>
#include <cstring>
#include <exception>
#include <filesystem>
#include <memory>
#include <random>
#include <string>

#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>

namespace cscoins_wallet {

void seed_openssl_RAND() {
  if(RAND_status()) { return; }
  std::random_device rd;
  auto seed = rd();
  while(!RAND_add(&seed, sizeof(seed))) { seed = rd(); }
}

bool keys_are_pair(const RSA& public_key, const RSA& private_key) {
  using std::string;
  seed_openssl_RAND(); // Needed for encrypting
  constexpr string msg = "Files might be modified, must be checked every reading";
  string ciphertext(RSA_size(private_key.get()), '\0');
  size_t nbytes_encrypted = RSA_public_encrypt(msg.size(), msg.data(),
      ciphertext.data(), public_key.get(), RSA_NO_PADDING);
  string decrypted(nbytes_encrypted, '\0');
  RSA_private_decrypt(nbytes_encrypted, ciphertext.data(),
      decrypted.data(), private_key.get(), RSA_NO_PADDING);
  return msg == decrypted;
}

class CSCoinsWallet {
 public:
  CSCoinsWallet(path public_key_file, path private_key_file) {
    using std::filesystem;
    switch(status(public_key_file) & status(private_key_file)) {  // Bitwise and
      case file_type::regular:
        load_keys_from_file(public_key_file, private_key_file)
        break;
      case file_type::not_found:
        // TODO: Generate keys if they don't exist
        break;
      case file_type::unknown:
        throw filesystem_error("Are you sure the persmissions are correct?",
                               public_key_file, private_key_file, errno);
        break;
      default:
        throw filesystem_error("One of those is a directory, or worse.",
                               public_key_file, private_key_file, errno);
        break;
    }
  }

 private:
  std::unique_ptr<RSA*, RSA_free> public_key;
  std::unique_ptr<RSA*, RSA_free> private_key;

  void load_keys_from_file(path public_path, path private_path) {
    const std::unique_ptr<std::FILE *, std::fclose> public_key_file {
      std::fopen(public_path.c_str(), "r");
    };
    if(!public_key_file)
      throw filesystem_error("Error opening public key file.", public_path, errno);
    public_key = PEM_read_RSA_PublicKey(public_key_file.get(),
                                        nullptr, nullptr, nullptr)
    if(!public_key)
      throw filesystem_error("Error reading public key file.", public_path, errno);
    if(public_key->d || public_key->p || public_key->q)  // private key fields
      throw filesystem_error("That's a private key dipshit.", public_path, errno);

    const std::unique_ptr<std::FILE *, std::fclose> private_key_file {
      std::fopen(private_path.c_str(), "r")
    };
    if(!private_key_file)
      throw filesystem_error("Error opening private key file.", private_path, errno);
    private_key = PEM_read_RSA_PrivateKey(private_key_file.get(),
                                          nullptr, nullptr, nullptr);
    if(!private_key)
      throw filesystem_error("Error reading private key file.", private_path, errno);
    if(RSA_check_key(private_key) != 1) {
      auto err = ERR_get_error();
      throw filesystem_error(ERR_error_string(err, nullptr), private_path, errno);
    }
    if(!keys_are_pair(public_key, private_key)
      throw filesystem_error("These public/private keys aren't a pair.",
                             public_path, private_path, errno);
  }
};

}  // cscoins_wallet
#endif