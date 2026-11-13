CC = gcc
CFLAGS = -Wall -Wextra -O2 -lm

SRC = main_neural_network.c neural_network.c image_converter.c png_to_pgm.c
OBJ = $(SRC:.c=.o)
EXEC = result

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(OBJ) -o $(EXEC) $(CFLAGS)

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	rm -f $(OBJ) $(EXEC)

re: clean all

