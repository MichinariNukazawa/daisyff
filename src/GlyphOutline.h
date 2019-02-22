/**
 * @file
  @author michianri.nukazawa@gmail.com / project daisy bell
  @details license: MIT
 */
#ifndef DAISYFF_GLYPH_OUTLINE_HPP_
#define DAISYFF_GLYPH_OUTLINE_HPP_

#include "src/OpenType.h"

enum GlyphDescriptionFlag_Bit{
	GlyphDescriptionFlag_Bit0_ON_CURVE_POINT		= 0x01,
	GlyphDescriptionFlag_Bit1_X_SHORT_VECTOR		= 0x02,
	GlyphDescriptionFlag_Bit2_Y_SHORT_VECTOR		= 0x04,
	GlyphDescriptionFlag_Bit3_REPEAT_FLAG			= 0x08,
	GlyphDescriptionFlag_Bit4_X_IS_SAME_OR_POSITIVE		= 0x10,
	GlyphDescriptionFlag_Bit5_Y_IS_SAME_OR_POSITIVE		= 0x20,
};

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
	cpath->anchorPoints = ffrealloc(cpath->anchorPoints, sizeof(GlyphAnchorPoint) * (cpath->anchorPointNum + anchorPointNum));

	memcpy(&(cpath->anchorPoints[cpath->anchorPointNum]), anchorPoints, sizeof(GlyphAnchorPoint) * anchorPointNum);
	cpath->anchorPointNum += anchorPointNum;
}

void GlyphOutline_addClosePath(GlyphOutline *outline, const GlyphClosePath *cpath)
{
	outline->closePaths = ffrealloc(outline->closePaths, sizeof(GlyphClosePath) * (outline->closePathNum + 1));

	memcpy(&(outline->closePaths[outline->closePathNum]), cpath, sizeof(GlyphClosePath) * 1);
	(outline->closePathNum) += 1;
}

void GlyphDescriptionBuf_setOutline(GlyphDescriptionBuf *gdb, const GlyphOutline *outline)
{
	// ** pointNumカウントとEndPoints収集を行う
	ASSERT(0 < outline->closePathNum);
	gdb->numberOfContours = outline->closePathNum;
	gdb->endPoints = ffmalloc(sizeof(uint16_t) * gdb->numberOfContours);
	gdb->pointNum = 0;
	for(int l = 0; l < outline->closePathNum; l++){
		const GlyphClosePath *closePath = &(outline->closePaths[l]);
		ASSERT(0 < closePath->anchorPointNum);
		gdb->pointNum += closePath->anchorPointNum;
		gdb->endPoints[l] = gdb->pointNum;
	}

	// ** buffer確保
	// repeatによる短縮はこの段階では行わず最大長を取る。
	gdb->flags		= ffmalloc(sizeof(uint8_t) * gdb->pointNum);
	// Instructionは現在のdaisyffの実装では使用しない。
	gdb->instructionLength	= 0;
	gdb->instructions	= NULL;
	// SHORT_VECTORによる短縮はこの段階では行わず最大長を取る。
	gdb->xCoodinates	= ffmalloc(sizeof(int16_t) * gdb->pointNum);
	gdb->yCoodinates	= ffmalloc(sizeof(int16_t) * gdb->pointNum);

	// ** flags, coodinatesをベタ展開
	int pointcount = 0;
	for(int l = 0; l < outline->closePathNum; l++){
		const GlyphClosePath *closePath = &(outline->closePaths[l]);
		for(int a = 0; a < closePath->anchorPointNum; a++){
			const GlyphAnchorPoint *anchorPoint = &(closePath->anchorPoints[a]);

			uint8_t flag = (GlyphDescriptionFlag_Bit0_ON_CURVE_POINT);
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
