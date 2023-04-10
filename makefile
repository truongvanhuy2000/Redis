# Declare phony targets
.PHONY: all clean count

# Define the target executable
output: object/main.o object/Server.o object/timer.o object/helper.o object/request.o object/response.o
	g++ object/main.o object/Server.o object/timer.o object/helper.o object/request.o object/response.o -o output/Server
client:
	g++ client.cpp -o output/client
# Define rules for each object file
object/main.o: main.cpp
	g++ -c main.cpp -o object/main.o

object/Server.o: Server.cpp include/Server.hpp
	g++ -c Server.cpp -o object/Server.o

object/timer.o: timer.cpp include/timer.hpp
	g++ -c timer.cpp -o object/timer.o

object/helper.o: helper.cpp include/helper.hpp
	g++ -c helper.cpp -o object/helper.o

object/request.o: request.cpp include/request.hpp
	g++ -c request.cpp -o object/request.o

object/response.o: response.cpp include/response.hpp
	g++ -c response.cpp -o object/response.o
# Define the clean target to remove object files
clean:
	rm -rf object/*.o