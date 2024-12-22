# Makefile for arranging windows code

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -O2

# Output binary name
TARGET = arrange_windows

# Source file
SRC = main.c

# Platform-specific settings
ifeq ($(OS),Windows_NT)
    # Windows settings
    LDFLAGS = -lgdi32
    TARGET := $(TARGET).exe
else
    # Linux settings
    LDFLAGS = -lX11
endif

# Build target
all: $(TARGET) run

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS) -lm

# Run the binary after building
run: $(TARGET)
	@./$(TARGET)

# Clean build artifacts
clean:
	rm -f $(TARGET)

.PHONY: all clean run
