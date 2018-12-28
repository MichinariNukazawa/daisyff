#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <ctype.h>
#include <assert.h>

#define ERROR_LOG(fmt, ...) \
	fprintf(stderr, "error: %s()[%d]: "fmt"\n", __func__, __LINE__, ## __VA_ARGS__)
#define WARN_LOG(fmt, ...) \
	fprintf(stderr, "warning: %s()[%d]: "fmt"\n", __func__, __LINE__, ## __VA_ARGS__)
#define DEBUG_LOG(fmt, ...) \
	fprintf(stdout, "debug: %s()[%d]: "fmt"\n", __func__, __LINE__, ## __VA_ARGS__)
#define ASSERT(hr) \
	do{ \
		if(!(hr)){ \
			fprintf(stderr, "pv assert: %s()[%d]:'%s'\n", __func__, __LINE__, #hr); \
			assert(hr); \
		} \
	}while(0);

#define ASSERTF(hr, fmt, ...) \
	do{ \
		if(!(hr)){ \
			fprintf(stderr, "pv_assertf: %s()[%d]: "fmt"\n", \
					__func__, __LINE__, ## __VA_ARGS__); \
			assert(hr); \
		} \
	}while(0);

typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint32_t tag;

/**
  */
bool tag_valid(tag tag_)
{
	for(int i = 0; i < 4; i++){
		uint8_t v = 0xff & ((uint32_t)tag_ >> ((3 - i) * 4));
		if(0 == isprint(v)){
			DEBUG_LOG("0x%02x(%d) is not valid.", v, i);
			return false;
		}
	}

	return true;
}

bool tag_init(tag *tag_, char *s)
{
	ASSERT(tag_);

	if(4 != strlen(s)){
		WARN_LOG("%zu", strlen(s));
		return false;
	}

	memcpy((void *)tag_, s, 4);

	return tag_valid(*tag_);
}

typedef struct{
	uint32		sfntVersion;
	uint16		numTables;
	uint16		searchRange;
	uint16		entrySelector;
	uint16		rangeShift;
}OffsetTable;

bool OffsetTable_init(OffsetTable *offsetTable_, uint32 sfntVersion, int numTables_)
{
	ASSERT(offsetTable_);

	/*
	if(! (2 <= numTables_)){
		WARN_LOG("%d", numTables_);
		return false;
	}
	*/

	OffsetTable offsetTable = {
		.sfntVersion		= sfntVersion,
		.numTables		= numTables_,
	};
	offsetTable.searchRange		= numTables_ * 16;
	offsetTable.entrySelector	= (uint16)log2(numTables_);
	offsetTable.rangeShift		= (numTables_ * 16) - offsetTable.searchRange;

	*offsetTable_ = offsetTable;

	return true;
}

int main(int argc, char **argv)
{
	if(argc < 2){
		return 1;
	}

	int fd = open(argv[1], O_CREAT|O_TRUNC|O_RDWR, 0777);
	if(-1 == fd){
		fprintf(stderr, "open: %d %s\n", errno, strerror(errno));
		return 1;
	}

	uint32 sfntVersion;
	ASSERT(tag_init((tag *)&sfntVersion, "OTTO"));
	int numTables_ = 0;
	OffsetTable offsetTable;
	ASSERT(OffsetTable_init(&offsetTable, sfntVersion, numTables_));

	uint8_t *data = (void *)(&offsetTable);

	ssize_t s = write(fd, data, sizeof(OffsetTable));
	if(0 == s){
		fprintf(stderr, "write: %d %s\n", errno, strerror(errno));
	}
	close(fd);
	return 0;
}

