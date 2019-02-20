/**
  @file
  @author michianri.nukazawa@gmail.com / project daisy bell
  @details license: MIT
 */

#include "src/OpenType.h"
#include "include/version.h"
#include <inttypes.h>

#define FONT_ERROR_LOG(fmt, ...) \
	fprintf(stderr, "font error: %s()[%d]: "fmt"\n", __func__, __LINE__, ## __VA_ARGS__)
#define FONT_WARN_LOG(fmt, ...) \
	fprintf(stderr, "font warning: %s()[%d]: "fmt"\n", __func__, __LINE__, ## __VA_ARGS__)

uint64_t ntohll(uint64_t x)
{
	if(IS_LITTLE_ENDIAN()){
		return bswap_64(x);
	}else{
		return x;
	}
}

typedef uint32_t TagType;

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
	char *tagstr = malloc(strlen("0x12345678") + 1);
	ASSERT(tagstr);

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

typedef uint32_t FixedType;

//!< @todo ここは正直正しいのかどうかよくわからない(minorが複数桁の場合)
char *FixedType_ToPrintString(FixedType fixedvalue)
{
	char *fixedstring = malloc(strlen("0000.0000") + 1);
	ASSERT(fixedstring);

	uint16_t major = (uint16_t)(fixedvalue >> 16);
	uint16_t minor = (uint16_t)(fixedvalue >>  0);
	sprintf(fixedstring, "%04x.%04x", major, minor);

	return fixedstring;
}

typedef uint64_t LongdatetimeType;

char *LongdatetimeType_ToPrintString(LongdatetimeType longdatetimevalue)
{
	time_t time = longdatetimevalue - LONGDATETIME_DELTA;

	struct tm tm;

	ASSERT(NULL != gmtime_r(&time, &tm));

	char *tstring = malloc(256);
	ASSERT(tstring);
	size_t size = strftime(tstring, 256, "%Y-%m-%dT%H:%M:%S", &tm);

	return tstring;
}

const char *OffsetTable_SfntVersion_ToPrintString(uint32_t sfntVersion)
{
	return TagType_ToPrintString(sfntVersion);
}

TableDirectory_Member *TableDirectory_QueryTag(TableDirectory_Member *tableDirectory, size_t numTables, tag tagvalue)
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

enum LocaTable_Kind{
	LocaTable_Kind_Short	= 0,
	LocaTable_Kind_Long	= 1,
};
typedef int LocaTable_Kind;

char *GlyphDiscriptionFlag_ToPrintString(uint8_t flag)
{
	char *str = malloc(512);
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
}GlyphDiscriptionPoint;

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

void dump0(uint8_t *buf, size_t size)
{
	fprintf(stdout, " 0x ");
	for(int i = 0; i < size; i++){
		if((0 != i) && (0 == (i % 8))){
			fprintf(stdout, "\n");
		}
		fprintf(stdout, "%02x-", buf[i]);
	}
	fprintf(stdout, "\n");
}

int main(int argc, char **argv)
{
	/**
	第1引数でフォントファイル名を指定する
	*/
	if(argc < 2){
		return 1;
	}
	const char *fontfilepath = argv[1];

	int fd = open(fontfilepath, O_RDONLY, 0777);
	if(-1 == fd){
		fprintf(stderr, "open: %d %s\n", errno, strerror(errno));
		return 1;
	}

	// ** OffsetTable
	OffsetTable offsetTable;
	ssize_t ssize;
	ssize = read(fd, (void *)&offsetTable, sizeof(offsetTable));
	if(ssize != sizeof(offsetTable)){
		fprintf(stderr, "read: %zd %d %s\n", ssize, errno, strerror(errno));
		return 1;
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

	// ** TableDirectory
	TableDirectory_Member *tableDirectory = NULL;
	for(int i = 0; i < offsetTable.numTables; i++){
		TableDirectory_Member tableDirectory_Member;
		ssize_t ssize;
		ssize = read(fd, (void *)&tableDirectory_Member, sizeof(tableDirectory_Member));
		if(ssize != sizeof(tableDirectory_Member)){
			FONT_ERROR_LOG("read: %zd %d %s", ssize, errno, strerror(errno));
			return 1;
		}

		tableDirectory = realloc(tableDirectory, sizeof(TableDirectory_Member) * (i + 1));
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

	size_t numTables = offsetTable.numTables; // use TableDirectory

	uint16_t headTable_Host_indexToLocFormat = 0; // use LocaTable from HeadTable member

	// ** HeadTable
	TableDirectory_Member *tableDirectory_HeadTable = TableDirectory_QueryTag(tableDirectory, numTables, TagType_Generate("head"));
	if(NULL == tableDirectory_HeadTable){
		FONT_WARN_LOG("HeadTable not detected.");
	}else{
		HeadTable headTable;

		if(! copyrange(fd, (void *)&headTable, ntohl(tableDirectory_HeadTable->offset), sizeof(headTable))){
			FONT_ERROR_LOG("copyrange: %d %s", errno, strerror(errno));
			return 1;
		}

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
			LongdatetimeType_ToPrintString(headTable_Host.created)		,
			headTable_Host.modified		,
			LongdatetimeType_ToPrintString(headTable_Host.modified)		,
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

		headTable_Host_indexToLocFormat = headTable_Host.indexToLocFormat;
	}

	size_t maxpTable_Host_numGlyphs = 0; // use LocaTable from MaxpTable member

	// ** MaxpTable
	TableDirectory_Member *tableDirectory_MaxpTable = TableDirectory_QueryTag(tableDirectory, numTables, TagType_Generate("maxp"));
	if(NULL == tableDirectory_MaxpTable){
		FONT_WARN_LOG("MaxpTable not detected.");
	}else{
		MaxpTable_Version05 maxpTable;
		size_t tableSize = sizeof(maxpTable);

		if(! copyrange(fd, (void *)&maxpTable, ntohl(tableDirectory_MaxpTable->offset), sizeof(maxpTable))){
			FONT_ERROR_LOG("copyrange: %d %s", errno, strerror(errno));
			return 1;
		}

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

		maxpTable_Host_numGlyphs = maxpTable_Host.numGlyphs;
	}

	uint32_t *locaList = NULL; // use GlyfTable from LocaTable

	// ** LocaTable
	LocaTable_Kind locaTable_Kind = ((0 == headTable_Host_indexToLocFormat) ? LocaTable_Kind_Short : LocaTable_Kind_Long);
	TableDirectory_Member *tableDirectory_LocaTable = TableDirectory_QueryTag(tableDirectory, numTables, TagType_Generate("loca"));
	if(NULL == tableDirectory_LocaTable){
		FONT_WARN_LOG("LocaTable not detected.");
	}else{
		size_t locaOffsetSize = (locaTable_Kind == LocaTable_Kind_Short) ? sizeof(uint16_t):sizeof(uint32_t);
		size_t tableSize = locaOffsetSize * (maxpTable_Host_numGlyphs + 1);
		uint8_t locaTable[tableSize];

		if(! copyrange(fd, (void *)&locaTable, ntohl(tableDirectory_LocaTable->offset), sizeof(locaTable))){
			FONT_ERROR_LOG("copyrange: %d %s", errno, strerror(errno));
			return 1;
		}

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

			locaList = (uint32_t *)realloc(locaList, sizeof(uint32_t) * (i + 1));
			ASSERT(locaList);
			locaList[i] = dv;

			if(i != maxpTable_Host_numGlyphs){
				fprintf(stdout, "	 Idx %6d -> GlyphOffset 0x%08x(0x%08x %6u)\n", i, dv, sv, dv);
			}else{
				fprintf(stdout, "	                  Ended at 0x%08x(0x%08x %6u)\n", dv, sv, dv);
			}
		}
	}

	// ** GlyfTable
	TableDirectory_Member *tableDirectory_GlyfTable = TableDirectory_QueryTag(tableDirectory, numTables, TagType_Generate("glyf"));
	if(NULL == tableDirectory_GlyfTable){
		FONT_WARN_LOG("GlyfTable not detected.");
	}else{
		fprintf(stdout, "\n");
		fprintf(stdout,
			"'glyf' Table - Glyph Data\n"
			"-------------------------\n");

		for(int glyphId = 0; glyphId< maxpTable_Host_numGlyphs; glyphId++){
			size_t offsetOnTable = locaList[glyphId];
			size_t datasize = locaList[glyphId + 1] - locaList[glyphId];

			// *** GlyphDiscription.Header
			GlyphDiscriptionHeader glyphDiscriptionHeader;
			if(! copyrange(fd, (void *)&glyphDiscriptionHeader, ntohl(tableDirectory_GlyfTable->offset) + offsetOnTable, sizeof(GlyphDiscriptionHeader))){
				FONT_ERROR_LOG("copyrange: %d %s", errno, strerror(errno));
				return 1;
			}

			GlyphDiscriptionHeader glyphDiscriptionHeader_Host = {
				.numberOfContours	= ntohs(glyphDiscriptionHeader.numberOfContours		),
				.xMin			= ntohs(glyphDiscriptionHeader.xMin			),
				.yMin			= ntohs(glyphDiscriptionHeader.yMin			),
				.xMax			= ntohs(glyphDiscriptionHeader.xMax			),
				.yMax			= ntohs(glyphDiscriptionHeader.yMax			),
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
				glyphDiscriptionHeader_Host.numberOfContours	,
				glyphDiscriptionHeader_Host.xMin		,
				glyphDiscriptionHeader_Host.yMin		,
				glyphDiscriptionHeader_Host.xMax		,
				glyphDiscriptionHeader_Host.yMax		);

			if(0 == datasize){
				fprintf(stdout, "	 skip datasize is zero.\n");
				continue;
			}
			//! @todo check GlyphDiscription elemetns on memory data range.

			uint8_t *gdata = malloc(datasize);
			if(! copyrange(fd, gdata, ntohl(tableDirectory_GlyfTable->offset) + offsetOnTable, datasize)){
				FONT_ERROR_LOG("copyrange: %d %s", errno, strerror(errno));
				return 1;
			}

			//dump0(gdata, sizeof(GlyphDiscriptionHeader));
			//dump0(&gdata[sizeof(GlyphDiscriptionHeader)], 16);

			size_t pointNum = 0; //! @todo use flags from EndPoints?

			size_t offsetInTable = 0;

			// *** GlyphDiscription.EndPoints
			offsetInTable += sizeof(GlyphDiscriptionHeader);
			fprintf(stdout, "\n");
			fprintf(stdout,
				"	 EndPoints (%d)\n"
				"	 ---------\n",
				glyphDiscriptionHeader_Host.numberOfContours
				);
			for(int co = 0; co < glyphDiscriptionHeader_Host.numberOfContours; co++){
				ASSERTF(offsetInTable < ntohl(tableDirectory_GlyfTable->length),
						"%d", ntohl(tableDirectory_GlyfTable->length));

				uint16_t *p = (uint16_t *)&gdata[offsetInTable];
				uint16_t v = *p;
				uint16_t endPtsOfContour = ntohs(v);
				fprintf(stdout, "	 %2d: %2d\n", co, endPtsOfContour);

				pointNum = endPtsOfContour + 1;
				offsetInTable += sizeof(uint16_t);
			}

			// *** GlyphDiscription.LengthOfInstructions
			uint16_t *p = (uint16_t *)&gdata[offsetInTable];
			uint16_t v = *p;
			uint16_t instructionLength = ntohs(v);
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

			GlyphDiscriptionPoint *gpoints = malloc(pointNum * sizeof(GlyphDiscriptionPoint));
			ASSERT(gpoints);
			memset(gpoints, 0, (pointNum * sizeof(GlyphDiscriptionPoint)));

			// *** GlyphDiscription.Flags
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
				fprintf(stdout, "	 flag %2d: %s 0x%02x\n", iflag, GlyphDiscriptionFlag_ToPrintString(flag), flag);
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
						iflag, GlyphDiscriptionFlag_ToPrintString(flag), flag, (isRepeated?"<repeated>":""));
			}

			//dump0(&gdata[offsetInTable], 8);

			// *** GlyphDiscription.XYCoordinates
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
					int v;
					if(GlyphFlag_IsXShortVector(flag)){
						raw = *(uint8_t *)(&gdata[offsetInTable]);
						v = raw;
						offsetInTable += sizeof(uint8_t);
					}else{
						raw = ntohs(*(uint16_t *)(&gdata[offsetInTable]));
						//int16_t vv;
						//memcpy((void*)&vv, (void*)&raw, sizeof(int16_t));
						//v = vv;
						v = raw * -1; //! @todo @note ttxdump合わせなのだけれど理由がわからない
						offsetInTable += sizeof(uint16_t);
					}
					int rel = v;
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
					int16_t v;
					if(GlyphFlag_IsYShortVector(flag)){
						raw = *(uint8_t *)(&gdata[offsetInTable]);
						v = raw;
						offsetInTable += sizeof(uint8_t);
					}else{
						/*
						uint8_t rawx[2];
						memcpy((void*)rawx, (void*)(&gdata[offsetInTable]), sizeof(int16_t));
						//raw = ntohs(rawx);
						raw = (int16_t)(((uint16_t)rawx[0] << 8) | (uint16_t)rawx[1]);
						int16_t vv;
						memcpy((void*)(&vv), (void*)(&raw), sizeof(uint16_t));
						v = vv;
						v = raw * -1;
						*/
						//DEBUG_LOG("%04x %04x %d %d", raw, rawx, vv, v);
						//memcpy((void*)&v, (void*)(&gdata[offsetInTable]), sizeof(int16_t));
						raw = ntohs(*(uint16_t *)(&gdata[offsetInTable]));
						v = raw * -1; //! @todo @note ttxdump合わせなのだけれど理由がわからない
						//int16_t vv;
						//memcpy((void*)&vv, (void*)&raw, sizeof(int16_t));
						//v = vv;
						//v = raw * -1; //! @todo @note ttxdump合わせなのだけれど理由がわからない
						offsetInTable += sizeof(uint16_t);
					}
					int rel = v;
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
					gpoints[cor].raw.x,
					(GlyphFlag_IsYShortVector(flag)? 'S':'L'),
					((1 == gpoints[cor].isRelYSame)? 'X':'-'),
					gpoints[cor].raw.y,
					gpoints[cor].raw.y);
			}
		}
	}

	close(fd);

	return 0;
}

