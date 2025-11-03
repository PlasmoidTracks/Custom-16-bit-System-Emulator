# Compiler and flags
CC = gcc
CFLAGS = -D_FILE_OFFSET_BITS=64 -Iinclude -Iinclude/utils -march=native -Wall -Werror=format-security -Werror=implicit-function-declaration -std=c11 -fpie -march=native \
         -Wextra -pedantic -fstack-protector-all -Wshadow -Wstrict-prototypes -Werror=return-type -pthread -finstrument-functions -D_GNU_SOURCE
LIBS = -Llib 

# Directories
SRC_DIR = src
OBJ_DIR = obj

# Source and object files
SRC_FILES = $(shell find $(SRC_DIR) -name '*.c')
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC_FILES))

# Main file (not inside src/)
MAIN_FILE = main.c
MAIN_OBJ = $(OBJ_DIR)/main.o

# Output executable
OUTPUT = main

# Default target (also generates compile_commands.json)
all: compile_commands.json $(OUTPUT)

# Debug mode
debug: CFLAGS += -g -ggdb -DDEBUG
debug: $(OUTPUT)

# Optimized build
fast: CFLAGS += -O3
fast: $(OUTPUT)

# Generate assembly
assembly: CFLAGS += -O0 -masm=intel
assembly: main.s

main.s: $(MAIN_FILE)
	$(CC) $(CFLAGS) -S $(MAIN_FILE) -o main.s

# Preprocess main.c with -E flag and output to main.i (not in obj/)
verbose: main.i

main.i: $(MAIN_FILE)
	$(CC) $(CFLAGS) -E $(MAIN_FILE) -o main.i

# Link executable
$(OUTPUT): $(OBJ_FILES) $(MAIN_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS) -lm

# Compile source files (place object files in obj/)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

# Create nested directories for object files (e.g. obj/utils/)
$(OBJ_DIR)/%.o: | $(OBJ_DIR)/%/
$(OBJ_DIR)/%/:
	@mkdir -p $(@D)

# Compile main.c separately and put it in obj/
$(OBJ_DIR)/main.o: $(MAIN_FILE) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

# Ensure obj/ directory exists
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Clean build artifacts
clean:
	rm -f $(OBJ_DIR)/*.o $(OUTPUT) main.s main.i compile_commands.json
	rm -rf $(OBJ_DIR)

# Generate compile_commands.json using Bear
compile_commands.json: $(SRC_FILES) $(MAIN_FILE)
	@echo "Generating compile_commands.json..."
	bear -- make debug $(MAKECMDGOALS)
