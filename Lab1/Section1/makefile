CC=gcc
CFLAGS=-I.

# The all target
all: server deliver

# Compile the server program
server: server/server.o
	$(CC) -o server/server server/server.o

# Compile the deliver program
deliver: deliver/deliver.o
	$(CC) -o deliver/deliver deliver/deliver.o

# Pattern rule for compiling .c to .o
server/%.o: server/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

deliver/%.o: deliver/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

# Clean the build
clean:
	rm -f server/*.o deliver/*.o server/server deliver/deliver
