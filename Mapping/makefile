# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -g -lm 


# Executable name
TARGET = mapping_exe

# Source files
SRCS = ecrire_position.c

# Object files
OBJS = $(SRCS:.c=.o)

# Header files
HEADERS = 

# Default rule
all: $(TARGET)

# Linking the executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) -lpthread -lm -lwiringPi

# Compiling source files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $<

# Clean
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
