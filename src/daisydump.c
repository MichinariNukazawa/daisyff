/**
  @file
  @author michianri.nukazawa@gmail.com / project daisy bell
  @details license: MIT
 */

#include "src/OpenType.h"
#include "include/version.h"
#include <inttypes.h>

enum{
	FFStrictMode_NONE = 0,
	FFStrictMode_ERROR,
	FFStrictMode_ALL,
};
typedef int FFStrictMode;

typedef struct{
	FFStrictMode		strictMode;
	char			tablename[5];
}FfDumpArg;
FfDumpArg arg = {0};

#define FONT_ERROR_LOG(fmt, ...) \
	do{ \
		fprintf(stderr, "font error: %s()[%d]: "fmt"\n", __func__, __LINE__, ## __VA_ARGS__); \
		if(FFStrictMode_ERROR <= arg.strictMode){ \
			fprintf(stderr, "exit from strictMode:%d", arg.strictMode); \
			exit(1); \
		} \
	}while(0);
#define FONT_WARN_LOG(fmt, ...) \
	do{ \
		fprintf(stderr, "font warning: %s()[%d]: "fmt"\n", __func__, __LINE__, ## __VA_ARGS__); \
	}while(0);
#define FONT_ASSERT(arg) \
	do{ \
		if(!(arg)){ \
			fprintf(stderr, "font assert: %s()[%d]:'%s'\n", __func__, __LINE__, #arg); \
			exit(1); \
		} \
	}while(0);

int CONVERT_INT_FROM_UINT16T(int16_t sv)
{
	if(0 <= sv){
		return (int)sv;
	}

	sv *= -1;
	int dv = (int)sv;
	return dv *= -1;
}

uint64_t ntohll(uint64_t x)
{
	if(IS_LITTLE_ENDIAN()){
		return bswap_64(x);
	}else{
		return x;
	}
}

void ntohArray16Move(uint8_t *buf8, const uint16_t *array16, size_t array16Num)
{
	// uint16_tの場合は逆変換するだけ
	htonArray16Move(buf8, array16, array16Num);
}

void ntohArray16(uint16_t *array16, size_t array16Num)
{
	uint8_t *buf8 = (uint8_t *)array16; // 自身へmove
	ntohArray16Move(buf8, array16, array16Num);
}

TagType TagType_Generate(const char *tagstring)
{
	ASSERT(tagstring);

	if(4 != strlen(tagstring)){
		ERROR_LOG("%zu `%s`", strlen(tagstring), tagstring);
		return false;
	}

	TagType tagvalue;
	memcpy((void *)&tagvalue, tagstring, 4);

	return tagvalue;
}

//!< @return malloc tag like string or 32bit hex dump.
const char *TagType_ToPrintString(uint32_t tagValue)
{
	char *tagstr = ffmalloc(strlen("0x12345678") + 1);

	bool isPrintable = true;
	for(int i = 0; i < 4; i++){
		char c = (uint8_t)(tagValue >> (8 * (3 - i)));
		if(0 == isprint(c)){
			isPrintable = false;
			break;
		}else{
			tagstr[i] = c;
		}
	}
	tagstr[4] = '\0';
	if(! isPrintable){
		sprintf(tagstr, "0x%08x", tagValue);
	}

	return tagstr;
}

//!< @todo ここは正直正しいのかどうかよくわからない(minorが複数桁の場合)
char *FixedType_ToPrintString(FixedType fixedvalue)
{
	char *fixedstring = ffmalloc(strlen("0000.0000") + 1);

	uint16_t major = (uint16_t)(fixedvalue >> 16);
	uint16_t minor = (uint16_t)(fixedvalue >>  0);
	sprintf(fixedstring, "%04x.%04x", major, minor);

	return fixedstring;
}

char *LONGDATETIMEType_ToPrintString(LONGDATETIMEType longdatetimevalue)
{
	time_t time = longdatetimevalue - LONGDATETIME_DELTA;

	struct tm tm;

	ASSERT(NULL != gmtime_r(&time, &tm));

	const int LEN = 256;
	char *tstring = ffmalloc(LEN);
	size_t size = strftime(tstring, LEN, "%Y-%m-%dT%H:%M:%S", &tm);
	ASSERT(size < LEN);

	return tstring;
}

const char *OffsetTable_SfntVersion_ToPrintString(uint32_t sfntVersion)
{
	return TagType_ToPrintString(sfntVersion);
}

TableDirectory_Member *TableDirectory_QueryTag(TableDirectory_Member *tableDirectory, size_t numTables, TagType tagvalue)
{
	for(int i = 0; i < numTables; i++){
		//DEBUG_LOG("%04x %04x", tableDirectory[i].tag, tagvalue);
		if(tableDirectory[i].tag == tagvalue){
			return &tableDirectory[i];
		}
	}

	return NULL;
}

HeadTable HeadTable_ToHostByteOrder(HeadTable headTable)
{
	HeadTable headTable_Host = {
		.majorVersion		= ntohs(headTable.majorVersion		),
		.minorVersion		= ntohs(headTable.minorVersion		),
		.fontRevision		= ntohl(headTable.fontRevision		),
		.checkSumAdjustment	= ntohl(headTable.checkSumAdjustment	),
		.magicNumber		= ntohl(headTable.magicNumber		),
		.flags			= ntohs(headTable.flags			),
		.unitsPerEm		= ntohs(headTable.unitsPerEm		),
		.created		= ntohll(headTable.created		),
		.modified		= ntohll(headTable.modified		),
		.xMin			= ntohs(headTable.xMin			),
		.yMin			= ntohs(headTable.yMin			),
		.xMax			= ntohs(headTable.xMax			),
		.yMax			= ntohs(headTable.yMax			),
		.macStyle		= ntohs(headTable.macStyle		),
		.lowestRecPPEM		= ntohs(headTable.lowestRecPPEM		),
		.fontDirectionHint	= ntohs(headTable.fontDirectionHint	),
		.indexToLocFormat	= ntohs(headTable.indexToLocFormat	),
		.glyphDataFormat	= ntohs(headTable.glyphDataFormat	),
	};

	return headTable_Host;
}

typedef MaxpTable_Version05 MaxpTable;

MaxpTable MaxpTable_ToHostByteOrder(MaxpTable maxpTable)
{
	MaxpTable maxpTable_Host = {
		.version		= ntohl(maxpTable.version	),
		.numGlyphs		= ntohs(maxpTable.numGlyphs	),
	};
	if(0x00005000 != maxpTable_Host.version){
		WARN_LOG("not implement"); //!< @todo not implement
	}

	return maxpTable_Host;
}

typedef struct{
	uint16_t	id;
	const char	*showString;
}PlatformIdInfo;
const PlatformIdInfo platformIdInfos[] = {
	{0,	"Unicode",},
	{1,	"Macintosh",},
	{2,	"ISO(deprecated)",},
	{3,	"Windows",},
	{4,	"Custom",},
};

const PlatformIdInfo *PlatformID_InfoFromId(uint16_t platformID)
{
	for(int i = 0; i < sizeof(platformIdInfos)/ sizeof(platformIdInfos[0]); i++){
		if(platformIdInfos[i].id == platformID){
			return &platformIdInfos[i];
		}
	}

	return NULL;
}
const char *PlatformID_ToShowString(uint16_t platformID)
{
	const PlatformIdInfo *platformInfo = PlatformID_InfoFromId(platformID);
	if(NULL == platformInfo){
		return "<unknown>";
	}

	return platformInfo->showString;
}

typedef struct{
	uint16_t	encodingId;
	const char	*showString;
}EncodingIdInWindowsPlatformInfo;
EncodingIdInWindowsPlatformInfo encodingIdInWindowsPlatformInfos[] = {
	{ 0,	"Symbol",},
	{ 1,	"Unicode BMP",},
	{ 2,	"ShiftJIS",},
	{ 3,	"PRC",},
	{ 4,	"Big5",},
	{ 5,	"Wansung",},
	{ 6,	"Johab",},
	{ 7,	"Reserved",},
	{ 8,	"Reserved",},
	{ 9,	"Reserved",},
	{10,	"Unicode full repertoire",},
};

const EncodingIdInWindowsPlatformInfo *EncodingIdInWindowsPlatform_InfoFromFormat(uint16_t encodingId)
{
	for(int i = 0; i < sizeof(encodingIdInWindowsPlatformInfos)/ sizeof(encodingIdInWindowsPlatformInfos[0]); i++){
		if(encodingIdInWindowsPlatformInfos[i].encodingId == encodingId){
			return &encodingIdInWindowsPlatformInfos[i];
		}
	}

	return NULL;
}
const char *EncodingIdInWindowsPlatform_ToShowString(uint16_t encodingId)
{
	const EncodingIdInWindowsPlatformInfo *encodingIdInWindowsPlatformInfo = EncodingIdInWindowsPlatform_InfoFromFormat(encodingId);
	if(NULL == encodingIdInWindowsPlatformInfo){
		return "<unknown>";
	}

	return encodingIdInWindowsPlatformInfo->showString;
}

const char *EncodingID_ToShowString(uint16_t platformId, uint16_t encodingId)
{
	//DEBUG_LOG("%u %u", platformId, encodingId);
	switch(platformId){
		case 0: // Unicode
			return "<daisyff not implement>"; //!< @todo
		case 1: // Macintosh
			return "set encodingId=0";
		case 3: // windows
			return EncodingIdInWindowsPlatform_ToShowString(encodingId);
		default:
			return "<unknown>";
	}
}

typedef struct{
	uint16_t	format;
	const char	*showString;
}CmapSubtableInfo;
const CmapSubtableInfo cmapSubtableInfos[] = {
	{ 0, "Byte encoding table"},
	{ 2, "High-byte mapping through table"},
	{ 4, "Segment mapping to delta values"},
	{ 6, "Trimmed table mapping"},
	{ 8, "mixed 16-bit and 32-bit coverage"},
	{10, "Trimmed array"},
	{12, "Segmented coverage"},
	{13, "Many-to-one range mappings"},
	{14, "Unicode Variation Sequences"},
};

const CmapSubtableInfo *CmapSubtable_InfoFromFormat(uint16_t format)
{
	for(int i = 0; i < sizeof(cmapSubtableInfos)/ sizeof(cmapSubtableInfos[0]); i++){
		if(cmapSubtableInfos[i].format == format){
			return &cmapSubtableInfos[i];
		}
	}

	return NULL;
}
const char *CmapSubtable_ToShowString(uint16_t format)
{
	const CmapSubtableInfo *cmapSubtableInfo = CmapSubtable_InfoFromFormat(format);
	if(NULL == cmapSubtableInfo){
		return "<unknown>";
	}

	return cmapSubtableInfo->showString;
}


enum LocaTable_Kind{
	LocaTable_Kind_Short	= 0,
	LocaTable_Kind_Long	= 1,
};
typedef int LocaTable_Kind;

char *GlyphDescriptionFlag_ToPrintString(uint8_t flag)
{
	char *str = ffmalloc(512);
	snprintf(str, 512,
		"%-7s %-7s %-7s %-7s %-7s %-7s %-9s",
		(0 != (flag & (1 << 6))) ? "Overlap":"",
		(0 != (flag & (1 << 5))) ? "YDual":"",
		(0 != (flag & (1 << 4))) ? "XDual":"",
		(0 != (flag & (1 << 3))) ? "Repeat":"",
		(0 != (flag & (1 << 2))) ? "Y-Short":"",
		(0 != (flag & (1 << 1))) ? "X-Short":"",
		(0 != (flag & (1 << 0))) ? "OnCurve":"OffCurve" // ttfdumpのOn/Offはこれの模様。
		);

	return str;
}

typedef struct{
	uint16_t x;
	uint16_t y;
}RawPoint;

typedef struct{
	int x;
	int y;
}Point;

typedef struct{
	int		isFlagRepeated;
	uint8_t		flag;
	int		isRelXSame;
	int		isRelYSame;
	RawPoint	raw;
	Point		rel;
	Point		abs;
}GlyphDescriptionPoint;

bool GlyphFlag_IsXShortVector(uint8_t flag)
{
	return (0 != (flag & (0x1 << 1)));
}

bool GlyphFlag_IsSameOrPisitiveXShortVector(uint8_t flag)
{
	return (0 != (flag & (0x1 << 4)));
}

bool GlyphFlag_IsYShortVector(uint8_t flag)
{
	return (0 != (flag & (0x1 << 2)));
}

bool GlyphFlag_IsSameOrPisitiveYShortVector(uint8_t flag)
{
	return (0 != (flag & (0x1 << 5)));
}

#define COPYRANGE_OR_DIE(ARG_fd, ARG_buf, ARG_offset, ARG_size) \
	do{ \
		int fd_ = (ARG_fd); void *buf_ = (ARG_buf); size_t offset_ = (ARG_offset); size_t size_ = (ARG_size); \
		if(! copyrange(fd_, buf_, offset_, size_)){ \
			FONT_ERROR_LOG("COPYRANGE_OR_DIE: %d %s %d %p %zu %zu", errno, strerror(errno), fd_, buf_, offset_, size_); \
			exit(1); \
		} \
	}while(0);

bool copyrange(int fd, uint8_t *buf, size_t offset, size_t size)
{
	errno = 0;
	off_t off = lseek(fd, offset, SEEK_SET);
	if(-1 == off){
		ERROR_LOG("lseek: %ld %d %s", off, errno, strerror(errno));
		return false;
	}

	ssize_t ssize;
	ssize = read(fd, buf, size);
	if(ssize != size){
		ERROR_LOG("read: %zd %d %s\n", ssize, errno, strerror(errno));
		return false;
	}

	return true;
}

/*
#define DEBUG_RANGE(ARG_fd, ARG_offset, ARG_size) \
	do{ \
		int fd_ = (ARG_fd); size_t offset_ = (ARG_offset); size_t size_ = (ARG_size); \
		void *buf_ = ffmalloc(size_); \
		COPYRANGE_OR_DIE(fd_, buf_, offset_, size_); \
	}while(0);
*/
void readTableDirectory(
		TableDirectory_Member **pTableDirectory,
		size_t numTables,
		int fd)
{
	TableDirectory_Member *tableDirectory = NULL;
	// ** TableDirectory
	//TableDirectory_Member *tableDirectory = NULL;
	for(int i = 0; i < numTables; i++){
		TableDirectory_Member tableDirectory_Member;
		ssize_t ssize;
		ssize = read(fd, (void *)&tableDirectory_Member, sizeof(tableDirectory_Member));
		if(ssize != sizeof(tableDirectory_Member)){
			FONT_ERROR_LOG("read: %zd %d %s", ssize, errno, strerror(errno));
			exit(1);
		}

		tableDirectory = ffrealloc(tableDirectory, sizeof(TableDirectory_Member) * (i + 1));
		ASSERT(tableDirectory);
		memcpy(&tableDirectory[i], &tableDirectory_Member, sizeof(TableDirectory_Member));

		{
			TableDirectory_Member tableDirectory_Member_Host;
			tableDirectory_Member_Host.tag		= ntohl(tableDirectory_Member.tag);
			tableDirectory_Member_Host.checkSum	= ntohl(tableDirectory_Member.checkSum);
			tableDirectory_Member_Host.offset	= ntohl(tableDirectory_Member.offset);
			tableDirectory_Member_Host.length	= ntohl(tableDirectory_Member.length);
			const char *tagstring = TagType_ToPrintString(tableDirectory_Member_Host.tag);
			fprintf(stdout,
					"%2d. '%s' - checksum = 0x%08x, offset = 0x%08x(%6d), len =%8d\n",
					i,
					tagstring,
					tableDirectory_Member_Host.checkSum,
					tableDirectory_Member_Host.offset,
					tableDirectory_Member_Host.offset,
					tableDirectory_Member_Host.length);
		}
	}

	*pTableDirectory = tableDirectory;
}

void headTable(
		TableDirectory_Member *tableDirectory,
		size_t numTables,
		int fd,
		uint16_t *headTable_Host_indexToLocFormat)
{
	// ** HeadTable
	TableDirectory_Member *tableDirectory_HeadTable = TableDirectory_QueryTag(tableDirectory, numTables, TagType_Generate("head"));
	if(NULL == tableDirectory_HeadTable){
		FONT_WARN_LOG("HeadTable not detected.");
		return;
	}

	HeadTable headTable;

	COPYRANGE_OR_DIE(fd, (void *)&headTable, ntohl(tableDirectory_HeadTable->offset), sizeof(headTable));

	HeadTable headTable_Host = HeadTable_ToHostByteOrder(headTable);
	const char *macstyleprintstring = MacStyle_toStringForNameTable(headTable_Host.macStyle);
	macstyleprintstring = ((NULL != macstyleprintstring) ?  macstyleprintstring : "unknown");
	fprintf(stdout, "\n");
	fprintf(stdout,
		"'head' Table - Font Header\n"
		"--------------------------\n"
		"	 'head' version:	 %4u.%u(set to 1.0)\n"		// 1.0
		"	 fontReversion:		 %s\n"				// 0.6400
		"	 checkSumAdjustment:	 0x%08x\n"			// 0x6a403d51
		"	 magicNumber:		 0x%08x(set to 0x5f0f3cf5)\n"	// 0x5f0f3cf5
		"	 flags:			 0x%04x\n"			// 0x000b
		"	 unitsPerEm:		 %4u\n"				// 1000
		"	 created:		 0x%016"PRIx64"(%s)\n"		// 0x00000000d88068a7
		"	 modified:		 0x%016"PRIx64"(%s)\n"		// 0x00000000d8806be2
		"	 xMin:			 %4u\n"				// 33
		"	 yMin:			 %4u\n"				// 0
		"	 xMax:			 %4u\n"				// 727
		"	 yMax:			 %4u\n"				// 666
		"	 macStyle bits:		 0x%04x(%s)\n"			// 0x0000
		"	 lowestRecPPEM:		 %4u\n"				// 8
		"	 fontDirectionHint:	 %4d(set to 2)\n"		// 2
		"	 indexToLocFormat:	 %4d(%s)\n"			// 0
		"	 glyphDataFormat:	 %4d(set to 0)\n"		// 0
		,
		headTable_Host.majorVersion		,
		headTable_Host.minorVersion		,
		FixedType_ToPrintString(headTable_Host.fontRevision),
		headTable_Host.checkSumAdjustment	,
		headTable_Host.magicNumber		,
		headTable_Host.flags			,
		headTable_Host.unitsPerEm		,
		headTable_Host.created		,
		LONGDATETIMEType_ToPrintString(headTable_Host.created)		,
		headTable_Host.modified		,
		LONGDATETIMEType_ToPrintString(headTable_Host.modified)		,
		headTable_Host.xMin			,
		headTable_Host.yMin			,
		headTable_Host.xMax			,
		headTable_Host.yMax			,
		headTable_Host.macStyle		,
		macstyleprintstring,
		headTable_Host.lowestRecPPEM		,
		headTable_Host.fontDirectionHint	,
		headTable_Host.indexToLocFormat	,
		((0 == headTable_Host.indexToLocFormat) ? "short":"long"),
		headTable_Host.glyphDataFormat	);

	*headTable_Host_indexToLocFormat = headTable_Host.indexToLocFormat;
}

void maxpTable(
		TableDirectory_Member *tableDirectory,
		size_t numTables,
		int fd,
		size_t *maxpTable_Host_numGlyphs)
{

	// ** MaxpTable
	TableDirectory_Member *tableDirectory_MaxpTable = TableDirectory_QueryTag(tableDirectory, numTables, TagType_Generate("maxp"));
	if(NULL == tableDirectory_MaxpTable){
		FONT_WARN_LOG("MaxpTable not detected.");
		return;
	}

	MaxpTable_Version05 maxpTable;

	COPYRANGE_OR_DIE(fd, (void *)&maxpTable, ntohl(tableDirectory_MaxpTable->offset), sizeof(maxpTable));

	// MaxpTable Version 0.5
	MaxpTable_Version05 maxpTable_Host = MaxpTable_ToHostByteOrder(maxpTable);
	if(0x00005000 != maxpTable_Host.version){
		FONT_WARN_LOG("not implement or invalid version 0x%08x", maxpTable_Host.version);
	}
	fprintf(stdout, "\n");
	fprintf(stdout,
		"'maxp' Table - Maximum Profile\n"
		"------------------------------\n"
		"	 'maxp' version:	 %s\n"
		"	 numGlyphs:		 %d\n"
		,
		FixedType_ToPrintString(maxpTable_Host.version),
		maxpTable_Host.numGlyphs);

	*maxpTable_Host_numGlyphs = maxpTable_Host.numGlyphs;
}

void cmapTable_Format0(TableDirectory_Member *tableDirectory_CmapTable, int fd, size_t subtableOffset)
{
	size_t HEADER_SIZE = 6;
	uint16_t header[3];
	COPYRANGE_OR_DIE(fd, (void *)header, ntohl(tableDirectory_CmapTable->offset) + subtableOffset, sizeof(header));
	uint16_t length = htons(header[1]);
	uint16_t languageId = htons(header[2]);
	fprintf(stdout,
		"		 Length:     %3d(limited to header(6) + 256)\n"
		"		 Language:   %3d\n",
		length,
		languageId);

	FONT_ASSERT(6 + 256 >= length); // Format0 fullsize
	FONT_ASSERT(6 <= length); // Format0 fixed length head size

	uint8_t glyphIdArray[256];
	COPYRANGE_OR_DIE(fd, (void *)glyphIdArray, ntohl(tableDirectory_CmapTable->offset) + subtableOffset + HEADER_SIZE, length - HEADER_SIZE);
	fprintf(stdout,
		"		[%3d] = {\n"
		"		",
		(int)(length - HEADER_SIZE));
	for(int g = 0; g < (length - HEADER_SIZE); g++){
		if((0 != g) && (0 == g % 16)){
			fprintf(stdout, "\n		");
		}
		fprintf(stdout, "%3d,", glyphIdArray[g]);
	}
	fprintf(stdout, "}\n");
}

void cmapTable_Format4(TableDirectory_Member *tableDirectory_CmapTable, int fd, size_t subtableOffset)
{
	// ** CmapSubtableFormat4 FixedLengthHead
	size_t FIXED_LENGTH_HEAD_SIZE = sizeof(Uint16Type) * 7;
	uint16_t header[3];
	COPYRANGE_OR_DIE(fd, (void *)header, ntohl(tableDirectory_CmapTable->offset) + subtableOffset, sizeof(header));
	uint16_t length = htons(header[1]);
	uint16_t languageId = htons(header[2]);
	fprintf(stdout,
		"		 Length:     %3d(limited to header(6) + 256)\n"
		"		 Language:   %3d\n",
		length,
		languageId);

	FONT_ASSERT(FIXED_LENGTH_HEAD_SIZE <= length); // Format4 fixed length head size

	// ** read fixed length elements of head
	// segment listの最適化された検索パラメタ 先頭固定長領域を取ってくる
	CmapTable_CmapSubtable_Format4Buf format4buf_Host = {0};
	COPYRANGE_OR_DIE(fd, (void *)&format4buf_Host, ntohl(tableDirectory_CmapTable->offset) + subtableOffset, FIXED_LENGTH_HEAD_SIZE);
	ntohArray16((void *)&format4buf_Host, FIXED_LENGTH_HEAD_SIZE);

	uint16_t segCount	= format4buf_Host.segCountX2 / 2;
	uint16_t searchRange	= (2 * 2 * (int)floor(log2(segCount)));
	uint16_t entrySelector	= ((int)log2(searchRange/2.0));
	uint16_t rangeShift	= 2 * segCount - searchRange;

	fprintf(stdout,
		//"		 format		%4u\n"
		//"		 length		%4u\n"
		//"		 language	%4u\n"
		"		 segCountX2:	%4u (%4u: segCount)\n"
		"		 searchRange:	%4u (%4u = 2x(2**floor(log2(segCount))))\n"
		"		 entrySelector:	%4u (%4u = log2(searchRange/2))\n"
		"		 rangeShift:	%4u (%4u = 2xsegCount - searchRange)\n",
		//format4buf_Host.format,
		//format4buf_Host.length,
		//format4buf_Host.language,
		format4buf_Host.segCountX2,
		segCount,
		format4buf_Host.searchRange,
		searchRange,
		format4buf_Host.entrySelector,
		entrySelector,
		format4buf_Host.rangeShift,
		rangeShift);

	// ** read variable length elements (segCount)
	size_t offsetInSubtable = subtableOffset;
	offsetInSubtable += FIXED_LENGTH_HEAD_SIZE;

	// *** メモリ確保
	format4buf_Host.endCode		= ffmalloc(sizeof(Uint16Type) * segCount);
	format4buf_Host.startCode	= ffmalloc(sizeof(Uint16Type) * segCount);
	format4buf_Host.idDelta		= ffmalloc(sizeof(Uint16Type) * segCount);
	format4buf_Host.idRangeOffset	= ffmalloc(sizeof(Uint16Type) * segCount);
	//format4buf_Host.glyphIdArray	= ffmalloc(sizeof(Uint16Type) * segCount);

	// *** endCode
	COPYRANGE_OR_DIE(fd, (void *)format4buf_Host.endCode,
			ntohl(tableDirectory_CmapTable->offset) + offsetInSubtable,
			sizeof(Uint16Type) * segCount);
	ntohArray16((void *)format4buf_Host.endCode, sizeof(Uint16Type) * segCount);
	offsetInSubtable += sizeof(Uint16Type) * segCount;
	// *** reservedPad
	offsetInSubtable += sizeof(Uint16Type);
	// *** startCode
	COPYRANGE_OR_DIE(fd, (void *)format4buf_Host.startCode,
			ntohl(tableDirectory_CmapTable->offset) + offsetInSubtable,
			sizeof(Uint16Type) * segCount);
	ntohArray16((void *)format4buf_Host.startCode, sizeof(Uint16Type) * segCount);
	offsetInSubtable += sizeof(Uint16Type) * segCount;
	// *** idDelta
	COPYRANGE_OR_DIE(fd, (void *)format4buf_Host.idDelta,
			ntohl(tableDirectory_CmapTable->offset) + offsetInSubtable,
			sizeof(Uint16Type) * segCount);
	ntohArray16((void *)format4buf_Host.idDelta, sizeof(Uint16Type) * segCount);
	offsetInSubtable += sizeof(Uint16Type) * segCount;
	// *** idRangeOffset
	COPYRANGE_OR_DIE(fd, (void *)format4buf_Host.idRangeOffset,
			ntohl(tableDirectory_CmapTable->offset) + offsetInSubtable,
			sizeof(Uint16Type) * segCount);
	ntohArray16((void *)format4buf_Host.idRangeOffset, sizeof(Uint16Type) * segCount);
	offsetInSubtable += sizeof(Uint16Type) * segCount;

	// ** segments summary
	for(int seg = 0; seg < segCount; seg++){
		fprintf(stdout,
			"		 Seg %2d/%2u: startCode=0x%04x,end=0x%04x,delta=%5d,rangeOffset=%5u\n",
			seg,
			segCount,
			format4buf_Host.startCode[seg],
			format4buf_Host.endCode[seg],
			CONVERT_INT_FROM_UINT16T(format4buf_Host.idDelta[seg]),
			format4buf_Host.idRangeOffset[seg]);
	}

	// ** segment
	for(int seg = 0; seg < segCount; seg++){
		fprintf(stdout,
			" Segment %2d/%2u (offset:%5u):\n",
			seg,
			segCount,
			format4buf_Host.idRangeOffset[seg]);

		if(! (format4buf_Host.startCode[seg] <= format4buf_Host.endCode[seg])){
			FONT_ERROR_LOG("! %u < %u", format4buf_Host.startCode[seg], format4buf_Host.endCode[seg]);
			continue;
		}

		size_t glyphIdArrayNum = (format4buf_Host.endCode[seg] - format4buf_Host.startCode[seg]) + 1;

		if(0xffff == format4buf_Host.endCode[seg]){ // last segment to missignGlyph
			if(0xffff != format4buf_Host.startCode[seg] ||
				1 != format4buf_Host.idDelta[seg] ||
				0 != format4buf_Host.idRangeOffset[seg]){
				FONT_ERROR_LOG("0x%04x 0x%04x %d %u",
					format4buf_Host.endCode[seg],
					format4buf_Host.startCode[seg],
					format4buf_Host.idDelta[seg],
					format4buf_Host.idRangeOffset[seg]);
				continue;
			}
		}

		if(0 != format4buf_Host.idRangeOffset[seg]){
			fprintf(stdout,
				"		daisyff not implement. 0 != idRangeOffset");
			continue;
		}

		for(int i = 0; i < glyphIdArrayNum; i++){
			uint16_t glyphId = format4buf_Host.startCode[seg] + i + format4buf_Host.idDelta[seg];
			fprintf(stdout,
				"		 Char 0x%04x -> Index %3d\n",
				format4buf_Host.startCode[seg] + i,
				glyphId);
		}
	}
}

void cmapTable(TableDirectory_Member *tableDirectory, size_t numTables, int fd)
{
	// ** CmapTable
	TableDirectory_Member *tableDirectory_CmapTable = TableDirectory_QueryTag(tableDirectory, numTables, TagType_Generate("cmap"));
	if(NULL == tableDirectory_CmapTable){
		FONT_WARN_LOG("CmapTable not detected.");
		return;
	}

	fprintf(stdout, "\n");
	fprintf(stdout,
		"'cmap' Table - Character to Glyph Index Mapping Table\n"
		"-----------------------------------------------------\n");

	CmapTableHeader cmapTableHeader;
	COPYRANGE_OR_DIE(fd, (void *)&cmapTableHeader, ntohl(tableDirectory_CmapTable->offset), sizeof(CmapTableHeader));

	// *** CmapTableHeader
	CmapTableHeader cmapTableHeader_Host = {
		.version	= ntohs(cmapTableHeader.version		),
		.numTables	= ntohs(cmapTableHeader.numTables	),
	};
	fprintf(stdout,
		"	 'cmap' version: %d\n"
		//"	 number of encodings: 1\n"
		"	 number of subtables: %2d\n"
		,
		cmapTableHeader_Host.version,
		cmapTableHeader_Host.numTables);

	size_t offsetInTable = sizeof(CmapTableHeader);

	fprintf(stdout, "\n");

	// *** CmapTable EncogindRecords
	uint32_t cmapSubtableOffsets[cmapTableHeader_Host.numTables]; // CmapSubtableを引くのに使う
	for(int r = 0; r < cmapTableHeader_Host.numTables; r++){
		CmapTable_EncodingRecordElementHeader encodingRecord;
		COPYRANGE_OR_DIE(fd, (void *)&encodingRecord, ntohl(tableDirectory_CmapTable->offset) + offsetInTable, sizeof(CmapTable_EncodingRecordElementHeader));
		CmapTable_EncodingRecordElementHeader encodingRecord_Host = {
			.platformID	= ntohs(encodingRecord.platformID	),
			.encodingID	= ntohs(encodingRecord.encodingID	),
			.offset		= ntohl(encodingRecord.offset		),
		};
		fprintf(stdout,
			"Encoding   %d.	 PlatformID:  %d(%s)\n"
			"		 EcodingID:   %d(%s)\n"
			"		 SubTable: %d, Offset: 0x%08x(%4d)\n",
			r,
			encodingRecord_Host.platformID,
			PlatformID_ToShowString(encodingRecord_Host.platformID),
			encodingRecord_Host.encodingID,
			EncodingID_ToShowString(encodingRecord_Host.platformID, encodingRecord_Host.encodingID),
			r,
			encodingRecord_Host.offset,
			encodingRecord_Host.offset);
		offsetInTable += sizeof(CmapTable_EncodingRecordElementHeader);
		cmapSubtableOffsets[r] = encodingRecord_Host.offset;
	}

	// *** CmapTable Subtable
	for(int t = 0; t < cmapTableHeader_Host.numTables; t++){
		uint16_t format;
		COPYRANGE_OR_DIE(fd, (void *)&format, ntohl(tableDirectory_CmapTable->offset) + cmapSubtableOffsets[t], sizeof(uint16_t));
		format = ntohs(format);

		fprintf(stdout, "\n");
		fprintf(stdout,
			"SubTable   %d.	 Format %d - %s (offset:0x%08x)\n",
			t,
			format,
			CmapSubtable_ToShowString(format),
			(uint32_t)cmapSubtableOffsets[t]); // あまり大きいoffsetは想定していない

		int alreadyIndex = -1;
		for(int ii = 0; ii < t; ii++){
			if(cmapSubtableOffsets[t] == cmapSubtableOffsets[ii]){
				alreadyIndex = ii;
			}
		}
		if(-1 != alreadyIndex){
			fprintf(stdout,
				"	skip CmapSubtable already %2d,%2d/%2d(offset:0x%08x)\n",
				alreadyIndex, t, cmapTableHeader_Host.numTables, cmapSubtableOffsets[t]);
			continue;
		}

		switch(format){
			case 0:
			{
				cmapTable_Format0(tableDirectory_CmapTable, fd, cmapSubtableOffsets[t]);
			}
				break;
			case 4:
			{
				cmapTable_Format4(tableDirectory_CmapTable, fd, cmapSubtableOffsets[t]);
			}
				break;
			default:
				fprintf(stdout,
					"	CmapSubtable(%2d, 0x%08x) not implement or invalid.\n",
					format, format
					);
		}
	}
}

void locaTable(
		TableDirectory_Member *tableDirectory,
		size_t numTables,
		int fd,
		uint16_t headTable_Host_indexToLocFormat,
		size_t maxpTable_Host_numGlyphs,
		uint32_t **pLocaList)
{
	uint32_t *locaList = NULL;

	// ** LocaTable
	LocaTable_Kind locaTable_Kind = ((0 == headTable_Host_indexToLocFormat) ? LocaTable_Kind_Short : LocaTable_Kind_Long);
	TableDirectory_Member *tableDirectory_LocaTable = TableDirectory_QueryTag(tableDirectory, numTables, TagType_Generate("loca"));
	if(NULL == tableDirectory_LocaTable){
		FONT_WARN_LOG("LocaTable not detected.");
		return;
	}

	size_t locaOffsetSize = (locaTable_Kind == LocaTable_Kind_Short) ? sizeof(uint16_t):sizeof(uint32_t);
	size_t tableSize = locaOffsetSize * (maxpTable_Host_numGlyphs + 1);
	uint8_t locaTable[tableSize];

	COPYRANGE_OR_DIE(fd, (void *)&locaTable, ntohl(tableDirectory_LocaTable->offset), sizeof(locaTable));

	// LocaTable short,long
	fprintf(stdout, "\n");
	fprintf(stdout,
		"'loca' Table - Index to Location\n"
		"--------------------------------\n");
	for(int i = 0; i < (maxpTable_Host_numGlyphs + 1); i++){
		uint32_t sv = 0;
		uint32_t dv = 0;
		if(locaTable_Kind == LocaTable_Kind_Short){
			uint16_t shortv;
			memcpy(&shortv, &locaTable[i * sizeof(uint16_t)], sizeof(uint16_t));
			shortv = ntohs(shortv);
			sv = shortv;
			dv = shortv * 2;
		}else{
			uint32_t longv;
			memcpy(&longv, &locaTable[i * sizeof(uint32_t)], sizeof(uint32_t));
			longv = ntohl(longv);
			sv = longv;
			dv = longv;
		}

		locaList = (uint32_t *)ffrealloc(locaList, sizeof(uint32_t) * (i + 1));
		ASSERT(locaList);
		locaList[i] = dv;

		if(i != maxpTable_Host_numGlyphs){
			fprintf(stdout, "	 Idx %6d -> GlyphOffset 0x%08x(0x%08x %6u)\n", i, dv, sv, dv);
		}else{
			fprintf(stdout, "	                  Ended at 0x%08x(0x%08x %6u)\n", dv, sv, dv);
		}
	}

	*pLocaList = locaList;
}

void glyfTable(
		TableDirectory_Member *tableDirectory,
		size_t numTables,
		int fd,
		size_t maxpTable_Host_numGlyphs,
		uint32_t *locaList)
{
	// ** GlyfTable
	TableDirectory_Member *tableDirectory_GlyfTable = TableDirectory_QueryTag(tableDirectory, numTables, TagType_Generate("glyf"));
	if(NULL == tableDirectory_GlyfTable){
		FONT_WARN_LOG("GlyfTable not detected.");
		return;
	}

	fprintf(stdout, "\n");
	fprintf(stdout,
		"'glyf' Table - Glyph Data\n"
		"-------------------------\n");

	FONT_ASSERT(NULL != locaList);

	for(int glyphId = 0; glyphId< maxpTable_Host_numGlyphs; glyphId++){
		size_t offsetOnTable = locaList[glyphId];
		size_t datasize = locaList[glyphId + 1] - locaList[glyphId];

		// *** GlyphDescription.Header
		GlyphDescriptionHeader glyphDescriptionHeader;
		COPYRANGE_OR_DIE(fd, (void *)&glyphDescriptionHeader, ntohl(tableDirectory_GlyfTable->offset) + offsetOnTable, sizeof(GlyphDescriptionHeader));

		//DUMPUint16((uint16_t *)&glyphDescriptionHeader, sizeof(GlyphDescriptionHeader));

		GlyphDescriptionHeader glyphDescriptionHeader_Host = {
			.numberOfContours	= ntohs(glyphDescriptionHeader.numberOfContours		),
			.xMin			= ntohs(glyphDescriptionHeader.xMin			),
			.yMin			= ntohs(glyphDescriptionHeader.yMin			),
			.xMax			= ntohs(glyphDescriptionHeader.xMax			),
			.yMax			= ntohs(glyphDescriptionHeader.yMax			),
		};
		fprintf(stdout, "\n");
		fprintf(stdout,
			"Glyph %6d.\n"
			"	 numberOfContours:	 %4d\n"
			"	 xMin:			 %4d\n"
			"	 yMin:			 %4d\n"
			"	 xMax:			 %4d\n"
			"	 yMax:			 %4d\n"
			,
			glyphId,
			glyphDescriptionHeader_Host.numberOfContours	,
			glyphDescriptionHeader_Host.xMin		,
			glyphDescriptionHeader_Host.yMin		,
			glyphDescriptionHeader_Host.xMax		,
			glyphDescriptionHeader_Host.yMax		);

		if(0 == datasize){
			fprintf(stdout, "	 skip datasize is zero.\n");
			continue;
		}

		if(0 > glyphDescriptionHeader_Host.numberOfContours){
			fprintf(stdout, "	 skip CompositeGlyphDescription not implement.\n"); //!< @todo not implement.
			continue;
		}

		//! @todo check GlyphDescription elemetns on memory data range.

		uint8_t *gdata = ffmalloc(datasize);
		COPYRANGE_OR_DIE(fd, gdata, ntohl(tableDirectory_GlyfTable->offset) + offsetOnTable, datasize);

		//DUMPUint16((uint16_t *)gdata, sizeof(GlyphDescriptionHeader));
		//DUMPUint16Ntohs((uint16_t *)gdata, datasize / 2);
		//DUMP0(gdata, sizeof(GlyphDescriptionHeader));
		//DUMP0(&gdata[sizeof(GlyphDescriptionHeader)], 16);

		size_t pointNum = 0; //! @todo use flags from EndPoints?

		size_t offsetInTable = 0;

		// *** GlyphDescription.EndPoints
		offsetInTable += sizeof(GlyphDescriptionHeader);
		fprintf(stdout, "\n");
		fprintf(stdout,
			"	 EndPoints (%d)\n"
			"	 ---------\n",
			glyphDescriptionHeader_Host.numberOfContours
			);
		for(int co = 0; co < glyphDescriptionHeader_Host.numberOfContours; co++){
			ASSERTF(offsetInTable < ntohl(tableDirectory_GlyfTable->length),
					"%d", ntohl(tableDirectory_GlyfTable->length));

			uint16_t *p = (uint16_t *)&gdata[offsetInTable];
			uint16_t endPtsOfContour = ntohs(*p);
			fprintf(stdout, "	 %2d: %2d\n", co, endPtsOfContour);

			pointNum = endPtsOfContour + 1;
			offsetInTable += sizeof(uint16_t);
		}

		// *** GlyphDescription.LengthOfInstructions
		uint16_t *p = (uint16_t *)&gdata[offsetInTable];
		uint16_t v0 = *p;
		uint16_t instructionLength = ntohs(v0);
		fprintf(stdout, "\n");
		fprintf(stdout, "	 Length of Instructions: %2d\n", instructionLength);

		offsetInTable += sizeof(uint16_t);
		for(int inst = 0; inst < instructionLength; inst++){
			ASSERTF(offsetInTable < ntohl(tableDirectory_GlyfTable->length),
					"%d", ntohl(tableDirectory_GlyfTable->length));

			uint8_t instruction = gdata[offsetInTable];
			fprintf(stdout, "	 Instruction[%02d]: 0x%02x\n", inst, instruction);
			offsetInTable += sizeof(uint8_t);
		}

		GlyphDescriptionPoint *gpoints = ffmalloc(pointNum * sizeof(GlyphDescriptionPoint));

		// *** GlyphDescription.Flags
		fprintf(stdout, "\n");
		fprintf(stdout,
			"	 Flags (pointNum:%2zd)\n"
			"	 -----\n",
			pointNum);

		for(int iflag = 0; iflag < pointNum; iflag++){
			ASSERTF(offsetInTable < ntohl(tableDirectory_GlyfTable->length),
					"%d", ntohl(tableDirectory_GlyfTable->length));

			uint8_t flag = gdata[offsetInTable];
			gpoints[iflag].flag = flag;
			//fprintf(stdout, "	 flag %2d: %s 0x%02x\n", iflag, GlyphDescriptionFlag_ToPrintString(flag), flag);
			if(0 != (flag & (1 << 3))){
				offsetInTable += sizeof(uint8_t);
				uint8_t repeatNum = gdata[offsetInTable];
				for(int rep = 0; rep < repeatNum; rep++){
					iflag++;
					//offsetInTable += sizeof(uint8_t);
					gpoints[iflag].isFlagRepeated = 1;
					gpoints[iflag].flag = flag;
					//fprintf(stdout, ">	 flag %2d: <repeated(%02d/%02d 0x%02x)>\n", iflag, rep, repeatNum, repeatNum);
				}
			}
			offsetInTable += sizeof(uint8_t);
		}

		for(int iflag = 0; iflag < pointNum; iflag++){
			bool isRepeated = (0 != gpoints[iflag].isFlagRepeated);
			uint8_t flag = gpoints[iflag].flag;
			fprintf(stdout, "	 flag %2d: %s 0x%02x %s\n",
					iflag, GlyphDescriptionFlag_ToPrintString(flag), flag, (isRepeated?"<repeated>":""));
		}

		//DUMP0(&gdata[offsetInTable], 8);

		// *** GlyphDescription.XYCoordinates
		fprintf(stdout, "\n");
		fprintf(stdout,
			"	 Coordinates\n"
			"	 -----------\n");
		// xCoordinates
		for(int xcor = 0; xcor < pointNum; xcor++){
			uint8_t flag = gpoints[xcor].flag;
			if((! GlyphFlag_IsXShortVector(flag)) && GlyphFlag_IsSameOrPisitiveXShortVector(flag)){
				gpoints[xcor].isRelXSame = 1;
				if(0 == xcor){
					//! @note 先頭の場合はゼロ(FontForgeの生成したフォントファイルによるとありうるらしい。)
					continue;
				}
				gpoints[xcor].raw.x = 0; //gpoints[xcor - 1].raw.x;
				gpoints[xcor].rel.x = 0; //gpoints[xcor - 1].rel.x;
				gpoints[xcor].abs.x = gpoints[xcor - 1].abs.x;
			}else{
				uint16_t raw;
				int16_t rel;
				if(GlyphFlag_IsXShortVector(flag)){
					raw = *(uint8_t *)(&gdata[offsetInTable]);
					rel = raw;
					offsetInTable += sizeof(uint8_t);
				}else{
					raw = ntohs(*(uint16_t *)(&gdata[offsetInTable]));
					rel = CONVERT_INT_FROM_UINT16T(raw);
					rel *= -1; //!< @todo 正直ttfdump合わせでよくわかってない
					offsetInTable += sizeof(uint16_t);
				}
				if(GlyphFlag_IsXShortVector(flag) && GlyphFlag_IsSameOrPisitiveXShortVector(flag)){
					rel *= +1;
				}else{
					rel *= -1;
				}
				int prev = (0 == xcor)? 0:gpoints[xcor - 1].abs.x;
				gpoints[xcor].raw.x = raw;
				gpoints[xcor].rel.x = rel;
				gpoints[xcor].abs.x = prev + rel;
			}
		}

		// yCoordinates
		for(int ycor = 0; ycor < pointNum; ycor++){
			uint8_t flag = gpoints[ycor].flag;
			if((! GlyphFlag_IsYShortVector(flag)) && GlyphFlag_IsSameOrPisitiveYShortVector(flag)){
				gpoints[ycor].isRelYSame = 1;
				if(0 == ycor){
					continue;
				}
				gpoints[ycor].raw.y = 0; //gpoints[ycor - 1].raw.y;
				gpoints[ycor].rel.y = 0; //gpoints[ycor - 1].rel.y;
				gpoints[ycor].abs.y = gpoints[ycor - 1].abs.y;
			}else{
				uint16_t raw;
				int16_t rel;
				if(GlyphFlag_IsYShortVector(flag)){
					raw = *(uint8_t *)(&gdata[offsetInTable]);
					rel = raw;
					offsetInTable += sizeof(uint8_t);
				}else{
					raw = ntohs(*(uint16_t *)(&gdata[offsetInTable]));
					rel = CONVERT_INT_FROM_UINT16T(raw);
					rel *= -1; //!< @todo 正直ttfdump合わせでよくわかってない
					offsetInTable += sizeof(uint16_t);
				}
				if(GlyphFlag_IsYShortVector(flag) && GlyphFlag_IsSameOrPisitiveYShortVector(flag)){
					rel *= +1;
				}else{
					rel *= -1;
				}
				int prev = (0 == ycor)? 0:gpoints[ycor - 1].abs.y;
				gpoints[ycor].raw.y = raw;
				gpoints[ycor].rel.y = rel;
				gpoints[ycor].abs.y = prev + rel;
			}
		}

		// print
		for(int cor = 0; cor < pointNum; cor++){
			uint8_t flag = gpoints[cor].flag;
			fprintf(stdout,
				"	 %2d Rel ( %6d, %6d) -> Abs ( %6d, %6d) | Raw ( %c%c0x%04x %6d, %c%c0x%04x %6d)\n"
				,
				cor,
				gpoints[cor].rel.x,
				gpoints[cor].rel.y,
				gpoints[cor].abs.x,
				gpoints[cor].abs.y,
				(GlyphFlag_IsXShortVector(flag)? 'S':'L'),
				((1 == gpoints[cor].isRelXSame)? 'X':'-'),
				gpoints[cor].raw.x,
				CONVERT_INT_FROM_UINT16T(gpoints[cor].raw.x),
				(GlyphFlag_IsYShortVector(flag)? 'S':'L'),
				((1 == gpoints[cor].isRelYSame)? 'X':'-'),
				gpoints[cor].raw.y,
				CONVERT_INT_FROM_UINT16T(gpoints[cor].raw.y)
				);
		}
	}
}


int main(int argc, char **argv)
{
	/**
	第1引数でフォントファイル名を指定する
	*/
	if(argc < 2){
		exit(1);
	}
	const char *fontfilepath = argv[1];

	// ** 引数：Table指定
	if(argc >= 3){
		if(0 == strcmp("-t", argv[2])){
			if(! (argc >= 4)){
				ERROR_LOG("invalid table name");
				exit(1);
			}
			if(4 != strlen(argv[3])){
				ERROR_LOG("invalid table name");
				exit(1);
			}
			strcpy(arg.tablename, argv[3]);
		}else if(0 == strcmp("--strict", argv[2])){
			arg.strictMode = FFStrictMode_ALL;
		}else{
			ERROR_LOG("invalid args");
			exit(1);
		}
	}

	int fd = open(fontfilepath, O_RDONLY, 0777);
	if(-1 == fd){
		fprintf(stderr, "open: %d %s\n", errno, strerror(errno));
		exit(1);
	}

	// ** OffsetTable
	OffsetTable offsetTable;
	ssize_t ssize;
	ssize = read(fd, (void *)&offsetTable, sizeof(offsetTable));
	if(ssize != sizeof(offsetTable)){
		fprintf(stderr, "read: %zd %d %s\n", ssize, errno, strerror(errno));
		exit(1);
	}
	offsetTable.sfntVersion		= ntohl(offsetTable.sfntVersion);
	offsetTable.numTables		= ntohs(offsetTable.numTables);

	const char *sfntversionstr = OffsetTable_SfntVersion_ToPrintString(offsetTable.sfntVersion);

	fprintf(stdout,
			"Font File Dumper: v %s\n"
			"project daisy bell 2019\n"
			"Dumping File:%s\n"
			"\n"
			"\n"
			"Offset Table\n"
			"------------\n"
			"	 sfnt version: 		%s\n"
			"	 number of tables:%3d\n",
			//FULL_VERSION,
			SHOW_VERSION,
			fontfilepath,
			sfntversionstr,
			offsetTable.numTables);

	size_t numTables = offsetTable.numTables; // use TableDirectory

	// ** TableDirectory
	TableDirectory_Member *tableDirectory = NULL;
	readTableDirectory(&tableDirectory, numTables, fd);

	// ** table指定jump
	if(0 == strlen(arg.tablename)){
		// NOP // table not selected
	}else if(0 == strcmp("cmap", arg.tablename)){
		goto CmapTable;
	}else{
		ERROR_LOG("invalid table name");
		exit(1);
	}

	// ** Tables
	uint16_t headTable_Host_indexToLocFormat = 0; // use LocaTable from HeadTable member

	headTable(tableDirectory, numTables, fd, &headTable_Host_indexToLocFormat);

	size_t maxpTable_Host_numGlyphs = 0; // use LocaTable from MaxpTable member

	maxpTable(tableDirectory, numTables, fd, &maxpTable_Host_numGlyphs);

CmapTable:
	cmapTable(tableDirectory, numTables, fd);
	if(0 == strcmp("cmap", arg.tablename)){
		goto finally;
	}

	uint32_t *locaList = NULL; // use GlyfTable from LocaTable

	locaTable(tableDirectory, numTables, fd,
			headTable_Host_indexToLocFormat, maxpTable_Host_numGlyphs,
			&locaList);

	glyfTable(tableDirectory, numTables, fd,
			maxpTable_Host_numGlyphs, locaList);

finally:

	close(fd);

	return 0;
}

