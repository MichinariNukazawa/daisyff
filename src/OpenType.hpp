/**
  @file
  @author michianri.nukazawa@gmail.com / project daisy bell
  @details license: MIT
 */

#ifndef DAISYFF_OPEN_TYPE_HPP_
#define DAISYFF_OPEN_TYPE_HPP_

// strptime()のために先頭に置く
#define _XOPEN_SOURCE
#include <time.h>

#include <stdlib.h>

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
#include <stddef.h>

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

/* ********
 * FontFormat 基本データ型
 * ******** **/

typedef int16_t  int16;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint32_t tag;
typedef uint32_t Fixed;
typedef uint32_t LONGDATETIME;

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

Fixed Fixed_generate(uint8_t major, uint8_t minor)
{
	uint16_t fixed = (uint16_t)(major << 8) & (uint16_t)minor;
	return (Fixed)htons(fixed);
}

time_t timeFromStr(const char *time_details)
{
	struct tm tm;
	memset(&tm, 0, sizeof(struct tm));
	//! ex. "2016-09-14T14:46:13+00:00"
	char *r = strptime(time_details, "%Y-%m-%dT%H:%M:%S%z", &tm);
	// 全部変換されたら末尾のヌルバイトが返る
	ASSERT(NULL != r);
	ASSERT('\0' == *r);
	//{
	//	char buf[255];
	//	strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S%z", &tm);
	//	DEBUG_LOG("`%s` m:%d,d:%d,H:%d,M:%d UNIX time:%ld `%s`", // mtの月は 0-11 表現
	//		time_details, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min,
	//		mktime(&tm), buf);
	//}
	/** mktime()でUTC時間を渡すために、TZ環境変数をUTCにセットする。戻す処理は省略。
	https://linuxjm.osdn.jp/html/LDP_man-pages/man3/timegm.3.html */
	putenv("TZ="); //!< `warning: implicit declaration of function ‘setenv’;`を避ける。
	tzset();
	time_t time = mktime(&tm);
	ASSERT(-1 != time);
	return time;
}

LONGDATETIME LONGDATETIME_generate(time_t time)
{
	/**
	Unixエポック: UNIX時間（1970年1月1日 UTC(協定世界時)0時00分00秒
		date --date "1970-01-01T00:00:00+00:00" +%s // 0
	LONGDATETIME: 12:00 midnight, January 1, 1904
		date --date "1904-01-01T00:00:00+00:00" +%s // -2082844800
	 */
	const uint64_t LONGDATETIME_DELTA = 2082844800;
	//! @todo INT64_MAXを超えるかチェックしていない
	LONGDATETIME t = (LONGDATETIME)((uint64_t)time + LONGDATETIME_DELTA);
	return t;
}

/** ********
 * 内部型(SPECで定義されていないが取り回し等であると便利な型)
 *  ******** **/


enum HeadTableFlagsElement_Bit{
	HeadTableFlagsElement_Bit0_isBaselineAtYIsZero			= (0x1 << 0),
	HeadTableFlagsElement_Bit1_isLeftSidebearingPointAtXIsZero	= (0x1 << 1),
	HeadTableFlagsElement_Bit2_isDependPointSize			= (0x1 << 2),
	HeadTableFlagsElement_Bit3_isPpemScalerMath			= (0x1 << 3),
	HeadTableFlagsElement_Bit4_isAdvanceWidth			= (0x1 << 4),
	//! OpenTypeでは0固定
	HeadTableFlagsElement_Bit5_isRegardingInApplePlatform		= (0x1 << 5),
	//! Bits 6–10: These bits are not used in Opentype and should always be cleared. // 0固定
	HeadTableFlagsElement_Bit11_isLosslessCompress			= (0x1 << 11),
	//! MS SPECに"Font converted"としか書いていないため不明 とりあずON(1)?
	HeadTableFlagsElement_Bit12_isFontConverted			= (0x1 << 12),
	//! ClearType(アンチエイリアシング) // bitmapフォント等で無効にする場合が有る
	HeadTableFlagsElement_Bit13_isClearType				= (0x1 << 13),
	//! glyphの並びをCodePoint順にする
	HeadTableFlagsElement_Bit14_isResortforCodePoint		= (0x1 << 14),
	//! Bit 15 zero予約
};
typedef uint16_t HeadTableFlagsElement;

enum MacStyle_Bit{
	MacStyle_Bit0_Italic		= (0x1 << 0),
	MacStyle_Bit1_Underscore	= (0x1 << 1),
	MacStyle_Bit2_Negative		= (0x1 << 2),
	MacStyle_Bit3_Outlined		= (0x1 << 3),
	MacStyle_Bit4_Strikeout		= (0x1 << 4),
	MacStyle_Bit5_Bold		= (0x1 << 5),
	MacStyle_Bit6_Regular		= (0x1 << 6),
	//! @todo その他省略
};
typedef uint16_t MacStyle;

/** bounding box
  */
typedef struct{
	int16_t			xMin;
	int16_t			yMin;
	int16_t			xMax;
	int16_t			yMax;
}BBox;

BBox BBox_generate(int16_t xMin, int16_t yMin, int16_t xMax, int16_t yMax)
{
	ASSERT(xMin < xMax);
	ASSERT(yMin < yMax);
	BBox bbox = {xMin, yMin, xMax, yMax};
	return bbox;
}

/* ********
 * FontFormat Tableデータ表現構造体
 * ******** **/

#pragma pack (1)

typedef struct{
	uint16			majorVersion;		// 1固定
	uint16			minorVersion;		// 0固定
	Fixed			fontRevision;		// フォント製造者が付けるリビジョン番号
	uint32			checkSumAdjustment;	// フォント全体のチェックサム(別述)
	uint32			magicNumber;		// 0x5F0F3CF5固定
	uint16			flags;			// フラグ(別述)
	uint16			unitsPerEm;		// ユニット数
		/*  範囲16-16384でTrueTypeアウトラインでは速度最適化のため2の倍数を推奨。*/
	LONGDATETIME		created;		// 作成日時
	LONGDATETIME		modified;		// 更新日時
	int16			xMin;			// bounding box
	int16			yMin;			//
	int16			xMax;			//
	int16			yMax;			//
	uint16			macStyle;		// スタイル（Bold,Italic等）
		/* windowsではOS/2 TableのfsSelection要素とbitアサインが共通 */
	uint16			lowestRecPPEM;		// 可読なピクセル数の下限
	int16			fontDirectionHint;	// 廃止されたヒント情報(2固定)
	int16			indexToLocFormat;	// 不明。
		/* 0:short offset, 1:long offsetらしい。とりあえず0でよさそう。 */
	int16			glyphDataFormat;	// 0固定
}HeadTable;

bool HeadTable_init(
		HeadTable		*headTable_,
		Fixed			fontRevision,
		HeadTableFlagsElement	flags,
		LONGDATETIME		created,
		LONGDATETIME		modified,
		MacStyle		macStyle,
		BBox			bbox,
		uint16			lowestRecPPEM
		)
{
	HeadTable headTable = {
		.majorVersion		= htonl(1),	// 1固定
		.minorVersion		= htonl(0),	// 0固定
		.fontRevision		= fontRevision,
		.checkSumAdjustment	= htonl(0),	// ゼロ埋めしておき最後にCheckSumを計算する
		.magicNumber		= htonl(0x5F0F3CF5),
		.flags			= flags,
		.unitsPerEm		= htons(1024),
		.created		= created,
		.modified		= modified,
		.xMin			= htons(bbox.xMin),
		.yMin			= htons(bbox.yMin),
		.xMax			= htons(bbox.xMax),
		.yMax			= htons(bbox.yMax),
		.macStyle		= macStyle,
		.lowestRecPPEM		= htons(lowestRecPPEM),
		.fontDirectionHint	= htons(2),
		.indexToLocFormat	= htons(0),	// 不明。
		.glyphDataFormat	= htons(0),	// 0固定
	};
	*headTable_ = headTable;

	return true;
}

//! @notice headTable and data arguments is no strict alias
bool HeadTable_CalcCheckSumAdjustmentElement(const uint8_t *data_, size_t dataSize_)
{
	const uint32_t *data = (const uint32_t *)data_;
	size_t dataSize = dataSize_ / 4;

	return true;
}

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
		.numGlyphs		= htons(numGlyphs),
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
		.sfntVersion		= htonl(sfntVersion),
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
	DEBUG_LOG("in table:`%s` dataSize:%zu appendTableNum:%d tableSize:%zu", tagstring, tableBuf->dataSize, tableBuf->appendTableNum, tableSize);

	ASSERT(tableBuf);
	ASSERT(tagstring);
	ASSERT(tableData);
	ASSERT(0 < tableSize);

	// ** TableDirectory の作成
	// TableDirectoryを1メンバ分伸ばす
	tableBuf->tableDirectory = realloc(tableBuf->tableDirectory, sizeof(TableDirectory_Member) * (tableBuf->appendTableNum + 1));
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
	const size_t alignedSize = TableSizeAlign(tableSize);
	tableBuf->data = realloc(tableBuf->data, tableBuf->dataSize + alignedSize);
	ASSERT(tableBuf->data);
	memset(&tableBuf->data[tableBuf->dataSize], 0x00, alignedSize);
	memcpy(&tableBuf->data[tableBuf->dataSize], tableData, tableSize);

	tableBuf->dataSize		+= alignedSize;
	tableBuf->appendTableNum	+= 1;

	DEBUG_LOG("out table:`%s` %zu %zu %zu", tagstring, tableSize, alignedSize, offset);
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

#endif // #ifndef DAISYFF_OPEN_TYPE_HPP_

