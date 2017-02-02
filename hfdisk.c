//
// hfdisk - an editor for Apple format partition tables
//
// Written by Eryk Vershen (eryk@apple.com)
//
// Still under development (as of 20 Dec 1996)
//

/*
 * Copyright 1996,1997 by Apple Computer, Inc.
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * APPLE COMPUTER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL APPLE COMPUTER BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 */

#include <stdio.h>
#include <getopt.h>
#include <stddef.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <sys/ioctl.h>

#include "hfdisk.h"
#include "io.h"
#include "errors.h"
#include "partition_map.h"
#include "dump.h"
#include "version.h"


//
// Defines
//
#define ARGV_CHUNK 5


//
// Types
//


//
// Global Constants
//
enum getopt_values {
    kLongOption = 0,
    kBadOption = '?',
    kOptionArg = 1000,
    kListOption = 1001
};

const NAMES plist[] = {
	{"Drvr", "Apple_Driver"},
	{"Dr43", "Apple_Driver43"},
	{"Free", "Apple_Free"},
	{" HFS", "Apple_HFS"},
	{" MFS", "Apple_MFS"},
	{"PDOS", "Apple_PRODOS"},
	{"junk", "Apple_Scratch"},
	{"unix", "Apple_UNIX_SVR2"},
	{" map", "Apple_partition_map"},
	{0, 0}
};


//
// Global Variables
//
int lflag;
char *lfile;
int vflag;
int hflag;
int dflag;
int rflag;


//
// Forward declarations
//
void do_add_intel_partition(partition_map_header *map);
void do_change_map_size(partition_map_header *map);
void do_create_partition(partition_map_header *map, int get_type);
void do_create_bootstrap_partition(partition_map_header *map);
void do_delete_partition(partition_map_header *map);
int do_expert(partition_map_header *map);
void do_reorder(partition_map_header *map);
void do_write_partition_map(partition_map_header *map);
void edit(char *name);
int get_base_argument(long *number, partition_map_header *map);
int get_size_argument(uint32_t base, long *number, partition_map_header *map);
int get_options(int argc, char **argv);
void print_notes();


//
// Routines
//
int
main(int argc, char **argv)
{
    int name_index;
    int err=0;

    if (sizeof(DPME) != PBLOCK_SIZE) {
	fatal(-1, "Size of partion map entry (%d) "
		"is not equal to block size (%d)\n",
		sizeof(DPME), PBLOCK_SIZE);
    }
    if (sizeof(Block0) != PBLOCK_SIZE) {
	fatal(-1, "Size of block zero structure (%d) "
		"is not equal to block size (%d)\n",
		sizeof(Block0), PBLOCK_SIZE);
    }

    name_index = get_options(argc, argv);

    if (vflag) {
	printf("version " VERSION " (" RELEASE_DATE ")\n");
    }
    if (hflag) {
 	do_help();
    } else if (lflag) {
	if (lfile != NULL) {
	    dump(lfile);
	} else if (name_index < argc) {
	    while (name_index < argc) {
		dump(argv[name_index++]);
	    }
	} else {
	    list_all_disks();
	}
    } else if (name_index < argc) {
	while (name_index < argc) {
	    edit(argv[name_index++]);
	}
    } else if (!vflag) {
	usage("no device argument");
 	do_help();
	err=-EINVAL;	// debatable
    }
    exit(err);
}


int
get_options(int argc, char **argv)
{
    int c;
    static struct option long_options[] =
    {
	// name		has_arg			&flag	val
	{"help",	no_argument,		0,	'h'},
	{"list",	optional_argument,	0,	kListOption},
	{"version",	no_argument,		0,	'v'},
	{"debug",	no_argument,		0,	'd'},
	{"readonly",	no_argument,		0,	'r'},
	{0, 0, 0, 0}
    };
    int option_index = 0;
    extern int optind;
    extern char *optarg;
    int flag = 0;

    init_program_name(argv);

    lflag = 0;
    lfile = NULL;
    vflag = 0;
    hflag = 0;
    dflag = 0;
    rflag = 0;

    optind = 0;	// reset option scanner logic
    while ((c = getopt_long(argc, argv, "hlvdr", long_options,
	    &option_index)) >= 0) {
	switch (c) {
	case kLongOption:
	    // option_index would be used here
	    break;
	case 'h':
	    hflag = 1;
	    break;
	case kListOption:
	    if (optarg != NULL) {
		lfile = optarg;
	    }
	    // fall through
	case 'l':
	    lflag = 1;
	    break;
	case 'v':
	    vflag = 1;
	    break;
	case 'd':
	    dflag = 1;
	    break;
	case 'r':
	    rflag = 1;
	    break;
	case kBadOption:
	default:
	    flag = 1;
	    break;
	}
    }
    if (flag) {
	usage("bad arguments");
    }
    return optind;
}

//
// Edit the file
//
void
edit(char *name)
{
    partition_map_header *map;
    int command;
    int first = 1;
    int order;
    int get_type;
    int valid_file;

    map = open_partition_map(name, &valid_file);
    if (!valid_file) {
    	return;
    }

    printf("%s\n", name);

    while (get_command("Command (? for help): ", first, &command)) {
	first = 0;
	order = 1;
	get_type = 0;

	switch (command) {
	case '?':
	    print_notes();
	case 'H':
	case 'h':
	    printf("Commands are:\n");
	    printf("  h    help\n");
	    printf("  p    print the partition table\n");
	    printf("  P    (print ordered by base address)\n");
	    printf("  i    initialize partition map\n");
	    printf("  s    change size of partition map\n");
	    printf("  b    create new 800K bootstrap partition\n");
	    printf("  c    create new Linux partition\n");
	    printf("  C    (create with type also specified)\n");
	    printf("  d    delete a partition\n");
	    printf("  r    reorder partition entry in map\n");
	    if (!rflag) {
		printf("  w    write the partition table\n");
	    }
	    printf("  q    quit editing (don't save changes)\n");
	    if (dflag) {
		printf("  x    extra extensions for experts\n");
	    }
	    break;
	case 'P':
	    order = 0;
	    // fall through
	case 'p':
	    dump_partition_map(map, order);
	    break;
	case 'Q':
	case 'q':
	    goto finis;
	    break;
	case 'I':
	case 'i':
	    map = init_partition_map(name, map);
	    break;
	case 'B':
	case 'b':
	    do_create_bootstrap_partition(map);
	    break;
	case 'C':
	    get_type = 1;
	    // fall through
	case 'c':
	    do_create_partition(map, get_type);
	    break;
	case 'D':
	case 'd':
	    do_delete_partition(map);
	    break;
	case 'R':
	case 'r':
	    do_reorder(map);
	    break;
	case 'S':
	case 's':
	    do_change_map_size(map);
	    break;
	case 'X':
	case 'x':
	    if (!dflag) {
		goto do_error;
	    } else if (do_expert(map)) {
		goto finis;
	    }
	    break;
	case 'W':
	case 'w':
	    if (!rflag) {
		do_write_partition_map(map);
		break;
	    }
	default:
	do_error:
	    bad_input("No such command (%c)", command);
	    break;
	}
    }
finis:

    close_partition_map(map);
}


void
do_create_partition(partition_map_header *map, int get_type)
{
    long base;
    long length;
    char *name = NULL;
    char *type_name = NULL;

    if (map == NULL) {
	bad_input("No partition map exists");
	return;
    }
    if (!rflag && map->writeable == 0) {
	printf("The map is not writeable.\n");
    }
// XXX add help feature (i.e. '?' in any argument routine prints help string)
    if (get_base_argument(&base, map) == 0) {
	return;
    }
    if (get_size_argument(base, &length, map) == 0) {
	return;
    }

    if (get_string_argument("Name of partition: ", &name, 1) == 0) {
	bad_input("Bad name");
	return;
    }
    if (get_type == 0) {
	add_partition_to_map(name, kUnixType, base, length, map);

    } else {
	while (1) {
	    if (get_string_argument("Type of partition (L for known types): ", &type_name, 1) == 0) {
	      bad_input("Bad type");
	      return;
            } else if (strncmp(type_name, kFreeType, DPISTRLEN) == 0) {
		    bad_input("Can't create a partition with the Free type");
	    } else if (strncmp(type_name, kMapType, DPISTRLEN) == 0) {
		    bad_input("Can't create a partition with the Map type");
	    } else if (strncmp(type_name, "L", DPISTRLEN) == 0) {
		int i = 0;
		while (plist[i].full) {
		    printf("%s\n", plist[i].full);
		    ++i;
		}
		printf("\n");
	    }
		else
		{
			break;
		}
	}
	add_partition_to_map(name, type_name, base, length, map);
    }
}


void
do_create_bootstrap_partition(partition_map_header *map)
{
    long base;

    if (map == NULL) {
	bad_input("No partition map exists");
	return;
    }
 
    if (!rflag && map->writeable == 0) {
	printf("The map is not writeable.\n");
    }

    // XXX add help feature (i.e. '?' in any argument routine prints help string)
    if (get_base_argument(&base, map) == 0) {
	return;
    }

    // create 800K type Apple_Bootstrap partition named `bootstrap'
    add_partition_to_map(kBootstrapName, kBootstrapType, base, 1600, map);
}


int
get_base_argument(long *number, partition_map_header *map)
{
    int result = 0;

    uint32_t defaultFirstBlock = find_free_space(map);
    char prompt[32];
    sprintf(prompt, "First block [%"PRIu32"]: ", defaultFirstBlock);
    if (get_number_argument(prompt, number, defaultFirstBlock) == 0) {
	bad_input("Bad block number");
    } else {
	result = 1;
    }
    return result;
}


int
get_size_argument(uint32_t base, long *number, partition_map_header *map)
{
    int result = 0;

    uint32_t defaultSize = 20480; // 10MB

    // Work out how much free-space is available.
    partition_map* part = find_entry_by_sector(base, map);
    if (part)
    {
	size_t partEnd =
	    part->data->dpme_pblock_start +
	    part->data->dpme_pblocks;
	defaultSize = partEnd - base;
    }

    char prompt[80];
    sprintf(
	prompt,
	"Length (in blocks, kB (k), MB (M) or GB (G)) [%"PRIu32"]: ",
	defaultSize);

    if (get_number_argument(prompt, number, defaultSize) == 0) {
	bad_input("Bad length");
    } else {
	result = 1;
    }
    return result;
}


void
do_delete_partition(partition_map_header *map)
{
    partition_map * cur;
    long index;

    if (map == NULL) {
	bad_input("No partition map exists");
	return;
    }
    if (!rflag && map->writeable == 0) {
	printf("The map is not writeable.\n");
    }
    if (get_number_argument("Partition number: ", &index, kDefault) == 0) {
	bad_input("Bad partition number");
	return;
    }

	// find partition and delete it
    cur = find_entry_by_disk_address(index, map);
    if (cur == NULL) {
	printf("No such partition\n");
    } else {
	delete_partition_from_map(cur);
    }
}


void
do_reorder(partition_map_header *map)
{
    long old_index;
    long index;

    if (map == NULL) {
	bad_input("No partition map exists");
	return;
    }
    if (!rflag && map->writeable == 0) {
	printf("The map is not writeable.\n");
    }
    if (get_number_argument("Partition number: ", &old_index, kDefault) == 0) {
	bad_input("Bad partition number");
	return;
    }
    if (get_number_argument("New number: ", &index, kDefault) == 0) {
	bad_input("Bad partition number");
	return;
    }

    move_entry_in_map(old_index, index, map);
}


void
do_write_partition_map(partition_map_header *map)
{
    if (map == NULL) {
	bad_input("No partition map exists");
	return;
    }
    if (map->changed == 0) {
	bad_input("The map has not been changed.");
	return;
    }
    if (map->writeable == 0) {
	bad_input("The map is not writeable.");
	return;
    }
//    printf("Writing the map destroys what was there before. ");
    printf("IMPORTANT: You are about to write a changed partition map to disk. \n");
    printf("For any partition you changed the start or size of, writing out \n");
    printf("the map causes all data on that partition to be LOST FOREVER. \n");
    printf("Make sure you have a backup of any data on such partitions you \n");
    printf("want to keep before answering 'yes' to the question below! \n\n");
    if (get_okay("Write partition map? [N/y]: ") != 1) {
	return;
    }

    write_partition_map(map);

    printf("\nPartition map written to disk. If any partitions on this disk \n");
    printf("were still in use by the system (see messages above), you will need \n");
    printf("to reboot in order to utilize the new partition map.\n\n");

    // exit(0);
}

int
do_expert(partition_map_header *map)
{
    int command;
    int first = 0;
    int quit = 0;

    while (get_command("Expert command (? for help): ", first, &command)) {
	first = 0;

	switch (command) {
	case '?':
	    print_notes();
	case 'H':
	case 'h':
	    printf("Commands are:\n");
	    printf("  h    print help\n");
	    printf("  x    return to main menu\n");
	    printf("  p    print the partition table\n");
	    if (dflag) {
		printf("  P    (show data structures  - debugging)\n");
	    }
	    printf("  s    change size of partition map\n");
	    if (!rflag) {
		printf("  w    write the partition table\n");
	    }
	    printf("  q    quit without saving changes\n");
	    break;
	case 'X':
	case 'x':
	    goto finis;
	    break;
	case 'Q':
	case 'q':
	    quit = 1;
	    goto finis;
	    break;
	case 'S':
	case 's':
	    do_change_map_size(map);
	    break;
	case 'P':
	    if (dflag) {
		show_data_structures(map);
		break;
	    }
	    // fall through
	case 'p':
	    dump_partition_map(map, 1);
	    break;
	case 'W':
	case 'w':
	    if (!rflag) {
		do_write_partition_map(map);
		break;
	    }
	default:
	    bad_input("No such command (%c)", command);
	    break;
	}
    }
finis:
    return quit;
}

void
do_change_map_size(partition_map_header *map)
{
    long size;

    if (map == NULL) {
	bad_input("No partition map exists");
	return;
    }
    if (!rflag && map->writeable == 0) {
	printf("The map is not writeable.\n");
    }
    if (get_number_argument("New size: ", &size, kDefault) == 0) {
	bad_input("Bad size");
	return;
    }
    resize_map(size, map);
}


void
print_notes()
{
    printf("Notes:\n");
    printf("  Base and length fields are blocks, which are 512 bytes long.\n");
    printf("  The name of a partition is descriptive text.\n");
    printf("\n");
}
