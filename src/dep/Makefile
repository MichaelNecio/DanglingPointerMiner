OpenSSL_CONFIG = \
		no-shared no-ui no-stdio \
		no-deprecated no-autoalginit no-autoerrinit \
		no-dso no-ec no-ec2m \
		no-engine no-err no-filenames \
		no-gost no-cms no-static-engine \
		no-sock no-srp no-srtp \
		no-psk no-ocsp no-nextprotoneg \
		no-hw-padlock no-tests no-ts \
		no-bf no-blake2 no-camellia \
		no-cast no-chacha no-cmac \
		no-des no-dh no-dsa \
		no-ecdh no-idea no-md4 \
		no-mdc2 no-ocb no-poly1305 \
		no-rc2 no-rc4 no-rmd160 \
		no-scrypt no-seed no-siphash \
		no-whirlpool \
		$(CFLAGS)


all: /uWebSockets/libuWS.o
	cd openssl; ./config $(OpenSSL_CONFIG)
	$(MAKE) -C openssl


/uWebSockets/libuWS.o:
		$(MAKE) -C uWebSockets