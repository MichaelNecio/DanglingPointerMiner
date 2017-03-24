CC  = gcc
CXX = g++
CCFLAGS  = -Wall -Wextra -pthread -Ofast -march=native \
					 -fwhole-program -fipa-pta -fgcse-sm -fgcse-las \
					 -funsafe-loop-optimizations -Wunsafe-loop-optimizations
CXXFLAGS = $(CCFLAGS) -std=c++14 -fno-rtti
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
