#include <stdio.h>
#include <stdlib.h>
#include "lzrw3.h"

UWORD decompress(UBYTE* input, UWORD input_size, UBYTE *output)
{
	UBYTE* hashTable[HASH_TABLE_LENGTH];
	UBYTE* SCAN = input;
	UBYTE* DEST = output;

	UBYTE* SCAN_POST = SCAN + input_size;
	UBYTE* SCAN_MAX16 = SCAN + input_size - (MAX_CMP_GROUP - 2);
	UBYTE* DEST_POST = DEST + C_SIZE;

	UWORD control = 1;
	UWORD literals = 0;

	init_hashTable(hashTable);
	while(1)
	{
		UWORD unroll;

		if(control == 1)
		{
			control = 0x10000 | *(SCAN++);
			control |= *(SCAN++) << 8;
		}
		
		unroll = SCAN <= SCAN_MAX16 ? 16: 1;

		while(unroll--)
		{
			if(control & 1)
			{
				UBYTE* p_scan;
				UWORD lmt;
				UBYTE** p_hash;
				UBYTE* p_lookup = DEST;

				lmt = *(SCAN++);
				p_hash = &hashTable[((lmt & 0xF0) << 4) | *(SCAN++)];
				p_scan = *p_hash;
				lmt &= 0xF;


				*(DEST++) = *(p_scan++); if(DEST>DEST_POST) return C_SIZE;
				*(DEST++) = *(p_scan++); if(DEST>DEST_POST) return C_SIZE;
				*(DEST++) = *(p_scan++); if(DEST>DEST_POST) return C_SIZE;
				while(lmt--)
				{
					*(DEST++) = *(p_scan++); if(DEST>DEST_POST) return C_SIZE;
				}

				if(literals > 0)
				{
					UBYTE *r = p_lookup - literals;
					hashTable[HASH(r)] = r;
					if(literals == 2)
					{
						r++;
						hashTable[HASH(r)] = r;
					}
					literals = 0;
				}
				*p_hash = p_lookup;
			}
			else
			{
				*(DEST++) = *(SCAN++); if(DEST>DEST_POST) return C_SIZE;
				
				if(++literals == 3)
				{
					UBYTE* p_scan = DEST - 3;
					hashTable[HASH(p_scan)] = p_scan;
					literals = 2;
				}
			}
			control >>= 1;
		}
	}
	return (UWORD) (DEST - output);
}

UWORD compress(UBYTE* input, UWORD input_size, UBYTE* output)
{
	UBYTE* hashTable[HASH_TABLE_LENGTH];
	UWORD return_val = 0;
	//initialize hash Table
	init_hashTable(hashTable);
	/*	SCAN: scanning point
		DEST: the position to write incoded data */
	UBYTE* SCAN = input;
	UBYTE* DEST = output;
	//	last positions of stream 
	UBYTE* SCAN_POST = SCAN + input_size;
	UBYTE* DEST_POST = DEST + C_SIZE;
	UBYTE* SCAN_MAX1 = SCAN + input_size - MAX_RAW_ITEM;
	UBYTE* SCAN_MAX16 = SCAN + input_size - MAX_RAW_ITEM * 16;
	// control word
	UBYTE* p_control;
	UWORD control = TOPWORD;
	//literal buffers
	UBYTE** l_buf1 = 0;		//point the hash table entry to the youngest literal
	UBYTE** l_buf2 = 0;		//point the hash table entry to the 2nd youngest literal
	
	//write control bits of first group
	p_control = DEST;
	DEST += 2;
	
	while(1)
	{
		UBYTE *p_scan;
		UBYTE *p_lookup;
		UWORD unroll;
		UWORD index;
		UBYTE **p_hash;

		if(DEST > DEST_POST)
		{
			goto overrun;
		}

		unroll = 16;
		if(SCAN > SCAN_MAX16)
		{
			unroll = 1;
			if(SCAN > SCAN_MAX1)
			{
				if(SCAN == SCAN_POST)
					break;
				else
					goto letteral;
			}
		}
	
		begin_unrolled_loop:
		
		index = HASH(SCAN);
		p_hash = &hashTable[index];
		p_scan = *p_hash;


		#define PS *(p_scan++) != *(SCAN++)
		p_lookup = SCAN;
		if(PS || PS || PS)
		{
			//letteral
			SCAN = p_lookup;

			letteral:

			*(DEST++) = *(SCAN++);
			control &= 0xFFFEFFFF;

			if(l_buf2 != 0)
				*l_buf2 = p_lookup - 2;
			
			l_buf2 = l_buf1;
			l_buf1 = p_hash;

		}
		else
		{
			//copy
			PS || PS || PS || PS || PS || PS || PS || PS ||
			PS || PS || PS || PS || PS || PS || PS || SCAN++;
			*(DEST++) = ((index & 0xF00) >> 4) | (--SCAN - p_lookup - 3);
			*(DEST++) = index & 0xFF;

			if(l_buf1 != 0)
			{
				if(l_buf2 != 0)
				{
					*l_buf2 = p_lookup -2;
					l_buf2 = 0;
				}
				*l_buf1 = p_lookup -1;
				l_buf1 = 0;
			}

			*p_hash = p_lookup;
		}
		control >>= 1;

		end_unrolled_loop:
		
		if(--unroll)
			goto begin_unrolled_loop;

		if((control & TOPWORD) == 0)
		{
			*(p_control++) = control & 0xFF;
			*p_control = (control >> 8) &0xFF;

			p_control = DEST;
			DEST += 2;

			control = TOPWORD;
		}
	}
	while(control & TOPWORD)
		control >>= 1;
	*(p_control++) = control & 0xFF;
	*(p_control++) = (control >> 8) & 0xFF;

	if(p_control == DEST)
		DEST -= 2;
	return (UWORD) DEST - (UWORD) output;

	overrun:
		printf("ERROR:overruned\n");
		return -1;
}

void init_hashTable(UBYTE** hashTable)
{
	#define ZH *hashTable++ = START_STRING
	UWORD i;
	for(i = 0 ; i < 256 ; i++)
	{
		ZH;ZH;ZH;ZH;ZH;ZH;ZH;ZH;ZH;ZH;ZH;ZH;ZH;ZH;ZH;ZH;
	}
}

