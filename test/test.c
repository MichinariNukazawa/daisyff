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

void longdatetime_test()
{
	LONGDATETIME t;
	/*
	// https://support.microsoft.com/ja-jp/help/214330/differences-between-the-1900-and-the-1904-date-system-in-excel
	t = LONGDATETIME_generate(timeFromStr("1904-07-05T00:00:00+00:00"));
	DEBUG_LOG("%u", t);
	EXPECT_EQ_UINT(34519, t);
	 */

	// https://nixeneko.hatenablog.com/entry/2018/06/20/000000 (Noto Sans CJK JP RegularのVersion 1.004)
	//DEBUG_LOG("diff: %u", 0x00000000D1A40DF0 - LONGDATETIME_generate(timeFromStr("2015-06-15T05:06:56+00:00")));
	//EXPECT_EQ_UINT(0x00000000D1A40DF0, LONGDATETIME_generate(timeFromStr("2015-06-15T05:06:56+00:00")));

	// https://nixeneko.hatenablog.com/entry/2016/10/08/001900 (Noto Sans Regular based)
	DEBUG_LOG("diff: %16"PRIu64"", 0x00000000D3FF1335 - LONGDATETIME_generate(timeFromStr("2016-09-14T14:46:13+00:00")));
	EXPECT_EQ_UINT(0x00000000D3FF1335, LONGDATETIME_generate(timeFromStr("2016-09-14T14:46:13+00:00")));
}

void glyphOutline0_test()
{
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

	GlyphDiscriptionBuf gdb_;
	GlyphDiscriptionBuf *gdb = &gdb_;
	GlyphDiscriptionBuf_setOutline(gdb, &outline);

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
}

int main()
{

	longdatetime_test();
	glyphOutline0_test();

	fprintf(stdout, "success.\n");

	return 0;
}

