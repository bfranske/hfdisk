CFLAGS=-std=gnu99
all: pdisk

pdisk: pdisk.o dump.o partition_map.o convert.o io.o errors.o bitfield.o

clean:
	rm -f *.o pdisk

convert.o: convert.c partition_map.h convert.h
dump.o: dump.c io.h errors.h partition_map.h
errors.o: errors.c errors.h
io.o: io.c pdisk.h io.h errors.h
partition_map.o: partition_map.c partition_map.h pdisk.h convert.h io.h errors.h
pdisk.o: pdisk.c pdisk.h io.h errors.h partition_map.h version.h

partition_map.h: dpme.h
dpme.h: bitfield.h

