# DanglingPointerMiner
The pride of UWindsor cryptocurrency mining.

UWindsor Team A's CS Coin miner for the 2017 CS Games.

Use 'git clone --recursive -j[nprocs] [repo]' to clone this repo and all of its submodules.

# Dependencies
* **[Beast](https://github.com/vinniefalco/Beast)** For WebSockets, depends on Boost.System and OpenSSL ([CS Coins server will use Secure Web Sockets](https://github.com/csgames/cscoins/issues/6)).
* **[RapidJSON](https://github.com/miloyip/rapidjson)** For parsing the JSON response, [rapidly](https://github.com/miloyip/nativejson-benchmark#parsing-time).
* **[Boost.Algorithm](https://github.com/boostorg/algorithm)** Has functions to convert integers to hex strings and back.
* **[OpenSSL](https://github.com/openssl/openssl)** Has SHA256 and RSA functions (mt1997_64 is in the STL)

