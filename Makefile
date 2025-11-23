# Simple Makefile for the Dining Hall synchronization exercise

# Toolchain configuration (override on command line as needed)
CC ?= gcc
CFLAGS ?= -Wall -Wextra -pedantic -O2
THREAD_FLAGS ?= -pthread
LDFLAGS ?=
TARGET ?= main
SRC ?= main.c

# Derived variables
OBJ := $(SRC:.c=.o)

.PHONY: all run clean distclean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(THREAD_FLAGS) $(CFLAGS) $(OBJ) $(LDFLAGS) -o $@

%.o: %.c
	$(CC) $(THREAD_FLAGS) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJ)

distclean: clean
	rm -f $(TARGET)
