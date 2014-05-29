//
// io.c - simple io and input parsing routines
//
// Written by Eryk Vershen (eryk@apple.com)
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
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include "hfdisk.h"
#include "io.h"
#include "errors.h"


//
// Defines
//
#define BAD_DIGIT 17	/* must be greater than any base */
#define	STRING_CHUNK	16
#define UNGET_MAX_COUNT 10

//
// Types
//


//
// Global Constants
//
const long kDefault = -1;


//
// Global Variables
//


//
// Forward declarations
//


//
// Routines
//


int
get_okay(char *prompt)
{
    int result = 0;
    char* string = NULL;
    if (get_string_argument(prompt, &string, 1))
    {
	if (string[0] == 'Y' || string[0] == 'y')
	{
	    result = 1;
	}
    }
    free(string);
    return result;
}

	
int
get_command(char *prompt, int promptBeforeGet, int *command)
{
    int result = 0;
    char* string = NULL;
    if (get_string_argument(prompt, &string, 1))
    {
	*command = string[0];
	result = 1;
    }
    free(string);
    return result;

}

	
int
get_number_argument(char *prompt, long *number, long default_value)
{
    int result = 0;

    char* buf = NULL;
    size_t buflen = 0;
    char multiplier;
    int matched;

    while (result == 0) {
	printf("%s", prompt);

	if (getline(&buf, &buflen, stdin) == -1)
	{
	    // EOF
	    break;
	}
	else if ((default_value > 0) && (strncmp(buf, "\n", 1) == 0))
	{
	    *number = default_value;
	    result = 1;
	    break;
	}
	else if ((matched = sscanf(buf, "%ld%c", number, &multiplier)) >= 1)
	{
	    result = 1;
	    if (matched == 2) {
		if (multiplier == 'g' || multiplier == 'G') {
		    *number *= (1024*1024*1024 / PBLOCK_SIZE);
		} else if (multiplier == 'm' || multiplier == 'M') {
		    *number *= (1024*1024 / PBLOCK_SIZE);
		} else if (multiplier == 'k' || multiplier == 'K') {
		    *number *= (1024 / PBLOCK_SIZE);
		} else if (multiplier != '\n') {
		    result = 0;
		}
	    }
	}
    }
    free(buf);
    return result;
}


int
get_string_argument(char *prompt, char **string, int reprompt)
{
    int result = 0;
    size_t buflen = 0;

    while (result == 0) {
	printf("%s", prompt);

	if (getline(string, &buflen, stdin) == -1)
	{
	    // EOF
	    break;
	}
	else if ((strncmp(*string, "\n", 1) == 0) && !reprompt)
	{
	    result = 0;
	    break;
	}
	else
	{
	    size_t len = strnlen(*string, buflen);
	    if ((*string)[len - 1] == '\n') {
		(*string)[len - 1] = '\0';
	    }
	    result = 1;
	}
    }
    return result;
}

int
number_of_digits(unsigned long value)
{
    int j;

    j = 1;
    while (value > 9) {
	j++;
	value = value / 10;
    }
    return j;
}


//
// Print a message on standard error & flush the input.
//
void
bad_input(char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
}


int
read_block(int fd, unsigned long num, char *buf, int quiet)
{
    off_t x;
    long t;

    {
	x = ((long long) num * PBLOCK_SIZE); /* cast to ll to work around compiler bug */
	if ((x = lseek(fd, x, 0)) < 0) {
	    if (quiet == 0) {
		error(errno, "Can't seek on file");
	    }
	    return 0;
	}
	if ((t = read(fd, buf, PBLOCK_SIZE)) != PBLOCK_SIZE) {
	    if (quiet == 0) {
		error((t<0?errno:0), "Can't read block %u from file", num);
	    }
	    return 0;
	}
	return 1;
    }
}


int
write_block(int fd, unsigned long num, char *buf)
{
    off_t x;
    long t;

    if (rflag) {
	printf("Can't write block %lu to file", num);
	return 0;
    }
    {
	x = num * PBLOCK_SIZE;
	if ((x = lseek(fd, x, 0)) < 0) {
	    error(errno, "Can't seek on file");
	    return 0;
	}
	if ((t = write(fd, buf, PBLOCK_SIZE)) != PBLOCK_SIZE) {
	    error((t<0?errno:0), "Can't write block %u to file", num);
	    return 0;
	}
	return 1;
    }
}


int
close_device(int fildes)
{
	return close(fildes);
}


int
open_device(const char *path, int oflag)
{
	return open(path, oflag);
}
