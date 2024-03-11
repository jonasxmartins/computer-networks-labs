CC=gcc
CFLAGS=-I.
LDFLAGS=-pthread

# The all target
all: server client

# Compile the server program
server: server/server.o
	$(CC) -o server/server server/server.o

# Compile the client program
client: client/client.o
	$(CC) -o client/client client/client.o

# Pattern rule for compiling .c to .o
server/%.o: server/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

client/%.o: client/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

# Clean the build
clean:
	rm -f server/*.o client/*.o server/server client/client