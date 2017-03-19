../../../bin/uWebSockets/lib/libuWS.a:  ../../../bin/openssl/lib/libcrypto.a \
																				../../../bin/zlib/lib/libz.a uWebSockets
	cd uWebSockets; g++ -O3 -flto -ffat-lto-objects -static -march=native \
											-I src -I ../../../bin/zlib/include -I ../../../bin/openssl/include/openssl \
											../../../bin/zlib/lib/libz.a \
											../../../bin/openssl/lib/libcrypto.a ../../../bin/openssl/lib/libssl.a \
											src/Extensions.cpp src/Group.cpp \
											src/WebSocketImpl.cpp src/Networking.cpp \
											src/Hub.cpp src/Node.cpp src/WebSocket.cpp \
											src/HTTPSocket.cpp src/Socket.cpp src/Epoll.cpp \
											 -c -o ../../../bin/uWebSockets/lib/libuWS.o

../../../bin/openssl/lib/libcrypto.a: ../../../bin/zlib/lib/libz.a openssl
	cd openssl; ./config --prefix="$$PWD/../../../bin/openssl" no-deprecated \
		zlib --with-zlib-include=../../../bin/zlib/include \
		--with-zlib-lib=../../../bin/zlib/lib \
		-O3 -flto -ffat-lto-objects -static -march=native 
	$(MAKE) -C openssl
	$(MAKE) -C openssl test
	$(MAKE) -C openssl install

../../../bin/zlib/lib/libz.a: zlib
	cd zlib; prefix=../../../bin/zlib CC=gcc CFLAGS="-O3 -flto -ffat-lto-objects -march=native -static" ./configure --static
	$(MAKE) -C zlib
	$(MAKE) -C zlib test
	$(MAKE) -C zlib install
