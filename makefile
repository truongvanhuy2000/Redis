.PHONY: all clean count
output: main.o echoServer.o
	g++ main.o echoServer.o -o echoServer
main.o: main.cpp
	g++ -c main.cpp
echoServer.o: echoServer.cpp echoServer.hpp helper.hpp
	g++ -c echoServer.cpp
clean:
	rm -rf *.o