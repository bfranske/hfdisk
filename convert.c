//
// convert.c - Little-endian conversion
//
// Written by Eryk Vershen (eryk@apple.com)
//
// See comments in convert.h
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

#include "partition_map.h"
#include "convert.h"

#include <arpa/inet.h>

int
convert_dpme(DPME *data, int to_cpu_form)
{
	// Since we will toss the block if the signature doesn't match
	// we don't need to check the signature down here.
	data->dpme_signature = htons(data->dpme_signature);
	data->dpme_reserved_1 = htons(data->dpme_reserved_1);
	data->dpme_map_entries = htonl(data->dpme_map_entries);
	data->dpme_pblock_start = htonl(data->dpme_pblock_start);
	data->dpme_pblocks = htonl(data->dpme_pblocks);
	data->dpme_lblock_start = htonl(data->dpme_lblock_start);
	data->dpme_lblocks = htonl(data->dpme_lblocks);
	data->dpme_flags = htonl(data->dpme_flags);
	data->dpme_boot_block = htonl(data->dpme_boot_block);
	data->dpme_boot_bytes = htonl(data->dpme_boot_bytes);
	data->dpme_load_addr = htonl(data->dpme_load_addr);
	data->dpme_load_addr_2 = htonl(data->dpme_load_addr_2);
	data->dpme_goto_addr = htonl(data->dpme_goto_addr);
	data->dpme_goto_addr_2 = htonl(data->dpme_goto_addr_2);
	data->dpme_checksum = htonl(data->dpme_checksum);
	convert_bzb((BZB *)data->dpme_bzb, to_cpu_form);
	return 0;
}

int
convert_bzb(BZB *data, int to_cpu_form)
{
	// Since the data here varies according to the type of partition we
	// do not want to convert willy-nilly. We use the flag to determine
	// whether to check for the signature before or after we flip the bytes.
	if (to_cpu_form)
	{
		data->bzb_magic = htonl(data->bzb_magic);
		if (data->bzb_magic != BZBMAGIC)
		{
			data->bzb_magic = htonl(data->bzb_magic);
			if (data->bzb_magic != BZBMAGIC)
			{
				return 0;
			}
		}
	}
	else
	{
		if (data->bzb_magic != BZBMAGIC)
		{
			return 0;
		}
		data->bzb_magic = htonl(data->bzb_magic);
	}
	data->bzb_inode = htons(data->bzb_inode);
	data->bzb_flags = htonl(data->bzb_flags);
	data->bzb_tmade = htonl(data->bzb_tmade);
	data->bzb_tmount = htonl(data->bzb_tmount);
	data->bzb_tumount = htonl(data->bzb_tumount);
	return 0;
}


int
convert_block0(Block0 *data, int to_cpu_form)
{
	// Since this data is optional we do not want to convert willy-nilly.
	// We use the flag to determine whether to check for the signature
	// before or after we flip the bytes and to determine which form of
	// the count to use.
	if (to_cpu_form)
	{
		data->sbSig = htons(data->sbSig);
		if (data->sbSig != BLOCK0_SIGNATURE)
		{
			data->sbSig = htons(data->sbSig);
			if (data->sbSig != BLOCK0_SIGNATURE)
			{
				return 0;
			}
		}
	}
	else
	{
		if (data->sbSig != BLOCK0_SIGNATURE)
		{
			return 0;
		}
		data->sbSig = htons(data->sbSig);
	}
	data->sbBlkSize = htons(data->sbBlkSize);
	data->sbBlkCount = htonl(data->sbBlkCount);
	data->sbDevType = htons(data->sbDevType);
	data->sbDevId = htons(data->sbDevId);
	data->sbData = htonl(data->sbData);

	uint16_t count;
	if (to_cpu_form)
	{
		data->sbDrvrCount = htons(data->sbDrvrCount);
		count = data->sbDrvrCount;
	}
	else
	{
		count = data->sbDrvrCount;
		data->sbDrvrCount = htons(data->sbDrvrCount);
	}

	if (count > 0)
	{
		DDMap* m = (DDMap *) data->sbMap;
		for (int i = 0; i < count; i++)
		{
			m[i].ddBlock = htonl(m[i].ddBlock);
			m[i].ddSize = htons(m[i].ddSize);
			m[i].ddType = htons(m[i].ddType);
		}
	}
	return 0;
}

