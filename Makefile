#
#
LDFLAGS= -Wall -ansi -pedantic       

all: client client_test


client: client.c
	gcc -o client client.c $(LDFLAGS)
client_test test t: client_test.c
	gcc -o client_test client_test.c $(LDFLAGS)
e:
	clear
	gcc -o client client.c $(LDFLAGS)
	./client
ed:
	clear
	gcc -o client client.c $(LDFLAGS)
	./client -d

c clear clean:
	rm -f client
	rm -f client_test
