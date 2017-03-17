CC  = gcc
CXX = g++
CCFLAGS  = -Wall -Wextra -pthread
CXXFLAGS = $(CCFLAGS) -std=c++14
CPPFLAGS = -I dep/rapidjson/include \
			  -I src/lib -I src/solvers \
				-I /usr/local/opt/openssl/include \
				-L /usr/local/opt/openssl/lib \
				-luv -lz -lssl -lcrypto dep/uWebSockets/libuWS.dylib

all:
	g++ src/master/master.cpp $(CPPFLAGS) -std=c++1y -lcrypto -lpthread -O3
