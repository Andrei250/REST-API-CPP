SOURCES = helpers.cpp buffer.cpp handler.cpp client.cpp

build:
	g++ $(SOURCES) -o client

run:
	./client

clean:
	rm -rf client
