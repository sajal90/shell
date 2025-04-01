# Makefile for mysh

CC      = gcc
CFLAGS  = -Wall -Wextra -Wpedantic -std=c11 -Iinclude
LDFLAGS =

# Debug / Release
DEBUG  ?= 0
ifeq ($(DEBUG),1)
  CFLAGS += -g3 -DDEBUG
else
  CFLAGS += -O2 -DNDEBUG
endif

SRC_DIR = src
OBJ_DIR = build

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

TARGET = mysh

.PHONY: all clean install uninstall

all: $(OBJ_DIR) $(TARGET)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

install: $(TARGET)
	install -m 0755 $(TARGET) /usr/local/bin/$(TARGET)

uninstall:
	rm -f /usr/local/bin/$(TARGET)
