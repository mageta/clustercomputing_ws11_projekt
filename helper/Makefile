TARGETS = helper vector_test matrix_test stack_test

CC	= /usr/bin/gcc
CCFLAGS	= -Wall --std=c99 -ggdb -fPIC -pthread
# CCFLAGS	= -Wall -pthread -O2 -pipe -fomit-frame-pointer
LDFLAGS	= -lm
DIRS	=
LIBS	=

all: $(DIRS) $(TARGETS)

helper: vector.o matrix.o stack.o queue.o
	ar -cvr libhelper.a $^

tests: helper vector_test matrix_test stack_test

vector_test: vector_test.o helper
	$(CC) $(CCFLAGS) $(DIRS:%=-L./%) $(DIRS:%=-I./%) -o $@ $< vector.o $(LDFLAGS)

matrix_test: matrix_test.o helper
	$(CC) $(CCFLAGS) $(DIRS:%=-L./%) $(DIRS:%=-I./%) -o $@ $< matrix.o $(LDFLAGS)

stack_test: stack_test.o helper
	$(CC) $(CCFLAGS) $(DIRS:%=-L./%) $(DIRS:%=-I./%) -o $@ $< stack.o vector.o $(LDFLAGS)

$(DIRS): force_look
	@cd $@; $(MAKE) all

%.o: %.c
	$(CC) $(CCFLAGS) -c $<

clean:
	-rm -f *~ *.o *.a $(TARGETS)
	@for d in $(DIRS); do (cd $$d; $(MAKE) clean ); done

distclean: clean

force_look:
	@true
