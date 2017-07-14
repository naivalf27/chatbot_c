install: serv cli

start_server: serv
	./server.out

start_client: cli
	./client.out

serv: server.c
	gcc -g -Wall -I/usr/include/libxml2 server.c -o server.out -lpthread -lxml2

cli: client.c
	gcc client.c -o client.out

clean:
	rm -rf *.out
