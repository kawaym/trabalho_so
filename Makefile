CC ?= gcc
CFLAGS ?= -Wall -Wextra -pedantic -O2
THREAD_FLAGS ?= -pthread
LDFLAGS ?=
TARGET ?= chatgpt-edh
SRC ?= chatgpt-edh.c

OBJ := $(SRC:.c=.o)

CLAUDE = claude-edh

.PHONY: all run clean distclean

all: $(TARGET) $(CLAUDE)

$(TARGET): $(OBJ)
	$(CC) $(THREAD_FLAGS) $(CFLAGS) $(OBJ) $(LDFLAGS) -o $@

$(CLAUDE): claude-edh.c
	$(CC) $(THREAD_FLAGS) $(CFLAGS) claude-edh.c -o $(CLAUDE)

%.o: %.c
	$(CC) $(THREAD_FLAGS) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJ)

distclean: clean
	rm -f $(TARGET) $(CLAUDE) ${CLAUDE}.o
