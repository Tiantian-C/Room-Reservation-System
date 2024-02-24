# Define the compiler
CC = gcc

# Define any compile-time flags
CFLAGS = -Wall

# Define the source files
SOURCES = serverM.c serverS.c serverD.c serverU.c client.c

# Define the target executables
TARGETS = serverM serverS serverD serverU client

# Default target to build everything
all: $(TARGETS)

# A pattern rule to build each target from its source file
$(TARGETS):
	$(CC) $(CFLAGS) -o $@ $@.c

# Phony target for cleaning up generated files
.PHONY: clean
clean:
	rm -f $(TARGETS)

	
