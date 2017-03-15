all:
	g++ master.cpp easywsclient/easywsclient.cpp -std=c++1y -lcrypto -Wno-deprecated -Wno-deprecated-declarations -pthread

