OpenSSL_CONFIG = \
		no-shared no-ui no-stdio no-deprecated


all: /uWebSockets/libuWS.o
	cd openssl; ./config $(OpenSSL_CONFIG)
	$(MAKE) -C openssl


/uWebSockets/libuWS.o:
		$(MAKE) -C uWebSockets