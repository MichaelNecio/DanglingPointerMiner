CC  = gcc
CXX = g++
CCFLAGS  = -Wall -Wextra -pthread -Ofast -march=native
CXXFLAGS = $(CCFLAGS) -std=c++14
CPPFLAGS = 	-I dep/rapidjson/include \
			-I dep/uWebSockets/src \
			-I src/lib -I src/solvers \
			-I /usr/local/opt/openssl/include \
			-L /usr/local/opt/openssl/lib \
			-lz -lssl -lcrypto -lpthread -luWS

all:
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) src/master/master.cpp -static-libstdc++ -o DanglingPointerMiner

osx:
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) src/master/master.cpp -luv
