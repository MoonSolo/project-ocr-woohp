CC = gcc
CFLAGS = -Wall -O2
LDFLAGS = -lm
TARGET = neural_network_nxor
SRCS = neural_network_nxor.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) $(LDFLAGS)

clean:
	rm -f $(TARGET)
