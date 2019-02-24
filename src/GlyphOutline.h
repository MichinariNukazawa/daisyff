/**
 * @file
  @author michianri.nukazawa@gmail.com / project daisy bell
  @details license: MIT
 */
#ifndef DAISYFF_GLYPH_OUTLINE_HPP_
#define DAISYFF_GLYPH_OUTLINE_HPP_

#include "src/Util.h"

typedef struct{
	int16_t x;
	int16_t y;
}GlyphPoint;

typedef struct{
	GlyphPoint point;
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
	//DEBUG_LOG("%p %p %zu", cpath, anchorPoints, anchorPointNum);
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

GlyphOutline GlyphOutline_Notdef()
{
	// ** //! @todo flags repeat, Coodinates SHORT_VECTOR
	GlyphOutline outline = {0};

	GlyphClosePath cpath0 = {0};
	GlyphAnchorPoint apoints0[] = {
		{{  50, 100},},
		{{ 450, 100},},
		{{ 450, 600},},
		{{  50, 600},},
	};
	GlyphClosePath_addAnchorPoints(&cpath0, apoints0, sizeof(apoints0) / sizeof(apoints0[0]));
	GlyphOutline_addClosePath(&outline, &cpath0);

	int w = 10; // line width
	GlyphClosePath cpath1 = {0};
	GlyphAnchorPoint apoints1[] = {
		{{  50 + w, 100 + w},},
		{{  50 + w, 600 - w},},
		{{ 450 - w, 600 - w},},
		{{ 450 - w, 100 + w},},
	};
	GlyphClosePath_addAnchorPoints(&cpath1, apoints1, sizeof(apoints1) / sizeof(apoints1[0]));
	GlyphOutline_addClosePath(&outline, &cpath1);

	return outline;
}

#endif // #define DAISYFF_GLYPH_OUTLINE_HPP_

