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
#include <stdlib.h>
#include <arpa/inet.h>

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
			exit(1); \
		} \
	}while(0);

#define ASSERTF(hr, fmt, ...) \
	do{ \
		if(!(hr)){ \
			fprintf(stderr, "pv_assertf: %s()[%d]:'%s' "fmt"\n", \
					__func__, __LINE__, #hr, ## __VA_ARGS__); \
			exit(1); \
		} \
	}while(0);

typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint32_t tag;
typedef uint32_t Fixed;

#pragma pack (1)

/**
  */
bool tag_valid(const tag tag_)
{
	for(int i = 0; i < 4; i++){
		uint8_t v = 0xff & ((uint32_t)tag_ >> ((3 - i) * 8));
		if(0 == isprint(v)){
			DEBUG_LOG("0x%08x, 0x%02x(%d) is not printable.", tag_, v, i);
			return false;
		}
	}

	return true;
}

bool tag_init(tag *tag_, const char *tagstring)
{
	ASSERT(tag_);
	ASSERT(tagstring);

	if(4 != strlen(tagstring)){
		ERROR_LOG("%zu `%s`", strlen(tagstring), tagstring);
		return false;
	}

	memcpy((void *)tag_, tagstring, 4);

	if(! tag_valid(*tag_)){
		ERROR_LOG("`%s`", tagstring);
		return false;
	}
	return true;
}

//bool Fixed_init(Fixed *Fixed_, )

typedef struct{
	Fixed		version;
	uint16		numGlyphs;
}MaxpTable_SixByte;

bool MaxpTable_SixByte_init(MaxpTable_SixByte *maxpTable_sixByte_, unsigned int numGlyphs)
{
	ASSERT(maxpTable_sixByte_);
	if(0 == numGlyphs){
		WARN_LOG("numGlyphs is zero.");
	}

	MaxpTable_SixByte maxpTable_SixByte = {
		.version		= (Fixed)htonl(0x00005000),
		.numGlyphs		= numGlyphs,
	};

	*maxpTable_sixByte_ = maxpTable_SixByte;

	return true;
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
		.numTables		= htons(numTables_),
	};
	offsetTable.searchRange		= htons(numTables_ * 16);
	offsetTable.entrySelector	= htons((uint16)log2(numTables_));
	offsetTable.rangeShift		= htons((numTables_ * 16) - offsetTable.searchRange);

	*offsetTable_ = offsetTable;

	return true;
}

uint32 CalcTableChecksum(uint32 *table, uint32 numberOfBytesInTable)
{
	uint32 sum = 0;
	uint32 nLongs = (numberOfBytesInTable + 3) / 4;
	while (nLongs-- > 0)
		sum += *table++;
	return sum;
}

typedef struct{
	uint32 tag;			//!< table種別を表す識別子
	uint32 checkSum;		//!< テーブルのチェックサム
	uint32 offset;			//!< フォントファイル先頭からのオフセット
	uint32 length;			//!< テーブルの長さ
}TableDirectory_Member;

void TableDirectory_Member_init(TableDirectory_Member *self_, const char *tagstring, const uint8_t *tableData, size_t tableSize, uint32_t offset)
{
	ASSERT(self_);
	ASSERT(tagstring);
	ASSERT(tableData);
	ASSERT(0 < tableSize);

	TableDirectory_Member self;
	ASSERT(tag_init(&(self.tag), tagstring));
	self.checkSum	= htonl(CalcTableChecksum((uint32_t *)tableData, tableSize));
	self.offset	= htonl(offset);
	self.length	= htonl(tableSize);

	*self_ = self;

	return;
}

size_t TableSizeAlign(size_t size)
{
	int p = ((0 != (size % 4))? 1 : 0);
	size = ((size / 4) + p) * 4;
	return size;
}

typedef struct{
	TableDirectory_Member *tableDirectory;		//!< (TableDirectoryは構造体配列で表現することとする)
	uint8_t *data;
	size_t dataSize;
	unsigned int appendTableNum;
}Tablebuf;

void Tablebuf_init(Tablebuf *tableBuf)
{
	ASSERT(tableBuf);

	tableBuf->tableDirectory	= NULL;
	tableBuf->data			= NULL;
	tableBuf->dataSize		= 0;
	tableBuf->appendTableNum	= 0;
}

//! @note allocエラーは即時終了かつ発生呼び出し元トレースの必要はない
bool Tablebuf_appendTable(Tablebuf *tableBuf, const char *tagstring, const uint8_t *tableData, size_t tableSize)
{
	ASSERT(tableBuf);
	ASSERT(tagstring);
	ASSERT(tableData);
	ASSERT(0 < tableSize);

	// ** TableDirectory の作成
	// TableDirectoryを1メンバ分伸ばす
	tableBuf->tableDirectory = realloc(tableBuf->tableDirectory, sizeof(TableDirectory_Member) * tableBuf->appendTableNum);
	ASSERT(tableBuf->tableDirectory);
	const size_t offset = tableBuf->dataSize;
	// TableDirectoryにTableを追加
	TableDirectory_Member_init(
			&tableBuf->tableDirectory[tableBuf->appendTableNum],
			tagstring,
			tableData,
			tableSize,
			offset);		//!< ここでoffsetはTable数==TableDirectory長が判明するまで不明なので仮の値を入れておく

	// ** Tableバイト列の生成と連結 新たなTableを末尾に追加 // Table末尾は32bit allignかつzero padding
	const size_t alignedSize = TableSizeAlign(tableBuf->dataSize + tableSize);
	tableBuf->data		= realloc(tableBuf->data, tableBuf->dataSize + alignedSize);
	ASSERT(tableBuf->data);
	memset(&tableBuf->data[tableBuf->dataSize], 0x00, alignedSize);
	memcpy(&tableBuf->data[tableBuf->dataSize], tableData, tableSize);

	tableBuf->dataSize	+= alignedSize;
	tableBuf->appendTableNum	+= 1;

	DEBUG_LOG("table:%s %zd %zu %zd", tagstring, tableSize, alignedSize, offset);
	return true;
}

void Tablebuf_finallyTableDirectoryOffset(Tablebuf *tableBuf, size_t offsetHeadSize)
{
	ASSERT(tableBuf);
	ASSERT(0 < offsetHeadSize);

	for(int i = 0; i < (int)tableBuf->appendTableNum; i++){
		size_t offset = ntohl(tableBuf->tableDirectory[i].offset) + offsetHeadSize;
		tableBuf->tableDirectory[i].offset = htonl(offset);
	}
}

#pragma pack()

int main(int argc, char **argv)
{
	/**
	第1引数でフォントファイル名を指定する
	*/
	if(argc < 2){
		return 1;
	}

	int fd = open(argv[1], O_CREAT|O_TRUNC|O_RDWR, 0777);
	if(-1 == fd){
		fprintf(stderr, "open: %d %s\n", errno, strerror(errno));
		return 1;
	}

	/**
	TrueType・CFF共通の必須テーブル
	cmap, head, hhea, hmtx, maxp, name, OS/2
	を作成していく。
	*/


	/**
	'maxp' Table:
	 使用グリフ数。
	 TrueType必須Table。
	*/
	int numGlyphs = 0;
	MaxpTable_SixByte maxpTable_SixByte;
	ASSERT(MaxpTable_SixByte_init(&maxpTable_SixByte, numGlyphs));

	/**
	Tableをバイト配列に変換して繋げていく。
		Tableにはパディングを入れる。
		TableDirectoryを作っていく(Table情報の配列)。
		OffsetTable生成時に必要なテーブル数を数えておく。
	  */
	Tablebuf tableBuf;
	Tablebuf_init(&tableBuf);

	//Tablebuf_appendTable(&tableBuf, (void *)(&offsetTable), sizeof(OffsetTable));
	Tablebuf_appendTable(&tableBuf, "maxp", (void *)(&maxpTable_SixByte), sizeof(MaxpTable_SixByte));

	// offsetは、Tableのフォントファイル先頭からのオフセット。先に計算しておく。
	const size_t offsetHeadSize = sizeof(OffsetTable) + (sizeof(TableDirectory_Member) * tableBuf.appendTableNum);
	Tablebuf_finallyTableDirectoryOffset(&tableBuf, offsetHeadSize);

	/**
	OffsetTable:
	 (Offset Subtable, sfnt)
	*/
	uint32 sfntVersion;
	memcpy((uint8_t *)&sfntVersion, "OTTO", 4);
	OffsetTable offsetTable;
	ASSERT(OffsetTable_init(&offsetTable, sfntVersion, tableBuf.appendTableNum));

	ssize_t s;
	s = write(fd, (uint8_t *)&offsetTable, sizeof(OffsetTable));
	if(0 == s){
		fprintf(stderr, "write: %d %s\n", errno, strerror(errno));
	}
	DEBUG_LOG("OffsetTable :%zu", sizeof(OffsetTable));
	s = write(fd, tableBuf.tableDirectory, sizeof(TableDirectory_Member) * tableBuf.appendTableNum);
	if(0 == s){
		fprintf(stderr, "write: %d %s\n", errno, strerror(errno));
	}
	DEBUG_LOG("TableDirectory :%zu", sizeof(TableDirectory_Member) * tableBuf.appendTableNum);
	s = write(fd, tableBuf.data, tableBuf.dataSize);
	if(0 == s){
		fprintf(stderr, "write: %d %s\n", errno, strerror(errno));
	}
	DEBUG_LOG("Table :%zu", tableBuf.dataSize);
	close(fd);
	return 0;
}

