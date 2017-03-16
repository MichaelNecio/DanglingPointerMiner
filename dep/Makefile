OpenSSL_CONFIG = no-shared 

all:
	cd openssl; ./config $(OpenSSL_CONFIG)
	$(MAKE) -C openssl
	$(MAKE) -C openssl test
