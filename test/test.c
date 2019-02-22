/**
  @file
  @author michianri.nukazawa@gmail.com / project daisy bell
  @details license: MIT
 */
#include "src/OpenType.h"
#include "src/GlyphOutline.h"
#include <stdio.h>
#include <inttypes.h>


/** 内部でuint64_tを使っているので一部で正しく動作しないかも(オーバーフローのラップアラウンドなど) */
#define EXPECT_EQ_UINT(ARG0, ARG1) \
	do{ \
		uint64_t ARG0V = (ARG0); \
		uint64_t ARG1V = (ARG1); \
		if((ARG0V) < (ARG1V)){ \
			fprintf(stderr, "EXPECT_EQ_UINT: %s()[%d]:('%s','%s') %"PRIu64"(0x%016"PRIx64") < %"PRIu64"(0x%016"PRIx64")\n", \
					__func__, __LINE__, #ARG0, #ARG1, ARG0V, ARG0V, ARG1V, ARG1V); \
			exit(1); \
		} \
		if((ARG0V) > (ARG1V)){ \
			fprintf(stderr, "EXPECT_EQ_UINT: %s()[%d]:('%s','%s') %"PRIu64"(0x%016"PRIx64") > %"PRIu64"(0x%016"PRIx64")\n", \
					__func__, __LINE__, #ARG0, #ARG1, ARG0V, ARG0V, ARG1V, ARG1V); \
			exit(1); \
		} \
	}while(0);

#define EXPECT_NE_UINT(ARG0, ARG1) \
	do{ \
		uint64_t ARG0V = (ARG0); \
		uint64_t ARG1V = (ARG1); \
		if((ARG0V) == (ARG1V)){ \
			fprintf(stderr, "EXPECT_NE_UINT: %s()[%d]:('%s','%s') %"PRIu64"(0x%016"PRIx64") < %"PRIu64"(0x%016"PRIx64")\n", \
					__func__, __LINE__, #ARG0, #ARG1, ARG0V, ARG0V, ARG1V, ARG1V); \
			exit(1); \
		} \
	}while(0);

#define EXPECT_TRUE(ARG0) \
	do{ \
		uint64_t ARG0V = (ARG0); \
		if(! (ARG0V)){ \
			fprintf(stderr, "EXPECT_TRUE: %s()[%d]:('%s')\n", \
					__func__, __LINE__, #ARG0); \
			exit(1); \
		} \
	}while(0);


#define EXPECT_EQ_ARRAY(ARG0, ARG1, ARG2) \
	do{ \
		const uint8_t *ARRAY0 = (ARG0); \
		const uint8_t *ARRAY1 = (ARG1); \
		size_t LENGTH = (ARG2); \
		for(int i = 0; i < LENGTH; i++){ \
			if((ARRAY0[i]) != (ARRAY1[i])){ \
				fprintf(stderr, "EXPECT_EQ_ARRAY: %s()[%d]:('%s','%s')[%d/%zu] %d(0x%02x) != %d(0x%02x) \n", \
						__func__, __LINE__, #ARG0, #ARG1, i, LENGTH, ARRAY0[i], ARRAY0[i], ARRAY1[i], ARRAY1[i]); \
				exit(1); \
			} \
		}\
	}while(0);


void longdatetime_test()
{
	DEBUG_LOG("in");

	LONGDATETIMEType t;
	/*
	// https://support.microsoft.com/ja-jp/help/214330/differences-between-the-1900-and-the-1904-date-system-in-excel
	t = LONGDATETIMEType_generate(timeFromStr("1904-07-05T00:00:00+00:00"));
	DEBUG_LOG("%u", t);
	EXPECT_EQ_UINT(34519, t);
	 */

	// https://nixeneko.hatenablog.com/entry/2018/06/20/000000 (Noto Sans CJK JP RegularのVersion 1.004)
	//DEBUG_LOG("diff: %u", 0x00000000D1A40DF0 - LONGDATETIMEType_generate(timeFromStr("2015-06-15T05:06:56+00:00")));
	//EXPECT_EQ_UINT(0x00000000D1A40DF0, LONGDATETIMEType_generate(timeFromStr("2015-06-15T05:06:56+00:00")));

	// https://nixeneko.hatenablog.com/entry/2016/10/08/001900 (Noto Sans Regular based)
	//DEBUG_LOG("diff: %16"PRIu64"", 0x00000000D3FF1335 - LONGDATETIMEType_generate(timeFromStr("2016-09-14T14:46:13+00:00")));
	EXPECT_EQ_UINT(0, 0x00000000D3FF1335 - LONGDATETIMEType_generate(timeFromStr("2016-09-14T14:46:13+00:00")));
	EXPECT_EQ_UINT(0x00000000D3FF1335, LONGDATETIMEType_generate(timeFromStr("2016-09-14T14:46:13+00:00")));

	DEBUG_LOG("out");
}

void glyphOutline0_test()
{
	DEBUG_LOG("in");

	GlyphOutline outline = {0};
	GlyphClosePath cpath0 = {0};
	GlyphAnchorPoint apoints0[] = {
		{{  50, 100},},
		{{ 450, 100},},
		{{ 450, 600},},
		{{  50, 600},},
	};
	GlyphClosePath_addAnchorPoints(&cpath0, apoints0, sizeof(apoints0) / sizeof(GlyphAnchorPoint));
	GlyphOutline_addClosePath(&outline, &cpath0);

	GlyphDescriptionBuf gdb_;
	GlyphDescriptionBuf *gdb = &gdb_;
	GlyphDescriptionBuf_setOutline(gdb, &outline);

	EXPECT_EQ_UINT(gdb->numberOfContours, 1);
	EXPECT_TRUE(gdb->endPoints	!= NULL);
	EXPECT_EQ_UINT(gdb->pointNum, 4);
	EXPECT_TRUE(gdb->flags		!= NULL);
	EXPECT_EQ_UINT(gdb->instructionLength, 0);
	EXPECT_TRUE(gdb->instructions	== NULL);
	EXPECT_TRUE(gdb->xCoodinates	!= NULL);
	EXPECT_TRUE(gdb->yCoodinates	!= NULL);

	int16_t *xCoodinates = (int16_t *)(gdb->xCoodinates);
	int16_t *yCoodinates = (int16_t *)(gdb->yCoodinates);
	EXPECT_EQ_UINT(xCoodinates[0],  50);
	EXPECT_EQ_UINT(yCoodinates[0], 100);
	EXPECT_EQ_UINT(xCoodinates[1], 450);
	EXPECT_EQ_UINT(yCoodinates[1], 100);
	EXPECT_EQ_UINT(xCoodinates[2], 450);
	EXPECT_EQ_UINT(yCoodinates[2], 600);
	EXPECT_EQ_UINT(xCoodinates[3],  50);
	EXPECT_EQ_UINT(yCoodinates[3], 600);

	DEBUG_LOG("out");
}

void glyphDescriptionBufEmpty_test()
{
	DEBUG_LOG("in");

	uint8_t dstarray[] = {
			0,0, // int16 numberOfContours;
			0,0, // int16 xMin;
			0,0, // int16 yMin;
			0x03,0xe8, // int16 xMax; = 1000
			0x03,0xe8, // int16 yMax; = 1000
			//uint16_t	*endPoints;
			0,0, //uint16_t	instructionLength;
			//uint8_t		*instructions;
			//uint8_t		*flags;
			//uint8_t		*xCoodinates;
			//uint8_t		*yCoodinates;
	};

	GlyphDescriptionBuf glyphDescriptionBuf = {0};
	GlyphDescriptionBuf_generateByteData(&glyphDescriptionBuf);

	EXPECT_EQ_UINT(glyphDescriptionBuf.dataSize, sizeof(dstarray));
	EXPECT_EQ_ARRAY(glyphDescriptionBuf.data, dstarray, sizeof(dstarray))

	DEBUG_LOG("out");
}

int main()
{

	longdatetime_test();
	glyphOutline0_test();
	glyphDescriptionBufEmpty_test();

	fprintf(stdout, "success.\n");

	return 0;
}

