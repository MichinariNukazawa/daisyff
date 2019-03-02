/**
  @file
  @author michianri.nukazawa@gmail.com / project daisy bell
  @details license: MIT
 */

#include "src/OpenType.h"

GlyphOutline GlyphOutline_A()
{
	// ** //! @todo flags repeat, Coodinates SHORT_VECTOR
	GlyphOutline outline = {0};

	GlyphClosePath cpath0 = {0};
	GlyphAnchorPoint apoints0[] = {
		{{  50, 100},},
		{{ 250, 600},},
		{{ 450, 100},},
		{{ 250, 180},},
	};
	GlyphClosePath_addAnchorPoints(&cpath0, apoints0, sizeof(apoints0) / sizeof(apoints0[0]));
	GlyphOutline_addClosePath(&outline, &cpath0);

	return outline;
}
int main(int argc, char **argv)
{
	/**
	第1引数でフォントファイル名を指定する
	*/
	if(argc < 2){
		return 1;
	}
	const char *fontname = argv[1];

	/**
	CFF(OpenType)(MSSPEC)の要求する以下の必須テーブルを作成していく。
	cmap, head, hhea, hmtx, maxp, name, OS/2, post
	TrueType・CFF共通の必須テーブル
	TrueType(AppleSPEC)
	cmap, glyph, head, hhea, hmtx, maxp, loca, maxp, name, post
	を作成していく。
	*/

	/**
	  'head' Table
	  */
	HeadTable headTable;
	HeadTableFlagsElement	flags = (HeadTableFlagsElement)(0x0
			& HeadTableFlagsElement_Bit5_isRegardingInApplePlatform
			& HeadTableFlagsElement_Bit11_isLosslessCompress
			& HeadTableFlagsElement_Bit12_isFontConverted
			& HeadTableFlagsElement_Bit13_isClearType
			& HeadTableFlagsElement_Bit14_isResortforCodePoint
			);
	ASSERT(HeadTable_init(
			&headTable,
			0x00010000,
			flags,
			LONGDATETIMEType_generate(timeFromStr("2019-01-01T00:00:00+00:00")),
			LONGDATETIMEType_generate(timeFromStr("2019-01-01T00:00:00+00:00")),
			(MacStyle)MacStyle_Bit6_Regular,
			BBox_generate(0,0, 1000, 1000),
			8
			));

	/**
	  'name' Table
	  */
	NameTableBuf nameTableBuf = NameTableBuf_init(
			"(c)Copyright the project daisy bell 2019", //"©Copyright the project daisy bell 2019",
			fontname,
			(MacStyle)MacStyle_Bit6_Regular,
			"Version 1.0",
			"project daisy bell",
			"MichinariNukazawa",
			"https://daisy-bell.booth.pm/",
			"https://twitter.com/MNukazawa"
			);

	/**
	  'glyf' Table
	  and 'loca' Table (glyph descriptions offset)
	  */
	GlyphTablesBuf glyphTablesBuf;
	GlyphTablesBuf_init(&glyphTablesBuf);

	GlyphDescriptionBuf glyphDescriptionBuf_A = {0};
	GlyphOutline outline_A = GlyphOutline_A();
	GlyphDescriptionBuf_generateByteDataWithOutline(&glyphDescriptionBuf_A, &outline_A);

	GlyphTablesBuf_appendSimpleGlyph(&glyphTablesBuf, 'A', &glyphDescriptionBuf_A);
	GlyphTablesBuf_finally(&glyphTablesBuf);

	/**
	'maxp' Table:
	 使用グリフ数。
	 TrueType必須Table。
	*/
	MaxpTable_Version05 maxpTable_Version05;
	ASSERT(MaxpTable_Version05_init(&maxpTable_Version05, glyphTablesBuf.numGlyphs));

	/**
	TableDiectoryを生成しつつ、Tableをバイト配列に変換して繋げていく。
		Tableにはパディングを入れる。
		TableDirectoryを作っていく(Table情報の配列)。
		OffsetTable生成時に必要なテーブル数を数えておく。
	  */
	Tablebuf tableBuf;
	Tablebuf_init(&tableBuf);

	//Tablebuf_appendTable(&tableBuf, (void *)(&offsetTable), sizeof(OffsetTable));
	Tablebuf_appendTable(&tableBuf, "head", (void *)(&headTable), sizeof(HeadTable));
	Tablebuf_appendTable(&tableBuf, "name", (void *)(nameTableBuf.data), nameTableBuf.dataSize);
	Tablebuf_appendTable(&tableBuf, "maxp", (void *)(&maxpTable_Version05), sizeof(MaxpTable_Version05));
	Tablebuf_appendTable(&tableBuf, "cmap", (void *)(glyphTablesBuf.cmapData), glyphTablesBuf.cmapDataSize);
	Tablebuf_appendTable(&tableBuf, "loca", (void *)(glyphTablesBuf.locaData), glyphTablesBuf.locaDataSize);
	Tablebuf_appendTable(&tableBuf, "glyf", (void *)(glyphTablesBuf.glyfData), glyphTablesBuf.glyfDataSize);

	// offsetは、Tableのフォントファイル先頭からのオフセット。先に計算しておく。
	const size_t offsetHeadSize = sizeof(OffsetTable) + (sizeof(TableDirectory_Member) * tableBuf.appendTableNum);
	Tablebuf_finallyTableDirectoryOffset(&tableBuf, offsetHeadSize);

	/**
	OffsetTable:
	 (Offset Subtable, sfnt)
	*/
	Uint32Type sfntVersion;
	memcpy((uint8_t *)&sfntVersion, "OTTO", 4);
	//sfntVersion = 0x00010000;
	OffsetTable offsetTable;
	ASSERT(OffsetTable_init(&offsetTable, sfntVersion, tableBuf.appendTableNum));

	size_t fontDataSize =
		sizeof(OffsetTable)
		+ (sizeof(TableDirectory_Member) * tableBuf.appendTableNum)
		+ tableBuf.dataSize
		;
	// 開放はアプリ終了時に任せる
	uint8_t *fontData = (uint8_t *)ffmalloc(sizeof(uint8_t) * fontDataSize);

	/**
	  フォントデータを連結(HeadTable.checkSumAdjustmentの計算に用いる)
	  */
	DEBUG_LOG("OffsetTable :%zu", sizeof(OffsetTable));
	DEBUG_LOG("TableDirectory :%zu", sizeof(TableDirectory_Member) * tableBuf.appendTableNum);
	DEBUG_LOG("Table :%zu", tableBuf.dataSize);

	size_t offset = 0;
	memcpy(&fontData[offset], (uint8_t *)&offsetTable, sizeof(OffsetTable));
	offset += sizeof(OffsetTable);
	memcpy(&fontData[offset], (uint8_t *)tableBuf.tableDirectory, (sizeof(TableDirectory_Member) * tableBuf.appendTableNum));
	offset += (sizeof(TableDirectory_Member) * tableBuf.appendTableNum);
	memcpy(&fontData[offset], (uint8_t *)tableBuf.data, tableBuf.dataSize);

	/**
	  'head'TableにcheckSumAdjustment要素を計算して書き込む。
	  */
	Uint32Type checkSumAdjustment = 0xB1B0AFBA - CalcTableChecksum((uint32_t *)fontData, fontDataSize);
	size_t checkSumAdjustmentOffset =
		sizeof(OffsetTable)
		+ (sizeof(TableDirectory_Member) * tableBuf.appendTableNum)
		+ 0 // 'head' Tableは先頭に置くこととする。
		+ offsetof(HeadTable, checkSumAdjustment)
		;
	DEBUG_LOG("checkSumAdjustment:%zu(0x%04lx) 0x%08x",
			checkSumAdjustmentOffset, checkSumAdjustmentOffset, checkSumAdjustment);
	uint32_t *checkSumAdjustmentPointer = (uint32_t *)&(fontData[checkSumAdjustmentOffset]);
	*checkSumAdjustmentPointer = htonl(checkSumAdjustment);

	/**
	  ファイル書き出し
	  */
	char *fontfilename = (char *)ffmalloc(strlen(fontname) + 5);
	sprintf(fontfilename, "%s.otf", fontname);
	int fd = open(fontfilename, O_CREAT|O_TRUNC|O_RDWR, 0777);
	if(-1 == fd){
		fprintf(stderr, "open: %d %s\n", errno, strerror(errno));
		return 1;
	}
	ssize_t s;
	s = write(fd, fontData, fontDataSize);
	if(0 == s){
		fprintf(stderr, "write: %d %s\n", errno, strerror(errno));
		return 1;
	}
	close(fd);

	return 0;
}

