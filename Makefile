TARGETS = connectivity pixelpattern clustering

CC	= /usr/bin/gcc
MPICC = /usr/local/bin/mpicc
CCFLAGS	= -Wall --std=c99 -ggdb -pthread
MPICCFLAGS = --std=c99
# CCFLAGS	= -Wall -pthread -O2 -pipe -fomit-frame-pointer
LDFLAGS	= -lm -lhelper
DIRS	= helper
LIBS	=

all: $(TARGETS) $(DIRS)

connectivity: connectivity.o helper
	$(CC) $(CCFLAGS) $(DIRS:%=-L./%) $(DIRS:%=-I./%) -o $@ $< $(LDFLAGS)

pixelpattern: pixelpattern.o helper
	$(CC) $(CCFLAGS) $(DIRS:%=-L./%) $(DIRS:%=-I./%) -o $@ $< $(LDFLAGS)

clustering: clustering.o helper
	$(MPICC) $(MPICCFLAGS) $(DIRS:%=-L./%) $(DIRS:%=-I./%) -o $@

$(DIRS): force_look
	@cd $@; $(MAKE) all

%.o: %.c
	$(CC) $(CCFLAGS) $(DIRS:%=-L./%) $(DIRS:%=-I./%) -o $@ -c $<

clean:
	-rm -f *~ *.o $(TARGETS)
	@for d in $(DIRS); do (cd $$d; $(MAKE) clean ); done

distclean: clean

force_look:
	@true
