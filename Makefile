CC = cc
CFLAGS = -Wall -O2
LDFLAGS =

all: dnsred
.c.o:
	$(CC) -c $(CFLAGS) $<
dnsred: dnsred.o
	$(CC) -o $@ $^ $(LDFLAGS)
clean:
	rm -f *.o dnsred
