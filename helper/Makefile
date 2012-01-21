TARGETS = helper
TESTS	= vector_test matrix_test stack_test list_test queue_test matrixgraph_test constvector_test

CC	= /usr/local/bin/mpicc
CCFLAGS	= -Wall --std=c99 -ggdb -fPIC -pthread -fms-extensions
# CCFLAGS	= -Wall --std=c99 -pthread -O2 -pipe -fomit-frame-pointer -fms-extensions
LDFLAGS	= -lm -lhelper
DIRS	=
LIBS	=

all: $(DIRS) $(TARGETS) $(TESTS)

helper: vector.o matrix.o stack.o queue.o list.o matrixgraph.o constvector.o algorithm.o
	ar -cvr libhelper.a $^

vector_test: vector_test.o helper
	$(CC) $(CCFLAGS) -L. $(DIRS:%=-L./%) $(DIRS:%=-I./%) -o $@ $< $(LDFLAGS)

constvector_test: constvector_test.o helper
	$(CC) $(CCFLAGS) -L. $(DIRS:%=-L./%) $(DIRS:%=-I./%) -o $@ $< $(LDFLAGS)

list_test: list_test.o helper
	$(CC) $(CCFLAGS) -L. $(DIRS:%=-L./%) $(DIRS:%=-I./%) -o $@ $< $(LDFLAGS)

matrix_test: matrix_test.o helper
	$(CC) $(CCFLAGS) -L. $(DIRS:%=-L./%) $(DIRS:%=-I./%) -o $@ $< $(LDFLAGS)

matrixgraph_test: matrixgraph_test.o helper
	$(CC) $(CCFLAGS) -L. $(DIRS:%=-L./%) $(DIRS:%=-I./%) -o $@ $< $(LDFLAGS)

stack_test: stack_test.o helper
	$(CC) $(CCFLAGS) -L. $(DIRS:%=-L./%) $(DIRS:%=-I./%) -o $@ $< $(LDFLAGS)

queue_test: queue_test.o helper
	$(CC) $(CCFLAGS) -L. $(DIRS:%=-L./%) $(DIRS:%=-I./%) -o $@ $< $(LDFLAGS)

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
