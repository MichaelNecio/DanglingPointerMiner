CC  = gcc
CXX = g++
CCFLAGS  = -Wall -Wextra -pthread -Ofast -march=native \
					 -fwhole-program -fipa-pta -fgcse-sm -fgcse-las \
					 -funsafe-loop-optimizations -Wunsafe-loop-optimizations \
					 -funroll-loops
CXXFLAGS = $(CCFLAGS) -std=c++14 -fno-rtti
CPPFLAGS = 	-I dep/rapidjson/include \
			-I dep/uWebSockets/src \ 
			-I src/lib -I src/solvers \
			-L dep/uWebSockets \
			-lz -lssl -lcrypto -lpthread -luWS

all:
	$(MAKE) -C dep
	$(CXX) src/master/master.cpp -o DanglingPointerMiner $(CXXFLAGS) $(CPPFLAGS)

osx:
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) src/master/master.cpp -luv
