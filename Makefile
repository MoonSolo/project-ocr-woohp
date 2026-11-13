CC = gcc

# Include paths
TREATMENT_DIR = project/Image_Handler/Treatment
GRID_DET_DIR = $(TREATMENT_DIR)/grid_detection

CFLAGS = -Wall -Wextra -O3 `pkg-config --cflags MagickWand` -I. -I$(TREATMENT_DIR) -I$(GRID_DET_DIR)
LDFLAGS = `pkg-config --libs MagickWand` -lm

# Source files
SRCS = resolution.c \
       solver.c \
       $(GRID_DET_DIR)/grid_detect.c \
       $(GRID_DET_DIR)/histogram.c \
       $(GRID_DET_DIR)/scoring.c \
       $(GRID_DET_DIR)/edge_trimming.c \
       $(TREATMENT_DIR)/grid_size.c 

OBJS = $(SRCS:.c=.o)

# CHANGED: Executable name is now "resolution"
EXEC = resolution

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(OBJS) -o $(EXEC) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(EXEC)

.PHONY: all clean

