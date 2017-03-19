all: ../../../bin/zlib/lib/libz.a
	cd openssl; ./config --prefix=../../../bin/openssl no-deprecated \
		zlib --with-zlib-include=../../../bin/zlib/include \
		--with-zlib-lib=../../../bin/zlib/lib \
		-O3 -flto -ffat-lto-objects -march=native -static
	$(MAKE) -C openssl
	$(MAKE) -C openssl install

../../../bin/zlib/lib/libz.a: zlib
	cd libz; prefix=../../../bin/zlib CC=gcc CFLAGS="-O3 -flto -ffat-lto-objects -march=native -static"
	$(MAKE) -C zlib
	$(MAKE) -C zlib install
