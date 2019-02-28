

#
#
LDFLAGS= -Wall -ansi -pedantic       

all: client


client: client.c
	gcc -o client client.c $(LDFLAGS)
clean:
	rm -f client
