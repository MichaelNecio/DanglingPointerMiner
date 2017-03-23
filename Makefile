CC  = gcc
CXX = g++
CCFLAGS  = -Wall -Wextra -pthread -O3
CXXFLAGS = $(CCFLAGS) -std=c++14
CPPFLAGS = 	-I dep/rapidjson/include \
			-I dep/uWebSockets/src \
			-I src/lib -I src/solvers \
			-I /usr/local/opt/openssl/include \
			-L /usr/local/opt/openssl/lib \
			-lz -lssl -lcrypto -lpthread -luWS

all:
	$(MAKE) -C dep
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) src/master/master.cpp -static-libstdc++ -o DanglingPointerMiner

osx:
	$(MAKE) -C dep
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) src/master/master.cpp -luv
