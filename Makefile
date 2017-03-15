all:
	g++ master.cpp easywsclient/easywsclient.cpp -std=c++11 -lcrypto -Wno-deprecated -Wno-deprecated-declarations

