LIBS=-lpthread

all: echo-server

echo-server: echo-server.c
	gcc ${LIBS} echo-server.c -o echo-server

clean:
	rm -f echo-server
