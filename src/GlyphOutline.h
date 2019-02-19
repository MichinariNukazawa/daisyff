/**
 * @file
  @author michianri.nukazawa@gmail.com / project daisy bell
  @details license: MIT
 */
#ifndef DAISYFF_GLYPH_OUTLINE_HPP_
#define DAISYFF_GLYPH_OUTLINE_HPP_

#include "src/OpenType.h"

enum GlyphDiscriptionFlag_Bit{
	GlyphDiscriptionFlag_Bit0_ON_CURVE_POINT		= 0x01,
	GlyphDiscriptionFlag_Bit1_X_SHORT_VECTOR		= 0x02,
	GlyphDiscriptionFlag_Bit2_Y_SHORT_VECTOR		= 0x04,
	GlyphDiscriptionFlag_Bit3_REPEAT_FLAG			= 0x08,
	GlyphDiscriptionFlag_Bit4_X_IS_SAME_OR_POSITIVE		= 0x10,
	GlyphDiscriptionFlag_Bit5_Y_IS_SAME_OR_POSITIVE		= 0x20,
};

typedef struct{
	int16_t		numberOfContours;
	uint16_t	*endPoints;
	uint8_t		*flags;
	uint16_t	instructionLength;
	uint8_t		*instructions;
	uint8_t		*xCoodinates;
	uint8_t		*yCoodinates;
	//
	size_t		pointNum;
}GlyphDiscriptionBuf;

typedef struct{
	int16_t x;
	int16_t y;
}Point;

typedef struct{
	Point point;
}GlyphAnchorPoint;

typedef struct{
	GlyphAnchorPoint	*anchorPoints;
	size_t			anchorPointNum;
}GlyphClosePath;

typedef struct{
	GlyphClosePath		*closePaths;
	size_t			closePathNum;
}GlyphOutline;

void GlyphClosePath_addAnchorPoints(GlyphClosePath *cpath, const GlyphAnchorPoint *anchorPoints, size_t anchorPointNum)
{
	cpath->anchorPoints = realloc(cpath->anchorPoints, sizeof(GlyphAnchorPoint) * (cpath->anchorPointNum + anchorPointNum));
	ASSERT(cpath->anchorPoints);

	memcpy(&(cpath->anchorPoints[cpath->anchorPointNum]), anchorPoints, sizeof(GlyphAnchorPoint) * anchorPointNum);
	cpath->anchorPointNum += anchorPointNum;
}

void GlyphOutline_addClosePath(GlyphOutline *outline, const GlyphClosePath *cpath)
{
	outline->closePaths = realloc(outline->closePaths, sizeof(GlyphClosePath) * (outline->closePathNum + 1));
	ASSERT(outline->closePaths);

	memcpy(&(outline->closePaths[outline->closePathNum]), cpath, sizeof(GlyphClosePath) * 1);
	(outline->closePathNum) += 1;
}

void GlyphDiscriptionBuf_setOutline(GlyphDiscriptionBuf *gdb, const GlyphOutline *outline)
{
	// ** pointNumカウントとEndPoints収集を行う
	ASSERT(0 < outline->closePathNum);
	gdb->numberOfContours = outline->closePathNum;
	gdb->endPoints = malloc(sizeof(uint16_t) * gdb->numberOfContours);
	gdb->pointNum = 0;
	for(int l = 0; l < outline->closePathNum; l++){
		const GlyphClosePath *closePath = &(outline->closePaths[l]);
		ASSERT(0 < closePath->anchorPointNum);
		gdb->pointNum += closePath->anchorPointNum;
		gdb->endPoints[l] = gdb->pointNum;
	}

	// ** buffer確保
	// repeatによる短縮はこの段階では行わず最大長を取る。
	gdb->flags		= malloc(sizeof(uint8_t) * gdb->pointNum);
	// Instructionは現在のdaisyffの実装では使用しない。
	gdb->instructionLength	= 0;
	gdb->instructions	= NULL;
	// SHORT_VECTORによる短縮はこの段階では行わず最大長を取る。
	gdb->xCoodinates	= malloc(sizeof(int16_t) * gdb->pointNum);
	gdb->yCoodinates	= malloc(sizeof(int16_t) * gdb->pointNum);

	// ** flags, coodinatesをベタ展開
	int pointcount = 0;
	for(int l = 0; l < outline->closePathNum; l++){
		const GlyphClosePath *closePath = &(outline->closePaths[l]);
		for(int a = 0; a < closePath->anchorPointNum; a++){
			const GlyphAnchorPoint *anchorPoint = &(closePath->anchorPoints[a]);

			uint8_t flag = (GlyphDiscriptionFlag_Bit0_ON_CURVE_POINT);
			int16_t *xCoodinates = (int16_t *)(gdb->xCoodinates);
			int16_t *yCoodinates = (int16_t *)(gdb->yCoodinates);

			gdb->flags [pointcount] = flag;
			xCoodinates[pointcount] = anchorPoint->point.x;
			yCoodinates[pointcount] = anchorPoint->point.y;

			pointcount++;
		}
	}

	// ** //! @todo flags repeat, Coodinates SHORT_VECTOR
}

#endif // #define DAISYFF_GLYPH_OUTLINE_HPP_

