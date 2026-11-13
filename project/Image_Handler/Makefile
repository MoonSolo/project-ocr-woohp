
CC       ?= gcc
PKG      ?= pkg-config
STD      ?= -std=c11

MAGICKWAND_PC ?= MagickWand

PKG_CFLAGS := $(shell $(PKG) --cflags $(MAGICKWAND_PC) 2>/dev/null)
PKG_LIBS   := $(shell $(PKG) --libs   $(MAGICKWAND_PC) 2>/dev/null)

WARNFLAGS = -Wall -Wextra -Wpedantic
OPTFLAGS ?= -O2
DBGFLAGS ?= -g
CFLAGS   ?= $(STD) $(WARNFLAGS) $(OPTFLAGS) $(DBGFLAGS) $(PKG_CFLAGS)
LDFLAGS  ?=
LDLIBS   ?= $(PKG_LIBS) -lm

DEPFLAGS = -MMD -MP

PRETREATMENT_SRC      := $(wildcard Pretreatment/*.c)
GRID_DETECTION_SRC    := $(wildcard Treatment/grid_detection/*.c)
TREATMENT_SRC         := Treatment/grid_size.c \
                         Treatment/cell_extract.c
WORD_DETECTION_SRC    := Treatment/word_detection/extract_word.c \
                         Treatment/word_detection/word_segmentation.c \
                         Treatment/word_detection/projection.c \
                         Treatment/word_detection/row_processing.c \
                         Treatment/word_detection/word_merging.c \
                         Treatment/word_detection/tall_row_processing.c \
                         Treatment/word_detection/letter_segmentation.c

ALL_SRC := $(PRETREATMENT_SRC) \
           $(GRID_DETECTION_SRC) \
           $(TREATMENT_SRC) \
           $(WORD_DETECTION_SRC)

ALL_OBJ := $(ALL_SRC:.c=.o)

DEBUGGER_SRC := debugger.c
DEBUGGER_OBJ := $(DEBUGGER_SRC:.c=.o)

TARGET := debugger

all: $(TARGET)

$(TARGET): $(ALL_OBJ) $(DEBUGGER_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

clean:
	rm -f $(ALL_OBJ) $(DEBUGGER_OBJ) $(TARGET)
	rm -f $(ALL_OBJ:.o=.d) $(DEBUGGER_OBJ:.o=.d)
	rm -rf DEBUG/

.PHONY: all clean

-include $(ALL_OBJ:.o=.d) $(DEBUGGER_OBJ:.o=.d)