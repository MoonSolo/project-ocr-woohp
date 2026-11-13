# === Makefile for process_dataset ===

CC = gcc
CFLAGS = -O2 -Wall
LDFLAGS = -lm
TARGET = process_dataset
SRC = process_dataset.c
OUTDIR = datas

.PHONY: all clean run

all: $(TARGET) | $(OUTDIR)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)
	@echo "✅ Compilation complete."

$(OUTDIR):
	mkdir -p $(OUTDIR)
	@echo "📁 Created folder: $(OUTDIR)"

run: all
	./$(TARGET) dataset

clean:
	rm -rf $(OUTDIR) $(TARGET)
	@echo "🧹 Cleaned: removed $(OUTDIR)/ and $(TARGET)"

