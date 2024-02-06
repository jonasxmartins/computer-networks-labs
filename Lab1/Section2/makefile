CC=gcc
CFLAGS=-I.

# The all target
all: server deliver

# Compile the server program
server: server/server.o structs/structs.o
	$(CC) -o server/server server/server.o structs/structs.o

# Compile the deliver program
deliver: deliver/deliver.o structs/structs.o 
	$(CC) -o deliver/deliver deliver/deliver.o structs/structs.o -lm

# Pattern rule for compiling .c to .o
server/%.o: server/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

deliver/%.o: deliver/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

structs/%.o: structs/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

# Clean the build
clean:
	rm -f server/*.o deliver/*.o structs/*.o server/server deliver/deliver
