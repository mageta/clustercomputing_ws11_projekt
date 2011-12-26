TARGETS = helper
TESTS	= vector_test matrix_test stack_test list_test queue_test

CC	= /usr/bin/gcc
CCFLAGS	= -Wall --std=c99 -ggdb -fPIC -pthread
# CCFLAGS	= -Wall -pthread -O2 -pipe -fomit-frame-pointer
LDFLAGS	= -lm
DIRS	=
LIBS	=

all: $(DIRS) $(TARGETS) $(TESTS)

helper: vector.o matrix.o stack.o queue.o list.o
	ar -cvr libhelper.a $^

vector_test: vector_test.o helper
	$(CC) $(CCFLAGS) $(DIRS:%=-L./%) $(DIRS:%=-I./%) -o $@ $< vector.o $(LDFLAGS)

list_test: list_test.o helper
	$(CC) $(CCFLAGS) $(DIRS:%=-L./%) $(DIRS:%=-I./%) -o $@ $< list.o $(LDFLAGS)

matrix_test: matrix_test.o helper
	$(CC) $(CCFLAGS) $(DIRS:%=-L./%) $(DIRS:%=-I./%) -o $@ $< matrix.o $(LDFLAGS)

stack_test: stack_test.o helper
	$(CC) $(CCFLAGS) $(DIRS:%=-L./%) $(DIRS:%=-I./%) -o $@ $< stack.o vector.o $(LDFLAGS)

queue_test: queue_test.o helper
	$(CC) $(CCFLAGS) $(DIRS:%=-L./%) $(DIRS:%=-I./%) -o $@ $< queue.o list.o $(LDFLAGS)

$(DIRS): force_look
	@cd $@; $(MAKE) all

%.o: %.c
	$(CC) $(CCFLAGS) -c $<

clean:
	-rm -f *~ *.o *.a $(TARGETS) $(TESTS)
	@for d in $(DIRS); do (cd $$d; $(MAKE) clean ); done

distclean: clean

force_look:
	@true
