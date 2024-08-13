
CC=clang
CFLAGS=-Wall -g -O3 -Iinclude
LDFLAGS=-lGLEW -lglfw -lGL -lm -lpthread -lrt -ldl -lX11

SRC_DIR = src
INCLUDE_DIR = include
OBJ_DIR = obj
BIN_DIR = bin

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))
TARGET = $(BIN_DIR)/main

$(info Source files: $(SRCS))
$(info Source files: $(OBJS))

# default target
all: $(TARGET)

# link object files to create the executable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	
# compile source files to object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@
	
# clean target
clean:
	rm -f $(OBJS) 
	
# phony targets
.PHONY: all clean
