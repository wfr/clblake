ifneq (,)
This makefile requires GNU Make.
endif

PROGRAM = blaketree
C_FILES := $(wildcard main.c blake256-*.c BlakeTree*.c opencl-util.c log.c)
OBJS := $(patsubst %.c, %.o, $(C_FILES))
CC = cc
#CFLAGS = -std=c99 -Werror -fopenmp $(shell pkg-config --cflags glib-2.0) -g -march=core2
CFLAGS = -std=c99 -Werror -fopenmp $(shell pkg-config --cflags glib-2.0) -O2 -march=core2 
LDFLAGS = -lOpenCL $(shell pkg-config --libs glib-2.0)

all: $(PROGRAM)

$(PROGRAM): .depend $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $(PROGRAM)

depend: .depend

.depend: cmd = gcc -MM -MF depend $(var); cat depend >> .depend;
.depend:
	@echo "Generating dependencies..."
	@$(foreach var, $(C_FILES), $(cmd))
	@rm -f depend

-include .depend

# These are the pattern matching rules. In addition to the automatic
# variables used here, the variable $* that matches whatever % stands for
# can be useful in special cases.
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f .depend *.o $(PROGRAM)

.PHONY: clean depend


