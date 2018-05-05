#define UBYTE unsigned char
#define UWORD unsigned int

#define FLAG_COMPRESS 0
#define FLAG_COPY 1

#define MAX_RAW_ITEM 18
#define MAX_RAW_GROUP 16 * MAX_RAW_ITEM
#define MAX_CMP_GROUP (2 + 16*2)

#define HASH_TABLE_LENGTH 4096
#define START_STRING (UBYTE*) "123456789012345678"

#define HASH(PTR) \
	(((40543*(((*(PTR))<<8)^((*((PTR)+1))<<4)^(*((PTR)+2))))>>4) & 0xFFF)


//chunk size
#define C_SIZE 8192

#define TOPWORD 0xFFFF0000


UWORD decompress(UBYTE*, UWORD, UBYTE*);
UWORD compress(UBYTE*, UWORD, UBYTE*);
void init_hashTable(UBYTE**);
