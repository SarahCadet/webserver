all : webserv
webserv : webserv.cpp cache.cpp
	g++ -g webserv.cpp -o webserv -lcurl

my-histogram : my-histogram.cgi
	chmod +x my-histogram.cgi

cache : cache.cpp
	g++ -g cache.cpp -o cache