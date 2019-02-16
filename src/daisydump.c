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

char *FixedType_ToPrintString(FixedType fixedvalue)
{
	char *fixedstring = malloc(strlen("0000.0000") + 1);
	ASSERT(fixedstring);

	uint16_t major = (uint16_t)(fixedvalue >> 16);
	uint16_t minor = (uint16_t)(fixedvalue >>  0);
	sprintf(fixedstring, "%u.%u", major, minor);

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
		DEBUG_LOG("%04x %04x", tableDirectory[i].tag, tagvalue);
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
	size_t numTables = offsetTable.numTables;

	// ** HeadTable
	TableDirectory_Member *tableDirectory_HeadTable = TableDirectory_QueryTag(tableDirectory, numTables, TagType_Generate("head"));
	if(NULL == tableDirectory_HeadTable){
		FONT_WARN_LOG("HeadTable not detected.");
	}else{
		HeadTable headTable;

		errno = 0;
		off_t off = lseek(fd, ntohl(tableDirectory_HeadTable->offset), SEEK_SET);
		if(-1 == off){
			FONT_ERROR_LOG("lseek: %ld %d %s", off, errno, strerror(errno));
			return 1;
		}

		ssize_t ssize;
		ssize = read(fd, (void *)&headTable, sizeof(headTable));
		if(ssize != sizeof(headTable)){
			FONT_ERROR_LOG("read: %zd %d %s", ssize, errno, strerror(errno));
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
	}

	// ** MaxpTable
	TableDirectory_Member *tableDirectory_MaxpTable = TableDirectory_QueryTag(tableDirectory, numTables, TagType_Generate("maxp"));
	if(NULL == tableDirectory_MaxpTable){
		FONT_WARN_LOG("MaxpTable not detected.");
	}else{
		MaxpTable_Version05 maxpTable;
		size_t tableSize = sizeof(maxpTable);

		errno = 0;
		off_t off = lseek(fd, ntohl(tableDirectory_MaxpTable->offset), SEEK_SET);
		if(-1 == off){
			FONT_ERROR_LOG("lseek: %ld %d %s", off, errno, strerror(errno));
			return 1;
		}

		ssize_t ssize;
		ssize = read(fd, (void *)&maxpTable, tableSize);
		if(ssize != tableSize){
			FONT_ERROR_LOG("read: %zd %d %s", ssize, errno, strerror(errno));
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
	}

	close(fd);

	return 0;
}

