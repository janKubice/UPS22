# Použitý překladač
CC = gcc
CFLAGS = -Wall -Wextra -pedantic -ansi -pthread

# Binární soubory projektu
BIN = server.exe
OBJ = server.o 

# Návody pro sestavení projektu
$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $(BIN) $(CFLAGS)

main.o: server.c
	gcc -c server.c $(CFLAGS)

clean:
	del -f $(BIN) $(OBJ)
