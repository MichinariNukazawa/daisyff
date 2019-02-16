/**
  @file
  @author michianri.nukazawa@gmail.com / project daisy bell
  @details license: MIT
 */
#include "src/OpenType.h"
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

int main()
{

	longdatetime_test();

	fprintf(stdout, "success.\n");

	return 0;
}

