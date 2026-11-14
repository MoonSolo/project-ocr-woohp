CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -g
PKGS = gtk+-3.0
PKGCONFIG = $(shell pkg-config --cflags --libs $(PKGS))

SRCS = rotate.c image_ops.c gui.c
TARGET = gui

IMAGE_HANDLER_DIR = ../Image_Handler
NN_DIR = ../Neural_network
WORKING_DIR = ../WORKING
DEBUG_DIR = DEBUG
RESOLUTION_DIR = ../../

.PHONY: all clean image_handler neural_network solver resolution

# --------------------------------------------------
# Global build
# --------------------------------------------------
all: $(WORKING_DIR) image_handler neural_network solver resolution $(TARGET)

# --------------------------------------------------
# External components
# --------------------------------------------------
image_handler:
	@echo "Building Image_Handler (debugger)..."
	$(MAKE) -C $(IMAGE_HANDLER_DIR)

neural_network:
	@echo "Building Neural_Network..."
	$(MAKE) -C $(NN_DIR)

#solver:
#	@echo "Building solver..."
#	$(MAKE) -C $(RESOLUTION_DIR) solver

resolution:
	@echo "Building resolution..."
	$(MAKE) -C $(RESOLUTION_DIR) resolution

# --------------------------------------------------
# GUI build
# --------------------------------------------------
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) $(PKGCONFIG) -lm

# --------------------------------------------------
# Working directory
# --------------------------------------------------
$(WORKING_DIR):
	@mkdir -p $(WORKING_DIR)
	@echo "Created working directory."

# --------------------------------------------------
# Cleanup
# --------------------------------------------------
clean:
	rm -rf $(DEBUG_DIR)
	rm -rf $(WORKING_DIR)
	rm -f $(TARGET) *.o *.d
	$(MAKE) -C $(IMAGE_HANDLER_DIR) clean
	$(MAKE) -C $(NN_DIR) clean
	$(MAKE) -C $(RESOLUTION_DIR) clean
	@echo "Cleaned everything."

