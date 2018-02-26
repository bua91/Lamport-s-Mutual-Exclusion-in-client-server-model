IDIR =../include
CC=gcc
CFLAGS=-I$(IDIR)
ODIR=obj

_DEPS = clientsh.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = clientsh.o client.o 
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))
$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

clientsh: $(OBJ)
	gcc -g -pthread -o $@ $^ $(CFLAGS) -lcrypto -ggdb

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~
