#ifndef CSCOINS_WALLET_H
#define CSCOINS_WALLET_H

#include <filesystem> // This is a C++ 17 standard library header
#include <string>
#include <cstring>
#include <random>
#include <cstdio> // OpenSSL key file reading functions take FILE *
#include <exception>
/* I think an exception is ideal in this case because we might have open resources,
 * the web socket, which won't be cleaned up satisfactorily by just exiting,
 * and I'd like to avoid coupling the web socket handling and the wallet code */

#include <openssl/sha.h>
#include <openssl/rsa.h>
#include <openssl/rand.h>

using std::unique_ptr;
using std::string;

void seed_openssl_RAND() {
  if(RAND_status()) return;

  std::random_device rd;
  auto seed = rd();
  while(!RAND_add(&seed, sizeof(seed))) { // RAND_add returns 1 when it is has been seeded to its satisfaction.
    seed = rd();
  }
}

bool keys_are_pair(const RSA& public_key, const RSA& private_key) {
  seed_openssl_RAND(); // Needed for encrypting

  constexpr string msg( "This absolutely needs to be checked at runtime."
                        "What if key files are modified on the system?");

  string ciphertext(RSA_size(private_key.get()), '\0');
  size_t nbytes_encrypted = RSA_public_encrypt(msg.size(), msg.data(), ciphertext.data(), public_key.get(), RSA_NO_PADDING);
  
  string decrypted(nbytes_encrypted, '\0');
  RSA_private_decrypt(nbytes_encrypted, ciphertext.data(), decrypted.data(), private_key.get(), RSA_NO_PADDING);

  return msg == decrypted;
}

class CSCoinsWallet {
  using namespace std::experimental::filesystem;
  private:
    unique_ptr<RSA*, RSA_free> public_key;
    unique_ptr<RSA*, RSA_free> private_key;
    constexpr string team_name { "Dangling Pointers" };

    void load_keys_from_file(path public_path, path private_path) {
      const unique_ptr<std::FILE *, std::fclose> public_key_file = std::fopen(public_path.c_str(), "r");
      if(!public_key_file)
        throw filesystem_error("Error opening public key file.", public_path, errno);
      public_key = PEM_read_RSA_PublicKey(public_key_file.get(), nullptr, nullptr, nullptr)
      if(!public_key)
        throw filesystem_error("Error reading public key file.", public_path, errno);

      // Make sure this is the public key and not the private one
      if(public_key->d || public_key->p || public_key->q)
        throw filesystem_error("That's a private key dipshit.", public_path, errno);

      const unique_ptr<std::FILE *, std::fclose> private_key_file{std::fopen(private_path.c_str(), "r")};
      if(!private_key_file)
        throw filesystem_error("Error opening private key file.", private_path, errno);
      private_key = PEM_read_RSA_PrivateKey(pubkey_file, nullptr, nullptr, nullptr);
      if(!private_key)
        throw filesystem_error("Error reading private key file.", private_path, errno);

      // Validate the private key, this function only works on private keys
      if(RSA_check_key(private_key) != 1) {
        auto openssl_error_code = ERR_get_error();
        throw filesystem_error(ERR_error_string(openssl_error_code, nullptr), private_path, errno);
      }

      if(!keys_are_pair(public_key, private_key)
        throw filesystem_error("These public/private keys aren't a pair.", public_path, private_path, errno);
    }

  public:
    CSCoinsWallet(path public  = "./.keys/DanglingPointers_rsa.public",
                  path private = "./.keys/DanglingPointers_rsa.private"
    ) {
      // Note the bitwise, not logical, and
      // If the files have two different statuses that's fucked anyway, so we'll handle that in default
      switch(status(public) & status(private)) {
        case file_type::regular :
          load_keys_from_file(public, private)
          break;

        case file_type::not_found :
          generate_keys(public, private);
          break;

        case file_type::unknown :
          throw filesystem_error("Are you sure the persmissions are correct?", public, private, errno);
          break;

        case default :
          throw filesystem_error("One of those is a directory, or worse.", public, private, errno);
          break;
      }
    }
}

#endif