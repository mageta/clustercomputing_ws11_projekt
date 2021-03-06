TARGETS = pixelpattern find_components clustering

CC	= /usr/local/bin/mpicc
# CCFLAGS	= -Wall --std=c99 -ggdb -pthread -fms-extensions
CCFLAGS	= -Wall --std=c99 -pthread -O2 -pipe -fomit-frame-pointer -fms-extensions -finline-functions -march=native
LDFLAGS	= -lm -lhelper
DIRS	= helper
LIBS	=

all: $(TARGETS) $(DIRS)

pixelpattern: pixelpattern.o components.o helper
	$(CC) $(CCFLAGS) $(DIRS:%=-L./%) $(DIRS:%=-I./%) -o $@ $< components.o $(LDFLAGS)

find_components: find_components.o components.o border_compare.o helper
	$(CC) $(CCFLAGS) $(DIRS:%=-L./%) $(DIRS:%=-I./%) -o $@ $< components.o border_compare.o $(LDFLAGS)

clustering: clustering.o components.o helper
	$(CC) $(CCFLAGS) $(DIRS:%=-L./%) $(DIRS:%=-I./%) -o $@ $< components.o $(LDFLAGS)

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
