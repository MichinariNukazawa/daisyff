/**
  @file
  @author michianri.nukazawa@gmail.com / project daisy bell
  @details license: MIT
 */

#include "src/OpenType.hpp"

int main(int argc, char **argv)
{
	/**
	第1引数でフォントファイル名を指定する
	*/
	if(argc < 2){
		return 1;
	}

	/**
	TrueType・CFF共通の必須テーブル
	cmap, head, hhea, hmtx, maxp, name, OS/2
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
			Fixed_generate(1,0),
			flags,
			LONGDATETIME_generate(timeFromStr("2019-01-01T00:00:00+00:00")),
			LONGDATETIME_generate(timeFromStr("2019-01-01T00:00:00+00:00")),
			(MacStyle)MacStyle_Bit6_Regular,
			BBox_generate(0,0, 1000, 1000),
			8
			));

	/**
	'maxp' Table:
	 使用グリフ数。
	 TrueType必須Table。
	*/
	int numGlyphs = 0;
	MaxpTable_SixByte maxpTable_SixByte;
	ASSERT(MaxpTable_SixByte_init(&maxpTable_SixByte, numGlyphs));

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
	Tablebuf_appendTable(&tableBuf, "maxp", (void *)(&maxpTable_SixByte), sizeof(MaxpTable_SixByte));

	// offsetは、Tableのフォントファイル先頭からのオフセット。先に計算しておく。
	const size_t offsetHeadSize = sizeof(OffsetTable) + (sizeof(TableDirectory_Member) * tableBuf.appendTableNum);
	Tablebuf_finallyTableDirectoryOffset(&tableBuf, offsetHeadSize);

	/**
	OffsetTable:
	 (Offset Subtable, sfnt)
	*/
	uint32 sfntVersion;
	memcpy((uint8_t *)&sfntVersion, "OTTO", 4);
	OffsetTable offsetTable;
	ASSERT(OffsetTable_init(&offsetTable, sfntVersion, tableBuf.appendTableNum));

	size_t fontDataSize =
		sizeof(OffsetTable)
		+ (sizeof(TableDirectory_Member) * tableBuf.appendTableNum)
		+ tableBuf.dataSize
		;
	// 開放はアプリ終了時に任せる
	uint8_t *fontData = (uint8_t *)malloc(sizeof(uint8_t) * fontDataSize);
	ASSERT(fontData);

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
	uint32 checkSumAdjustment = 0xB1B0AFBA - CalcTableChecksum((uint32_t *)fontData, fontDataSize);
	size_t checkSumAdjustmentOffset =
		sizeof(OffsetTable)
		+ (sizeof(TableDirectory_Member) * tableBuf.appendTableNum)
		+ 0 // 'head' Tableは先頭に置くこととする。
		+ offsetof(HeadTable, checkSumAdjustment)
		;
	uint32_t *checkSumAdjustmentPointer = (uint32_t *)&(tableBuf.data[checkSumAdjustmentOffset]);
	*checkSumAdjustmentPointer = htonl(checkSumAdjustment);

	/**
	  ファイル書き出し
	  */
	int fd = open(argv[1], O_CREAT|O_TRUNC|O_RDWR, 0777);
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

