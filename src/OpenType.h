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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <errno.h>

#include "src/Util.h"
#include "src/GlyphOutline.h"

// * ********
// * Basic font format type 基本データ型
// * ********

typedef uint8_t  Uint8Type;
typedef int16_t  Int16Type;
typedef uint16_t Uint16Type;
typedef uint32_t Uint32Type;
typedef uint32_t TagType;
typedef uint32_t FixedType;
typedef uint64_t LONGDATETIMEType;
//! @note 命名規則はCamelCaseだがLONGDATETIMEは見た目が締まるので大文字のままとした

typedef uint16_t Offset16Type;
typedef uint32_t Offset32Type;

/**
  */
bool TagType_valid(const TagType tag_)
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

bool TagType_init(TagType *tag_, const char *tagstring)
{
	ASSERT(tag_);
	ASSERT(tagstring);

	if(4 != strlen(tagstring)){
		ERROR_LOG("%zu `%s`", strlen(tagstring), tagstring);
		return false;
	}

	memcpy((void *)tag_, tagstring, 4);

	if(! TagType_valid(*tag_)){
		ERROR_LOG("`%s`", tagstring);
		return false;
	}
	return true;
}

uint16_t Bcd4Type_ConvertHexFromDecimal(unsigned int dec)
{
	ASSERT(dec <= 0xffff);

	uint16_t hex = 0;
	for(int i = 0; i < 4; i++){
		unsigned int decv = (dec / (unsigned int)pow(10, i)) % 10;
		uint16_t hexv = decv * ((uint16_t)0x1 << (i * 4));
		hex += hexv;
		//DEBUG_LOG("%u 0x%04x, 0x%04x", decv, hexv, hex);
	}

	return hex;
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
	char tz[] = "TZ="; //!< warning: -Werror=discarded-qualifiersを避ける
	putenv(tz); //!< `warning: implicit declaration of function ‘setenv’;`を避ける。
	tzset();
	time_t time = mktime(&tm);
	ASSERT(-1 != time);
	return time;
}

const uint64_t LONGDATETIME_DELTA = 2082844800;
LONGDATETIMEType LONGDATETIMEType_generate(time_t time)
{
	/**
	Unixエポック: UNIX時間（1970年1月1日 UTC(協定世界時)0時00分00秒
		date --date "1970-01-01T00:00:00+00:00" +%s // 0
	LONGDATETIMEType: 12:00 midnight, January 1, 1904
		date --date "1904-01-01T00:00:00+00:00" +%s // -2082844800
	 */
	//! @todo INT64_MAXを超えるかチェックしていない
	LONGDATETIMEType t = (LONGDATETIMEType)((uint64_t)time + LONGDATETIME_DELTA);
	return t;
}

/** ********
 * font format 内部型(SPECで定義されていないが取り回し等であると便利な型)
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
	/** MSSPECに"Font converted"としか書いていないため不明。
	  AppleSPECにはAdobe定義と書いてあってリンク切れしてる。
	  とりあずNotoに習ってON(1)
	  */
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
	Uint16Type		majorVersion;		// 1固定
	Uint16Type		minorVersion;		// 0固定
	FixedType		fontRevision;		// フォント製造者が付けるリビジョン番号
	Uint32Type		checkSumAdjustment;	// フォント全体のチェックサム(別述)
	Uint32Type		magicNumber;		// 0x5F0F3CF5固定
	Uint16Type		flags;			// フラグ(別述)
	Uint16Type		unitsPerEm;		// ユニット数
		/*  範囲16-16384でTrueTypeアウトラインでは速度最適化のため2の倍数を推奨。*/
	LONGDATETIMEType		created;		// 作成日時
	LONGDATETIMEType		modified;		// 更新日時
	Int16Type		xMin;			// bounding box
	Int16Type		yMin;			//
	Int16Type		xMax;			//
	Int16Type		yMax;			//
	Uint16Type		macStyle;		// スタイル（Bold,Italic等）
		/* windowsではOS/2 TableのfsSelection要素とbitアサインが共通 */
	Uint16Type		lowestRecPPEM;		// 可読なピクセル数の下限
	Int16Type		fontDirectionHint;	// 廃止されたヒント情報(2固定)
	Int16Type		indexToLocFormat;	// 不明。
		/* 0:short offset, 1:long offsetらしい。とりあえず0でよさそう。 */
	Int16Type		glyphDataFormat;	// 0固定
}HeadTable;

bool HeadTable_init(
		HeadTable		*headTable_,
		FixedType		fontRevision,
		HeadTableFlagsElement	flags,
		LONGDATETIMEType	created,
		LONGDATETIMEType	modified,
		MacStyle		macStyle,
		BBox			bbox,
		Uint16Type		lowestRecPPEM
		)
{
	HeadTable headTable = {
		.majorVersion		= htons(1),	// 1固定
		.minorVersion		= htons(0),	// 0固定
		.fontRevision		= htonl(fontRevision),
		.checkSumAdjustment	= htonl(0),	// ゼロ埋めしておき最後にCheckSumを計算する
		.magicNumber		= htonl(0x5F0F3CF5),
		.flags			= htons(flags),
		.unitsPerEm		= htons(1024),
		.created		= htonll(created),
		.modified		= htonll(modified),
		.xMin			= htons(bbox.xMin),
		.yMin			= htons(bbox.yMin),
		.xMax			= htons(bbox.xMax),
		.yMax			= htons(bbox.yMax),
		.macStyle		= htons(macStyle),
		.lowestRecPPEM		= htons(lowestRecPPEM),
		.fontDirectionHint	= htons(2),
		.indexToLocFormat	= htons(1),	// 'loca' Table要素サイズ(1==Offset32)
		.glyphDataFormat	= htons(0),	// 0固定
	};
	*headTable_ = headTable;

	return true;
}

typedef struct{
	FixedType		version;
	Uint16Type	numGlyphs;
}MaxpTable_Version05;

bool MaxpTable_Version05_init(MaxpTable_Version05 *maxpTable_Version05_, unsigned int numGlyphs)
{
	ASSERT(maxpTable_Version05_);
	if(0 == numGlyphs){
		WARN_LOG("numGlyphs is zero.");
	}

	MaxpTable_Version05 maxpTable_Version05 = {
		.version		= (FixedType)htonl(0x00005000),
		.numGlyphs		= htons(numGlyphs),
	};

	*maxpTable_Version05_ = maxpTable_Version05;

	return true;
}

typedef struct{
	Uint16Type		format;
	Uint16Type		count;
	Offset16Type		stringOffset;
	// NameRecord[count]
	// (Variable) string strage
}NameTableHeader_Format0;

const char *MacStyle_toStringForNameTable(MacStyle macStyle)
{
	if(macStyle == (MacStyle_Bit0_Italic | MacStyle_Bit5_Bold)){
		return "Bold Italic";
	}
	if(macStyle == MacStyle_Bit0_Italic){
		return "Italic";
	}
	if(macStyle == MacStyle_Bit5_Bold){
		return "Bold";
	}
	if(macStyle == MacStyle_Bit6_Regular){
		return "Regular";
	}

	return NULL;
}

typedef struct{
	Uint16Type	platformID;
	Uint16Type	encodingID;
	Uint16Type	languageID;
	Uint16Type	nameID;
	Uint16Type	length;
	Offset16Type	offset;
}NameRecord_Member;

typedef struct{
	Uint16Type		format;
	Uint16Type		count;
	Offset16Type		stringOffset;
	NameRecord_Member	*nameRecord;
	uint8_t			*stringStrage;
	size_t 			stringStrageSize;
	uint8_t			*data;
	size_t			dataSize;
}NameTableBuf;

enum PlatformID{
	PlatformID_Unicode		= 0,
};
enum EncodingID{
	EncodingID_Unicode_0		= 0,
};
typedef Uint16Type PlatformID;
typedef Uint16Type EncodingID;
typedef Uint16Type LanguageID;
typedef Uint16Type NameID;

bool PostScriptName_valid(const char *str)
{
	const char invalids[] = {'[', ']', '(', ')', '{', '}', '<', '>', '/', '%'};
	for(int i = 0; i < strlen(str); i++){
		if(0 == isprint(str[i])){
			DEBUG_LOG("0x%02x[%d] is not printable.", str[i], i);
			return false;
		}
		for(int t = 0; t < sizeof(invalids); t++){
			if(invalids[t] == str[i]){
				DEBUG_LOG("0x%02x[%d] is invalid character in PostScriptName.", str[i], i);
				return false;
			}
		}
	}

	return true;
}

uint8_t *convertNewUtf16FromUtf8(const char *stringdata)
{
	//! @todo ASCIIしか変換できない。(とりあえずcopyrightマークに非対応な状態)
	uint8_t *utf16s = (uint8_t *)ffmalloc((strlen(stringdata) * 2) + 2);
	for(int i = 0; i < strlen(stringdata); i++){
		utf16s[(i * 2) + 0] = 0x00;
		utf16s[(i * 2) + 1] = stringdata[i];
		utf16s[(i * 2) + 2] = 0x00;
		utf16s[(i * 2) + 3] = 0x00;
	}

	return utf16s;
}

void NameTableBuf_append(
		NameTableBuf *nameTableBuf,
		PlatformID platformID,
		EncodingID encodingID,
		LanguageID languageID,
		NameID nameID,
		const char *stringdata)
{
	ASSERT(nameTableBuf);
	ASSERT(stringdata);
	DEBUG_LOG("nameTableBuf->count:%d nameID:%2d`%s`", nameTableBuf->count, nameID, stringdata);

	size_t utf16sSize = strlen(stringdata) * 2;

	// ** NameTable.nameRecord[]に新しいNameRecordを追加。
	nameTableBuf->nameRecord = (NameRecord_Member *)ffrealloc(
			nameTableBuf->nameRecord,
			sizeof(NameRecord_Member) * (nameTableBuf->count + 1));
	NameRecord_Member nameRecord_Member = {
		.platformID	= htons(platformID),
		.encodingID	= htons(encodingID),
		.languageID	= htons(languageID),
		.nameID		= htons(nameID),
		.length		= htons(utf16sSize),
		.offset		= htons(nameTableBuf->stringStrageSize),
	};
	nameTableBuf->nameRecord[nameTableBuf->count] = nameRecord_Member;
	(nameTableBuf->count)++;

	// ** string strageを拡張して後ろに文字列データを追加
	//DEBUG_LOG("%zu %zu", nameTableBuf->stringStrageSize, strlen(stringdata));
	uint8_t *utf16s = convertNewUtf16FromUtf8(stringdata);
	size_t newsize = nameTableBuf->stringStrageSize + utf16sSize;
	nameTableBuf->stringStrage = (uint8_t *)ffrealloc(nameTableBuf->stringStrage, newsize);
	memcpy(&nameTableBuf->stringStrage[nameTableBuf->stringStrageSize], utf16s, utf16sSize);
	nameTableBuf->stringStrageSize = newsize;
}

void NameTableBuf_generateByteData(NameTableBuf *nameTableBuf)
{
	ASSERT(nameTableBuf);
	ASSERT(NULL == nameTableBuf->data); // 再実行はしない

	// byte dataメモリ確保
	size_t nameTableSize = sizeof(NameTableHeader_Format0)
		+ (sizeof(NameRecord_Member) * nameTableBuf->count)
		+ nameTableBuf->stringStrageSize;
	nameTableBuf->data		= (uint8_t *)ffmalloc(nameTableSize);
	nameTableBuf->dataSize		= nameTableSize;

	size_t stringOffset = sizeof(NameTableHeader_Format0) + (sizeof(NameRecord_Member) * nameTableBuf->count);

	// Table先頭部分を埋める
	NameTableHeader_Format0 nameTableHeader_Format0 = {
			.format		= htons(0),
			.count		= htons(nameTableBuf->count),
			.stringOffset	= htons(stringOffset),
	};
	memcpy(&nameTableBuf->data[0], &nameTableHeader_Format0, sizeof(NameTableHeader_Format0));

	// NameRecord[count]
	memcpy(&(nameTableBuf->data[sizeof(NameTableHeader_Format0)]),
			nameTableBuf->nameRecord,
			sizeof(NameRecord_Member) * nameTableBuf->count);

	// (Variable) string strage
	memcpy(&nameTableBuf->data[stringOffset], nameTableBuf->stringStrage, nameTableBuf->stringStrageSize);
}

NameTableBuf NameTableBuf_init(
			const char *copyright,
			const char *fontname,
			MacStyle    macStyle,
			const char *versionString,
			const char *vendorname,
			const char *designername,
			const char *vendorurl,
			const char *designerurl
		)
{
	//! @todo argument validation

	NameTableBuf nameTableBuf = {
		.format			= 0,
		.count			= 0,
		.stringOffset		= sizeof(NameTableHeader_Format0),
		.nameRecord		= NULL,
		.stringStrage		= NULL,
		.stringStrageSize	= 0,
		.data			= NULL,
		.dataSize		= 0,
	};

	// NameTable.NameRecord[](および.stringstrage)にNameRecord_Menberを追加。
	const char *macStyleString = MacStyle_toStringForNameTable(macStyle);
	ASSERT(macStyleString);
	const char *appfullfontname		= ffsprintf_new("%s %s %s", vendorname, fontname, macStyleString);
	const char *humanfullfontname		= ffsprintf_new("%s %s", fontname, macStyleString);
	const char *postscriptfontname		= ffsprintf_new("%s-%s", fontname, macStyleString);
	ASSERTF(PostScriptName_valid(postscriptfontname), "`%s`", postscriptfontname);
	NameTableBuf_append(&nameTableBuf, PlatformID_Unicode, EncodingID_Unicode_0, 0x0,  0, copyright);
	NameTableBuf_append(&nameTableBuf, PlatformID_Unicode, EncodingID_Unicode_0, 0x0,  1, fontname);
	NameTableBuf_append(&nameTableBuf, PlatformID_Unicode, EncodingID_Unicode_0, 0x0,  2, macStyleString);
	NameTableBuf_append(&nameTableBuf, PlatformID_Unicode, EncodingID_Unicode_0, 0x0,  3, appfullfontname);
	NameTableBuf_append(&nameTableBuf, PlatformID_Unicode, EncodingID_Unicode_0, 0x0,  4, humanfullfontname);
	NameTableBuf_append(&nameTableBuf, PlatformID_Unicode, EncodingID_Unicode_0, 0x0,  5, versionString);
	NameTableBuf_append(&nameTableBuf, PlatformID_Unicode, EncodingID_Unicode_0, 0x0,  6, postscriptfontname);
	NameTableBuf_append(&nameTableBuf, PlatformID_Unicode, EncodingID_Unicode_0, 0x0,  8, vendorname);
	NameTableBuf_append(&nameTableBuf, PlatformID_Unicode, EncodingID_Unicode_0, 0x0,  9, designername);
	NameTableBuf_append(&nameTableBuf, PlatformID_Unicode, EncodingID_Unicode_0, 0x0, 11, vendorurl);
	NameTableBuf_append(&nameTableBuf, PlatformID_Unicode, EncodingID_Unicode_0, 0x0, 12, designerurl);

	// NameTableのbufferからbyteデータを作成
	NameTableBuf_generateByteData(&nameTableBuf);

	return nameTableBuf;
}

typedef struct{
	Int16Type numberOfContours;
	Int16Type xMin;
	Int16Type yMin;
	Int16Type xMax;
	Int16Type yMax;
}GlyphDescriptionHeader;

typedef struct{
	// GlyphDescriptionHeader
	int16_t		numberOfContours;
	int16_t		xMin;
	int16_t		yMin;
	int16_t		xMax;
	int16_t		yMax;
	// SimpleGlyphDescription(Body)
	//uint16_t	*endPoints;
	uint16_t	instructionLength;
	uint8_t		*instructions;
	// for debug
	uint8_t		*flags;
	int16_t		*xCoodinates;
	int16_t		*yCoodinates;
	size_t		pointNum;
	//
	size_t		dataSize;
	uint8_t		*data;
}GlyphDescriptionBuf;

void GlyphDescriptionBuf_generateByteDataWithOutline(
		GlyphDescriptionBuf *glyphDescriptionBuf,
		const GlyphOutline *outline)
{
	ASSERT(glyphDescriptionBuf);
	ASSERT(NULL == glyphDescriptionBuf->data);

	// ** pointNumカウントとEndPoints収集を行う
	//ASSERT(0 < outline->closePathNum);
	glyphDescriptionBuf->numberOfContours = outline->closePathNum;
	uint16_t *endPoints = ffmalloc(sizeof(uint16_t) * glyphDescriptionBuf->numberOfContours);
	size_t pointNum = 0;
	for(int l = 0; l < outline->closePathNum; l++){
		const GlyphClosePath *closePath = &(outline->closePaths[l]);
		ASSERT(0 < closePath->anchorPointNum);
		pointNum += closePath->anchorPointNum;
		endPoints[l] = pointNum - 1;
	}

	// ** flags,x,yCoodinates収集を行う // @todo 短縮・SHORT_VECTOR
	uint8_t *flags = ffmalloc(sizeof(uint8_t) * pointNum);
	int16_t *xCoodinates = ffmalloc(sizeof(int16_t) * pointNum);
	int16_t *yCoodinates = ffmalloc(sizeof(int16_t) * pointNum);
	int n = 0;
	int16_t prex = 0;
	int16_t prey = 0;
	for(int l = 0; l < outline->closePathNum; l++){
		const GlyphClosePath *closePath = &(outline->closePaths[l]);
		for(int ai = 0; ai < closePath->anchorPointNum; ai++){
			const GlyphAnchorPoint *ap = &(closePath->anchorPoints[ai]);
			flags[n] = 0x01;
			xCoodinates[n] = (ap->point).x - prex;
			yCoodinates[n] = (ap->point).y - prey;
			prex = (ap->point).x;
			prey = (ap->point).y;
			n++;
		}
	}
	ASSERT_EQ_INT(pointNum, n);

	// ** byte dataメモリ確保
	glyphDescriptionBuf->dataSize
		= sizeof(GlyphDescriptionHeader)				// GlyphDescriptionHeader
		+ (sizeof(uint16_t) * glyphDescriptionBuf->numberOfContours)	// endPoints[numberOfContours]
		+ (sizeof(uint16_t))						// instructionLength
		+ (sizeof(uint8_t) * glyphDescriptionBuf->instructionLength)	// instructions[instructionLength]
		+ (sizeof(uint8_t) * pointNum)		// flags[] // 短縮は未実装
		+ (sizeof(int16_t) * pointNum)		// xCoodinates[] // SHORT_VECTORは未実装
		+ (sizeof(int16_t) * pointNum)		// yCoodinates[] // SHORT_VECTORは未実装
		;
	//DEBUG_LOG("glyphDescriptionBuf->dataSize:%zu", glyphDescriptionBuf->dataSize);
	glyphDescriptionBuf->data = ffmalloc(glyphDescriptionBuf->dataSize);

	GlyphDescriptionHeader glyphDescriptionHeader = {
		.numberOfContours	= htons(glyphDescriptionBuf->numberOfContours),
		.xMin			= htons(0),
		.yMin			= htons(0),
		.xMax			= htons(1000),
		.yMax			= htons(1000),
	};

	size_t offset = 0;
	size_t wsize;
	uint16_t v16;
	// GlyphDescriptionHeader
	wsize = sizeof(GlyphDescriptionHeader);
	memcpy(&(glyphDescriptionBuf->data[offset]), &glyphDescriptionHeader, wsize);
	offset += wsize;
	// endPoints[numberOfContours]
	htonArray16Move(&(glyphDescriptionBuf->data[offset]), endPoints, glyphDescriptionBuf->numberOfContours);
	offset += sizeof(uint16_t) * glyphDescriptionBuf->numberOfContours;
	// instructionLength
	wsize = sizeof(uint16_t);
	v16 = htons(glyphDescriptionBuf->instructionLength);
	memcpy(&(glyphDescriptionBuf->data[offset]), &v16, wsize);
	offset += wsize;
	// instructions[instructionLength]
	wsize = (sizeof(uint8_t) * glyphDescriptionBuf->instructionLength);
	memcpy(&(glyphDescriptionBuf->data[offset]), glyphDescriptionBuf->instructions, wsize);
	offset += wsize;
	// flags[] // 短縮は未実装
	wsize = (sizeof(uint8_t) * pointNum);
	memcpy(&(glyphDescriptionBuf->data[offset]), flags, wsize);
	offset += wsize;
	// xCoodinates[] // SHORT_VECTORは未実装
	wsize = (sizeof(int16_t) * pointNum);
	htonArray16Move(&(glyphDescriptionBuf->data[offset]), (uint16_t *)xCoodinates, pointNum);
	offset += wsize;
	// yCoodinates[] // SHORT_VECTORは未実装
	wsize = (sizeof(int16_t) * pointNum);
	htonArray16Move(&(glyphDescriptionBuf->data[offset]), (uint16_t *)yCoodinates, pointNum);
	offset += wsize;

	// デバッグ情報を残す
	glyphDescriptionBuf->flags		= flags;
	glyphDescriptionBuf->xCoodinates	= xCoodinates;
	glyphDescriptionBuf->yCoodinates	= yCoodinates;
	glyphDescriptionBuf->pointNum		= pointNum;
}

typedef struct{
	Uint16Type		version;
	Uint16Type		numTables;
	// EncodingRecord EncodingRecords[numTables]
}CmapTableHeader;

typedef struct{
	Uint16Type		platformID;
	Uint16Type		encodingID;
	Offset32Type		offset;
	// 'cmap' subtables
}CmapTable_EncodingRecordElementHeader;

#define CmapSubtableFormat0_ARRAY_SIZE (256)	// 1byte ascii
#define CmapSubtableFormat4_ARRAY_SIZE (65536)	// 2byte unicode
#define CmapSubtableFormat0_CODEPOINT_MAX (255)
#define CmapSubtableFormat4_CODEPOINT_MAX (65534)
/** @notice
  0xffff == 65535はCmapTable.subtable.format4.endCodeにmissingGlyphとして予約されている。
  明示的に使用禁止されていないが、MSSPECを見る限り使えないと考えられるし、
  使える場合も正しい処理を考えるのが大変そうなので、daisyffでは使用しないものとする
  */

typedef struct{
	Uint16Type		format;
	Uint16Type		length;
	Uint16Type		language;
	Uint8Type		glyphIdArray[CmapSubtableFormat0_ARRAY_SIZE];
}CmapTable_CmapSubtable_Format0;

typedef struct{
	Uint16Type	format;				//
	Uint16Type	length;				//
	Uint16Type	language;			//
	Uint16Type	segCountX2;			//
	Uint16Type	searchRange;			//
	Uint16Type	entrySelector;			//
	Uint16Type	rangeShift;			//
	Uint16Type	*endCode;			// endCode[segCount]
	Uint16Type	reservedPad;			// reservedPad
	Uint16Type	*startCode;			// startCode[segCount]
	Int16Type	*idDelta;			// idDelta[segCount]
	Uint16Type	*idRangeOffset;			// idRangeOffset[segCount]
	Uint16Type	*glyphIdArray;			// glyphIdArray[ ]
}CmapTable_CmapSubtable_Format4Buf;

typedef struct{
	Uint16Type	startCode;
	Uint16Type	endCode;
	Int16Type	idDelta;
	Uint16Type	idRangeOffset;
}CmapSubtable_Format4_SegmentBuf;

void CmapTableHeader_init(CmapTableHeader *cmapTableHeader, size_t numTables)
{
	ASSERT(0 < numTables && numTables <= UINT16_MAX);
	uint16_t numTables_ = numTables;

	*cmapTableHeader = (CmapTableHeader){
		.version	= htons(0),
		.numTables	= htons(numTables_),
	};
};

CmapTable_EncodingRecordElementHeader CmapTable_EncodingRecordElementHeader_generate(
		uint16_t platformId,
		uint16_t encodingId,
		size_t offset)
{
	return (CmapTable_EncodingRecordElementHeader){
		.platformID	= htons(platformId),
		.encodingID	= htons(encodingId),
		.offset		= htonl(offset),
	};
}

void CmapTable_CmapSubtable_Format0_finally(CmapTable_CmapSubtable_Format0 *format0, size_t numGlyph)
{
	size_t FORMAT0_HEADER_SIZE = sizeof(Uint16Type) * 3; // format, length, language

	format0->format		= htons(0);
	//format0->length		= htons(FORMAT0_HEADER_SIZE + numGlyph);
	format0->length		= htons(FORMAT0_HEADER_SIZE + CmapSubtableFormat0_ARRAY_SIZE);
	format0->language	= htons(0);
	//format0.glyphIdArray	= {0},
}

FFByteArray CmapTable_CmapSubtable_Format4_generateByteDataWithGlyphIdArray16(
		uint16_t languageId,
		uint16_t *glyphIdArray)
{
	ASSERT(glyphIdArray);

	// ** segmentsを収集
	CmapSubtable_Format4_SegmentBuf segmentBufs[CmapSubtableFormat4_ARRAY_SIZE];
	size_t segCount = 0;
	for(int c = 0; c <= CmapSubtableFormat4_CODEPOINT_MAX; c++){
		// 末尾セグメント用に予約されているはず
		// CmapSubtableFormat4_CODEPOINT_MAX 定義付近にコメント書いた
		ASSERTF(0xffff != c, "%d", c);

		if(0 == glyphIdArray[c]){ //!< glyphなし
			continue;
		}
		if(0 == c){ // .notdef
			continue;
		}
		// daisyffにおいて空グリフ,水平タブ(0x09) (fontforge生成ファイルでは収録されなかったので略)
		if(1 == glyphIdArray[c] || 2 == glyphIdArray[c]){
			continue;
		}

		//!< @todo 今回はsegment形式マッピングで圧縮するほど文字数もないため、
		// 処理を略し文字毎にsegmentを切ってしまう
		Int16Type idDelta = glyphIdArray[c] - c;
		segmentBufs[segCount] = (CmapSubtable_Format4_SegmentBuf){
			.startCode	= c,
			.endCode	= c,
			.idDelta	= idDelta, // とりあえず今回は計算が合うと思うが...
			.idRangeOffset	= 0,
		};
		DEBUG_LOG("seg:%zu start:0x%04x glyphId:%u code:0x%04x(%u) delta:%d",
				segCount, segmentBufs[segCount].startCode, glyphIdArray[c], c, c, idDelta);
		segCount++;

		ASSERT(segCount <= (UINT16_MAX / 2) - 1); // CmapTable.subtable.format4.segCountX2の最大数から末尾segment分を引く
	}
	// 末尾segmentsを生成
	{
		segmentBufs[segCount] = (CmapSubtable_Format4_SegmentBuf){
			.startCode	= 0xffff,
			.endCode	= 0xffff,
			.idDelta	= 0,
			.idRangeOffset	= 0,
		};
		segCount++;
	}
	ASSERT(segCount <= (UINT16_MAX / 2));

	// ** byte array生成
	FFByteArray array = {0};
	size_t segArrayElementSize = sizeof(Uint16Type) * segCount;
	size_t reserveSize = sizeof(Uint16Type);
	size_t fixedheadSize	= (sizeof(Uint16Type) * 7);
	size_t segmentsSize	= reserveSize + (segArrayElementSize * 4);
	size_t glyphIdArraySize	= 0;
	Uint16Type length = fixedheadSize + segmentsSize + glyphIdArraySize;
	DEBUG_LOG("segCount:%zu length:%u(0x%08x)", segCount, length, (uint32_t)length);

	// *** fixed length 部分
	FFByteArray_realloc(&array, length);
	uint16_t segCountX2	= segCount * 2;
	uint16_t searchRange	= (2 * 2 * (int)floor(log2(segCount)));
	uint16_t entrySelector	= ((int)log2(searchRange/2.0));
	uint16_t rangeShift	= 2 * segCount - searchRange;
	uint16_t *data16 = (uint16_t *)array.data;
	data16[0] = htons(4);			//Uint16Type	format
	data16[1] = htons(length);			//Uint16Type	length
	data16[2] = htons(languageId);		//Uint16Type	language
	data16[3] = htons(segCountX2	);	//Uint16Type	segCountX2
	data16[4] = htons(searchRange	);	//Uint16Type	searchRange
	data16[5] = htons(entrySelector	);	//Uint16Type	entrySelector
	data16[6] = htons(rangeShift	);	//Uint16Type	rangeShift

	// *** segments
	Uint16Type	*endCode	= (Uint16Type*)&(array.data[fixedheadSize + 0]);
	Uint16Type	*startCode	= (Uint16Type*)&(array.data[fixedheadSize + reserveSize + (segArrayElementSize * 1)]);
	Int16Type	*idDelta	= (Int16Type*) &(array.data[fixedheadSize + reserveSize + (segArrayElementSize * 2)]);
	Uint16Type	*idRangeOffset	= (Uint16Type*)&(array.data[fixedheadSize + reserveSize + (segArrayElementSize * 3)]);
	for(int seg = 0; seg < segCount; seg++){
		endCode[seg]		= htons(segmentBufs[seg].startCode		);
		startCode[seg]		= htons(segmentBufs[seg].endCode		);
		idDelta[seg]		= htons(segmentBufs[seg].idDelta		);
		idRangeOffset[seg]	= htons(segmentBufs[seg].idRangeOffset	);
	}

	//Uint16Type	*glyphIdArray;			// glyphIdArray[ ]

	return array;
}

typedef struct{
	GlyphDescriptionBuf	*glyphDescriptionBufs;
	size_t			numGlyphs;
	uint8_t			*cmapSubtableBuf_GlyphIdArray8;		//!< `glyphId = array[codepoint=0-255]`
	uint16_t		*cmapSubtableBuf_GlyphIdArray16;	//!< `glyphId = array[codepoint=0-65535]`
	FFByteArray		cmapByteArray;
	uint8_t			*locaData;
	size_t			locaDataSize;
	uint8_t			*glyfData;
	size_t			glyfDataSize;
}GlyphTablesBuf;

enum SimpleGlyphFlags_Bit{
	SimpleGlyphFlags_Bit1_X_SHORT_VECTOR		= (0x1 << 1),
	SimpleGlyphFlags_Bit2_Y_SHORT_VECTOR		= (0x1 << 2),
};

void GlyphTablesBuf_appendSimpleGlyph(
		GlyphTablesBuf *glyphTablesBuf,
		uint16_t codepoint,
		const GlyphDescriptionBuf *glyphDescriptionBuf)
{
	//DUMPUint16((uint16_t *)glyphDescriptionBuf->data, glyphDescriptionBuf->dataSize);

	// ** 'glyf' Table
	glyphTablesBuf->glyfData = (uint8_t *)ffrealloc(
			glyphTablesBuf->glyfData,
			glyphTablesBuf->glyfDataSize + glyphDescriptionBuf->dataSize);
	memcpy(&glyphTablesBuf->glyfData[glyphTablesBuf->glyfDataSize],
			glyphDescriptionBuf->data,
			glyphDescriptionBuf->dataSize);
	glyphTablesBuf->glyfDataSize += glyphDescriptionBuf->dataSize;

	// ** 'loca' Table
	// 'loca' Tableのoffsetsの型はHeadTable.indexToLocFormatにより指定。
	size_t newsize = sizeof(Offset32Type) * (glyphTablesBuf->numGlyphs + 2);
	glyphTablesBuf->locaData = realloc(glyphTablesBuf->locaData, newsize);
	ASSERT(glyphTablesBuf->locaData);
	// 先頭オフセット(初回先頭がゼロ。以降前回の末尾オフセットがあれば同じ値で上書きされる)
	Offset32Type *loca = (Offset32Type *)(glyphTablesBuf->locaData);
	loca[glyphTablesBuf->numGlyphs + 0] = htonl(glyphTablesBuf->glyfDataSize - glyphDescriptionBuf->dataSize);
	// 末尾オフセット
	loca[glyphTablesBuf->numGlyphs + 1] = htonl(glyphTablesBuf->glyfDataSize);
	glyphTablesBuf->locaDataSize = newsize;

	// ** 'cmap' Table
	ASSERT(glyphTablesBuf->cmapSubtableBuf_GlyphIdArray8);
	ASSERT(glyphTablesBuf->cmapSubtableBuf_GlyphIdArray16);
	if(codepoint <= CmapSubtableFormat0_CODEPOINT_MAX){
		if(0 != isprint((uint8_t)codepoint)){
			DEBUG_LOG("%3zu: 0x%02x`%c`", glyphTablesBuf->numGlyphs, codepoint, codepoint);
		}else{
			DEBUG_LOG("%3zu: 0x%02x`xx`", glyphTablesBuf->numGlyphs, codepoint);
		}

		glyphTablesBuf->cmapSubtableBuf_GlyphIdArray8[codepoint] = glyphTablesBuf->numGlyphs;
	}
	if(codepoint <= CmapSubtableFormat4_CODEPOINT_MAX){
		glyphTablesBuf->cmapSubtableBuf_GlyphIdArray16[codepoint] = glyphTablesBuf->numGlyphs;
	}

	// ** numGlyphs ('maxp' Table)
	(glyphTablesBuf->numGlyphs)++;
}

void GlyphTablesBuf_finally(GlyphTablesBuf *glyphTablesBuf)
{
	//! @note 2019/03/03現在CmapTable内部のSubtable順序等はFontForgeに生成させたフォントファイルを参考に合わせている

	// ** CmapTable.Header
	size_t numTables = 3;
	CmapTableHeader cmapTableHeader;
	CmapTableHeader_init(&cmapTableHeader, numTables);
	FFByteArray_append(&glyphTablesBuf->cmapByteArray,
			&cmapTableHeader, sizeof(CmapTableHeader));

	// ** // EncodingRecordElementHeader.offsetために事前に生成して長さを知る必要がある
	CmapTable_CmapSubtable_Format0 format0 = {0};
	memcpy(format0.glyphIdArray,
			glyphTablesBuf->cmapSubtableBuf_GlyphIdArray8, CmapSubtableFormat0_ARRAY_SIZE);
	CmapTable_CmapSubtable_Format0_finally(&format0, glyphTablesBuf->numGlyphs);
	FFByteArray arrayFormat4 = CmapTable_CmapSubtable_Format4_generateByteDataWithGlyphIdArray16(
			0,
			glyphTablesBuf->cmapSubtableBuf_GlyphIdArray16);

	// ** CmapTable.encodingRecordElementHeader
	size_t subtableOffset0
		= sizeof(CmapTableHeader) + (sizeof(CmapTable_EncodingRecordElementHeader) * 3);
	size_t subtableOffset1
		= sizeof(CmapTableHeader) + (sizeof(CmapTable_EncodingRecordElementHeader) * 3)
		+ arrayFormat4.length;
	CmapTable_EncodingRecordElementHeader encodingRecordElementHeader;
	DEBUG_LOG("subtableOffset: %zu(0x%08x) %zu(0x%08x)",
			subtableOffset0, (uint32_t)subtableOffset0, subtableOffset1, (uint32_t)subtableOffset1);

	// *** CmapTable.encodingRecordElementHeader[Format4 Unicode, ]
	encodingRecordElementHeader = CmapTable_EncodingRecordElementHeader_generate(0, 3, subtableOffset0);
	FFByteArray_append(&glyphTablesBuf->cmapByteArray,
			&encodingRecordElementHeader, sizeof(CmapTable_EncodingRecordElementHeader));
	// *** CmapTable.encodingRecordElementHeader[Format0 Macintosh, set 0]
	encodingRecordElementHeader = CmapTable_EncodingRecordElementHeader_generate(1, 0, subtableOffset1);
	FFByteArray_append(&glyphTablesBuf->cmapByteArray,
			&encodingRecordElementHeader, sizeof(CmapTable_EncodingRecordElementHeader));
	// *** CmapTable.encodingRecordElementHeader[Format4 Windows, Unicode BMP]
	encodingRecordElementHeader = CmapTable_EncodingRecordElementHeader_generate(3, 1, subtableOffset0);
	FFByteArray_append(&glyphTablesBuf->cmapByteArray,
			&encodingRecordElementHeader, sizeof(CmapTable_EncodingRecordElementHeader));

	// ** CmapTable.subtable[Format4]
	FFByteArray_appendArray(&glyphTablesBuf->cmapByteArray, arrayFormat4);
	DEBUG_LOG("format4: %zu(0x%08x)", arrayFormat4.length, (uint32_t)arrayFormat4.length);
	DUMP0(arrayFormat4.data, arrayFormat4.length);
	// ** CmapTable.subtable[Format0]
	FFByteArray_append(&glyphTablesBuf->cmapByteArray,
			&format0,
			sizeof(CmapTable_CmapSubtable_Format0));
}

void GlyphTablesBuf_init(GlyphTablesBuf *glyphTablesBuf)
{
	*glyphTablesBuf = (GlyphTablesBuf){
		.glyphDescriptionBufs	= NULL,
		.numGlyphs		= 0,
		.locaData		= NULL,
		.locaDataSize		= 0,
		.glyfData		= NULL,
		.glyfDataSize		= 0,
	};

	// ** 'cmap' Table バッファ確保
	glyphTablesBuf->cmapSubtableBuf_GlyphIdArray8
		= (uint8_t *)ffmalloc(sizeof(uint8_t) * CmapSubtableFormat0_ARRAY_SIZE);
	glyphTablesBuf->cmapSubtableBuf_GlyphIdArray16
		= (uint16_t *)ffmalloc(sizeof(uint16_t) * CmapSubtableFormat4_ARRAY_SIZE);

	// ** .notdefなどデフォルトの文字を追加
	//    & CmapTableテーブルにGlyphIdの初期値をセット
	//! @note Format0のBackspaceなどへのGlyphIdの割り当てはFontForgeの出力ファイルに倣った
	// .notdef
	GlyphDescriptionBuf glyphDescriptionBuf_notdef = {0};
	GlyphOutline outline_notdef = GlyphOutline_Notdef();
	GlyphDescriptionBuf_generateByteDataWithOutline(&glyphDescriptionBuf_notdef, &outline_notdef);
	GlyphTablesBuf_appendSimpleGlyph(glyphTablesBuf, 0x0, &glyphDescriptionBuf_notdef);

	GlyphDescriptionBuf glyphDescriptionBuf_empty = {0};
	GlyphOutline outline_empty = {0};
	GlyphDescriptionBuf_generateByteDataWithOutline(&glyphDescriptionBuf_empty, &outline_empty);
	// NUL and other
	GlyphTablesBuf_appendSimpleGlyph(glyphTablesBuf, 0, &glyphDescriptionBuf_empty);
	glyphTablesBuf->cmapSubtableBuf_GlyphIdArray8[ 8] = 1; // BackSpace = index 1
	glyphTablesBuf->cmapSubtableBuf_GlyphIdArray8[29] = 1; // GroupSeparator = index 1
	// TAB(HT) and other
	GlyphTablesBuf_appendSimpleGlyph(glyphTablesBuf, '\t', &glyphDescriptionBuf_empty);
	glyphTablesBuf->cmapSubtableBuf_GlyphIdArray8[13] = 1; // CR = index 2
}


typedef struct{
	Uint32Type	sfntVersion;
	Uint16Type	numTables;
	Uint16Type	searchRange;
	Uint16Type	entrySelector;
	Uint16Type	rangeShift;
}OffsetTable;

bool OffsetTable_init(OffsetTable *offsetTable_, Uint32Type sfntVersion, int numTables_)
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
		.searchRange		= htons(numTables_ * 16),
		.entrySelector		= htons((Uint16Type)log2(numTables_)),
		.rangeShift		= htons((numTables_ * 16) - offsetTable.searchRange),
	};

	*offsetTable_ = offsetTable;

	return true;
}

size_t TableSizeAlign(size_t size)
{
	int p = ((0 != (size % 4))? 1 : 0);
	size = ((size / 4) + p) * 4;
	return size;
}

Uint32Type CalcTableChecksum(Uint32Type *table, Uint32Type numberOfBytesInTable)
{
	Uint32Type sum = 0;
	Uint32Type nLongs = (numberOfBytesInTable + 3) / 4;
	while (nLongs-- > 0)
		sum += *table++;
	return sum;
}

//! @brief Table alignを考慮したchecksum計算関数
uint32_t calcChecksumWrapper(const uint8_t *data, size_t size)
{
	uint8_t *d = ffmalloc(TableSizeAlign(size));
	memcpy(d, data, size);
	return CalcTableChecksum((uint32_t *)d, size);
}

typedef struct{
	Uint32Type tag;			//!< table種別を表す識別子
	Uint32Type checkSum;		//!< テーブルのチェックサム
	Uint32Type offset;			//!< フォントファイル先頭からのオフセット
	Uint32Type length;			//!< テーブルの長さ
}TableDirectory_Member;

void TableDirectory_Member_init(TableDirectory_Member *self_, const char *tagstring, const uint8_t *tableData, size_t tableSize, uint32_t offset)
{
	ASSERT(self_);
	ASSERT(tagstring);
	ASSERT(tableData);
	ASSERT(0 < tableSize);

	uint32_t checksum = calcChecksumWrapper(tableData, tableSize);

	TableDirectory_Member self;
	ASSERT(TagType_init(&(self.tag), tagstring));
	self.checkSum	= htonl(checksum);
	self.offset	= htonl(offset);
	self.length	= htonl(tableSize);

	*self_ = self;

	return;
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
	tableBuf->tableDirectory = ffrealloc(tableBuf->tableDirectory, sizeof(TableDirectory_Member) * (tableBuf->appendTableNum + 1));
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
	tableBuf->data = ffrealloc(tableBuf->data, tableBuf->dataSize + alignedSize);
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

