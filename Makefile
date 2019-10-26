CC=gcc
CFLAGS=-Wall -Werror -c -Wextra -std=c11
PROJECT_SERVER=server
PROJECT_CLIENT=client

all: $(PROJECT_SERVER) $(PROJECT_CLIENT)

$(PROJECT_SERVER): $(PROJECT_SERVER).o
	$(CC) $^ -lnsl -lpthread -o $@
		
$(PROJECT_SERVER).o: $(PROJECT_SERVER).c
	$(CC) $(CFLAGS) $^ -lsocket -lnsl -lpthread -o $@

$(PROJECT_CLIENT): $(PROJECT_CLIENT).o
	$(CC) $^ -o $@
		
$(PROJECT_CLIENT).o: $(PROJECT_CLIENT).c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f *.o $(PROJECT_SERVER)
	rm -f *.o $(PROJECT_CLIENT)
