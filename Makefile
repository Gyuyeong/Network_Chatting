CXX = gcc
CXXFLAGS = -Wall -pthread

all: server client

server: server.c utils.c
	$(CXX) $(CXXFLAGS) -o $@ $^

client: client.c utils.c
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm server client