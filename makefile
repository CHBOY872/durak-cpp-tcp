CXX = g++
CXXFLAGS = -Wall -g

Durak.o: Durak.cpp Durak.hpp
	$(CXX) $(CXXFLAGS) ./Durak.cpp -c

Server.o: Server.cpp Server.hpp Durak.hpp
	$(CXX) $(CXXFLAGS) ./Server.cpp -c

main: Server.hpp main.cpp Server.o Durak.o
	$(CXX) $(CXXFLAGS) ./main.cpp ./Server.o ./Durak.o -o main