# MAKEFILE for IOS course, 2nd project
################################################################################
# Author:				David Kaspar (aka Dee'Kej)
#								FIT VUT Brno, 1BIT
# E-mail:				xkaspa34@stud.fit.vutbr.cz
# Date:					02-05-2012
# Last update:	02-05-2012
################################################################################


# Compiler.
CC = gcc

# Parameters of compilation for C.
CFLAGS = -std=gnu99 -pedantic -W -Wall -Wextra -Werror -lrt -O2

# Names of programs to be compiled/removed etc.
PROGS = readerWriter


################################################################################
# Dependencies:
################################################################################

readerWriter: readerWriter_main.o readerWriter_writer.o readerWriter_reader.o
	$(CC) readerWriter_main.o readerWriter_writer.o readerWriter_reader.o -lrt -o $@

readerWriter_main.o: readersWriters_main.c readersWriters.h
	$(CC) $(CFLAGS) -c readersWriters_main.c -o $@

readerWriter_writer.o: readersWriters_writer.c readersWriters.h
	$(CC) $(CFLAGS) -c readersWriters_writer.c -o $@

readerWriter_reader.o: readersWriters_reader.c readersWriters.h
	$(CC) $(CFLAGS) -c readersWriters_reader.c -o $@


################################################################################
# Other useful stuff:
################################################################################

# Rule to mark "false-positive" targets of commands below in project folder.
.PHONY: clean clean-all

# Remove object files generated during compiling.
clean:
	@rm -f *.o *.a *.so

# Remove all files generated during compiling.
clean-all:
	@rm -f *.o *.a *.so $(PROGS)

	
