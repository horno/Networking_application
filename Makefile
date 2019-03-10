

#
#
LDFLAGS= -Wall -ansi -pedantic       

all: client


client: client.c
	gcc -o client client.c $(LDFLAGS)
client_test: client_test.c
	gcc -o client_test client_test.c $(LDFLAGS)

clean:
	rm -f client
