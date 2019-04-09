

#
#
LDFLAGS= -Wall -ansi -pedantic       

all: client client_test


client: client.c
	gcc -o client client.c $(LDFLAGS)
client_test test: client_test.c
	gcc -o client_test client_test.c $(LDFLAGS)

c clear clean:
	rm -f client
	rm -f client_test
