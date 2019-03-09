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

	int baseline = 300;

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
	BBox bBox = BBox_generate(50, 450, - baseline, 1000 - baseline); // 値は一応のデザインルールから仮の値
	HeadTable headTable;
	HeadTableFlagsElement	flags = (HeadTableFlagsElement)(0x0
			//| HeadTableFlagsElement_Bit0_isBaselineAtYIsZero
			| HeadTableFlagsElement_Bit1_isLeftSidebearingPointAtXIsZero
			//| HeadTableFlagsElement_Bit3_isPpemScalerMath
			//| HeadTableFlagsElement_Bit13_isClearType
			);
	ASSERT(HeadTable_init(
			&headTable,
			0x00010000,
			flags,
			LONGDATETIMEType_generate(timeFromStr("2019-01-01T00:00:00+00:00")),
			LONGDATETIMEType_generate(timeFromStr("2019-01-01T00:00:00+00:00")),
			(MacStyle)MacStyle_Bit6_Regular,
			bBox,
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
	HheaTable hheaTable = {0};
	HmtxTableBuf hmtxTableBuf = {0};

	size_t advanceWidth = 500;
	size_t lsb = 50;
	{
		// ** .notdefなどデフォルトの文字を追加
		//    & CmapTableテーブルにGlyphIdの初期値をセット
		//! @note Format0のBackspaceなどへのGlyphIdの割り当てはFontForgeの出力ファイルに倣った
		// *** .notdef
		GlyphDescriptionBuf glyphDescriptionBuf_notdef = {0};
		GlyphOutline outline_notdef = GlyphOutline_Notdef();
		GlyphDescriptionBuf_setOutline(&glyphDescriptionBuf_notdef, &outline_notdef);
		GlyphTablesBuf_appendSimpleGlyph(&glyphTablesBuf, 0x0, &glyphDescriptionBuf_notdef);
		HmtxTableBuf_appendLongHorMetric(&hmtxTableBuf, advanceWidth, lsb);

		// 下の2つのGlyphで使用する空の字形
		GlyphDescriptionBuf glyphDescriptionBuf_empty = {0};
		GlyphOutline outline_empty = {0};
		GlyphDescriptionBuf_setOutline(&glyphDescriptionBuf_empty, &outline_empty);
		// *** NUL and other
		GlyphTablesBuf_appendSimpleGlyph(&glyphTablesBuf, 0, &glyphDescriptionBuf_empty);
		glyphTablesBuf.cmapSubtableBuf_GlyphIdArray8[ 8] = 1; // BackSpace = index 1
		glyphTablesBuf.cmapSubtableBuf_GlyphIdArray8[29] = 1; // GroupSeparator = index 1
		HmtxTableBuf_appendLongHorMetric(&hmtxTableBuf, 0, 0);
		// *** TAB(HT) and other
		GlyphTablesBuf_appendSimpleGlyph(&glyphTablesBuf, '\t', &glyphDescriptionBuf_empty);
		glyphTablesBuf.cmapSubtableBuf_GlyphIdArray8[13] = 1; // CR = index 2
		HmtxTableBuf_appendLongHorMetric(&hmtxTableBuf, 1000, 0);

		// ** 目的の字形・文字を追加していく
		GlyphDescriptionBuf glyphDescriptionBuf_A = {0};
		GlyphOutline outline_A = GlyphOutline_A();
		GlyphDescriptionBuf_setOutline(&glyphDescriptionBuf_A, &outline_A);
		GlyphTablesBuf_appendSimpleGlyph(&glyphTablesBuf, 'A', &glyphDescriptionBuf_A);
		HmtxTableBuf_appendLongHorMetric(&hmtxTableBuf, advanceWidth, lsb);

		// ** 追加終了して集計・ByteArray化する。
		GlyphTablesBuf_finally(&glyphTablesBuf);
	}
	{
		size_t ascender			= 1000 - baseline;
		size_t descender		= baseline;
		size_t lineGap			= 24;
		size_t minLeftSideBearing	= lsb;
		size_t minRightSideBearing	= lsb;
		size_t xMaxExtent		= lsb + (bBox.xMax - bBox.xMin);
		HheaTable_init(
				&hheaTable,
				ascender,
				descender,
				lineGap,
				hmtxTableBuf.advanceWidthMax,
				minLeftSideBearing,
				minRightSideBearing,
				xMaxExtent,
				hmtxTableBuf.numberOfHMetrics);
	}
	HmtxTableBuf_finally(&hmtxTableBuf);

	/**
	'maxp' Table:
	 使用グリフ数。
	 TrueType必須Table。
	*/
	MaxpTable_Version05 maxpTable_Version05;
	ASSERT(MaxpTable_Version05_init(&maxpTable_Version05, glyphTablesBuf.numGlyphs));

	/**
	  'post' Table: PostScriptエンジン(プリンタ等)が使用する参考情報
	 */
	PostTable_Header postTable = {
		.version		= htonl(0x00030000),
		.italicAngle		= htonl(0x00000000),
		.underlinePosition	= htons(-125),
		.underlineThickness	= htons(50),
		.isFixedPitch		= htons(0x0001),
	};

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
	Tablebuf_appendTable(&tableBuf, "cmap", (void *)(glyphTablesBuf.cmapByteArray.data), glyphTablesBuf.cmapByteArray.length);
	Tablebuf_appendTable(&tableBuf, "loca", (void *)(glyphTablesBuf.locaByteArray.data), glyphTablesBuf.locaByteArray.length);
	Tablebuf_appendTable(&tableBuf, "glyf", (void *)(glyphTablesBuf.glyfData), glyphTablesBuf.glyfDataSize);
	Tablebuf_appendTable(&tableBuf, "hhea", (void *)(&hheaTable), sizeof(HheaTable));
	Tablebuf_appendTable(&tableBuf, "hmtx", (void *)(hmtxTableBuf.byteArray.data), hmtxTableBuf.byteArray.length);
	Tablebuf_appendTable(&tableBuf, "post", (void *)(&postTable), sizeof(PostTable_Header));

	// offsetは、Tableのフォントファイル先頭からのオフセット。先に計算しておく。
	const size_t offsetHeadSize = sizeof(OffsetTable) + (sizeof(TableDirectory_Member) * tableBuf.appendTableNum);
	Tablebuf_finallyTableDirectoryOffset(&tableBuf, offsetHeadSize);

	/**
	OffsetTable:
	 (Offset Subtable, sfnt)
	*/
	Uint32Type sfntVersion;
	//memcpy((uint8_t *)&sfntVersion, "OTTO", 4);
	sfntVersion = 0x00010000;
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

