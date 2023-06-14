all: servermain serverA serverB client

servermain: servermain.cpp
	g++ -std=c++11 servermain.cpp -g -o servermain

serverA: serverA.cpp
	g++ -std=c++11 serverA.cpp -g -o serverA

serverB: serverB.cpp
	g++ -std=c++11 serverB.cpp -g -o serverB

client: client.cpp
	g++ -std=c++11 client.cpp -g -o client
clean:
	rm serverA serverB servermain client
