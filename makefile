.PHONY: all clean count
output: main.o Server.o
	g++ main.o Server.o -o Server
main.o: main.cpp
	g++ -c main.cpp
Server.o: Server.cpp Server.hpp helper.hpp
	g++ -c Server.cpp
clean:
	rm -rf *.o