CFLAGS=-std=gnu99 -Wall
all: hfdisk

hfdisk: hfdisk.o dump.o partition_map.o convert.o io.o errors.o bitfield.o

clean:
	rm -f *.o hfdisk

convert.o: convert.c partition_map.h convert.h
dump.o: dump.c io.h errors.h partition_map.h
errors.o: errors.c errors.h
io.o: io.c hfdisk.h io.h errors.h
partition_map.o: partition_map.c partition_map.h hfdisk.h convert.h io.h errors.h
hfdisk.o: hfdisk.c hfdisk.h io.h errors.h partition_map.h version.h

partition_map.h: dpme.h
dpme.h: bitfield.h

