CC=gcc
CFLAGS=-g -I. -lpthread -lrt
DEPS = Horde.h Horde_types.h Horde_API.h
OBJ = main.o macro_expansions.o evaluation.o construction.o debug.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

horde: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) 
	rm -f *.o  
	
clean:
	rm -f *.o  
