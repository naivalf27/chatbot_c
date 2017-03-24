install: serv cli

start_server: serv
	./server.out

start_client: cli
	./client.out
	
serv: server.c
	gcc server.c -o server.out -lpthread

cli: client.c
	gcc client.c -o client.out

clean:
	rm -rf *.out