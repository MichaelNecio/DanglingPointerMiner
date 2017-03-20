CC  = gcc
CXX = g++
CCFLAGS  = -Wall -Wextra -pthread
CXXFLAGS = $(CCFLAGS) -std=c++14
CPPFLAGS = 	-I dep/rapidjson/include \
			-I dep/uWebSockets/src \
			-I src/lib -I src/solvers \
			-I /usr/local/opt/openssl/include \
			-L /usr/local/opt/openssl/lib \
			-lz -lssl -lcrypto -L dep/uWebSockets -luWS

all:
	clang++ $(CPPFLAGS) -lcrypto -lpthread src/master/master.cpp -std=c++14 -O3

osx:
	g++ $(CPPFLAGS) -lcrypto -lpthread src/master/master.cpp -std=c++14 -O3 -luv
