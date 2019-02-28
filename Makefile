

#
#
LDFLAGS= -Wall -ansi -pedantic       # -lnsl -lsocket

all: client


client: client.c
	gcc -o client client.c $(LDFLAGS)
clean:
	rm -f client
