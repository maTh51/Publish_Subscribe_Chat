all:
	gcc -Wall -c common.c
	gcc -Wall client.c common.o -lpthread -o cliente
	gcc -Wall server-mt.c queue.h queue.c common.o -lpthread -o servidor

clean:
	rm common.o cliente servidor