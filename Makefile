CC  = gcc
CXX = g++
CCFLAGS  = -Wall -Wextra -pthread -static
CXXFLAGS = $(CCFLAGS) -std=c++14
CPPFLAGS = -I dependencies/easywsclient \
			  -I dependencies/rapidjson/include \
			  -I src/lib -I src/solvers

all:
	g++ src/master/master.cpp dep/easywsclient/easywsclient.cpp $(CPPFLAGS) -std=c++1y -lcrypto -Wno-deprecated -Wno-deprecated-declarations -pthread

