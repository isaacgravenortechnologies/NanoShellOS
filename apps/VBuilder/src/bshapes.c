/*****************************************
		NanoShell Operating System
	      (C) 2023 iProgramInCpp

   Codename V-Builder - Button renderer
******************************************/
#include <nanoshell/nanoshell.h>

/***************************************************************************
	Explanation of how this is supposed to render:

	LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
	LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLD
	LLWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWDD
	LLWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWDD
	LLWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWDD
	LLWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWDD
	LLWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWDD
	LLWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWDD
	LLWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWDD
	LDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD
	DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD
***************************************************************************/
void RenderButtonShapeNoRounding(Rectangle rect, unsigned colorDark, unsigned colorLight, unsigned colorMiddle)
{
	//draw some lines
	VidDrawVLine (colorLight, rect.top,   rect.bottom-1,   rect.left);
	VidDrawVLine (colorDark,  rect.top,   rect.bottom-1,   rect.right  - 1);
	VidDrawHLine (colorDark,  rect.left,  rect.right -1,   rect.bottom - 1);
	VidDrawHLine (colorLight, rect.left,  rect.right -1,   rect.top);
	
	//shrink
	rect.left++, rect.right--, rect.top++, rect.bottom--;
	
	//do the same
	VidDrawVLine (colorLight, rect.top,   rect.bottom-1,   rect.left);
	VidDrawVLine (colorDark,  rect.top,   rect.bottom-1,   rect.right  - 1);
	VidDrawHLine (colorDark,  rect.left,  rect.right -1,   rect.bottom - 1);
	VidDrawHLine (colorLight, rect.left,  rect.right -1,   rect.top);
	
	//shrink again
	rect.left++, rect.right -= 2, rect.top++, rect.bottom -= 2;
	
	//fill the background:
	if (colorMiddle != TRANSPARENT)
		VidFillRectangle(colorMiddle, rect);
}
void RenderButtonShapeSmall(Rectangle rectb, unsigned colorDark, unsigned colorLight, unsigned colorMiddle)
{
	rectb.bottom--;
	if (colorMiddle != TRANSPARENT)
		VidFillRectangle(colorMiddle, rectb);
	rectb.right++;
	rectb.bottom++;
	
	VidDrawHLine(WINDOW_TEXT_COLOR, rectb.left, rectb.right-1,  rectb.bottom-1);
	VidDrawVLine(WINDOW_TEXT_COLOR, rectb.top,  rectb.bottom-1, rectb.right-1);
	
	VidDrawHLine(colorLight, rectb.left, rectb.right-1,  rectb.top);
	VidDrawVLine(colorLight, rectb.top,  rectb.bottom-1, rectb.left);
	
	rectb.left++;
	rectb.top++;
	rectb.right--;
	rectb.bottom--;
	
	VidDrawHLine(colorDark, rectb.left, rectb.right-1,  rectb.bottom-1);
	VidDrawVLine(colorDark, rectb.top,  rectb.bottom-1, rectb.right-1);
	
	int colorAvg = 0;
	colorAvg |= ((colorLight & 0xff0000) + (colorMiddle & 0xff0000)) >> 1;
	colorAvg |= ((colorLight & 0x00ff00) + (colorMiddle & 0x00ff00)) >> 1;
	colorAvg |= ((colorLight & 0x0000ff) + (colorMiddle & 0x0000ff)) >> 1;
	VidDrawHLine(colorAvg, rectb.left, rectb.right-1,  rectb.top);
	VidDrawVLine(colorAvg, rectb.top,  rectb.bottom-1, rectb.left);
}
void RenderButtonShapeSmallInsideOut(Rectangle rectb, unsigned colorLight, unsigned colorDark, unsigned colorMiddle)
{
	rectb.bottom--;
	if (colorMiddle != TRANSPARENT)
		VidFillRectangle(colorMiddle, rectb);
	rectb.right++;
	rectb.bottom++;
	
	VidDrawHLine(WINDOW_TEXT_COLOR_LIGHT, rectb.left, rectb.right-1,  rectb.bottom-1);
	VidDrawVLine(WINDOW_TEXT_COLOR_LIGHT, rectb.top,  rectb.bottom-1, rectb.right-1);
	
	VidDrawHLine(colorDark, rectb.left, rectb.right-1,  rectb.top);
	VidDrawVLine(colorDark, rectb.top,  rectb.bottom-1, rectb.left);
	
	rectb.left  ++;
	rectb.top   ++;
	rectb.right --;
	rectb.bottom--;
	
	VidDrawHLine(colorLight, rectb.left, rectb.right-1,  rectb.bottom-1);
	VidDrawVLine(colorLight, rectb.top,  rectb.bottom-1, rectb.right-1);
	
	int colorAvg = 0;
	colorAvg |= (colorDark & 0xff0000) >> 1;
	colorAvg |= (colorDark & 0x00ff00) >> 1;
	colorAvg |= (colorDark & 0x0000ff) >> 1;
	VidDrawHLine(colorAvg, rectb.left, rectb.right-1,  rectb.top);
	VidDrawVLine(colorAvg, rectb.top,  rectb.bottom-1, rectb.left);
}
void RenderButtonShape(Rectangle rect, unsigned colorDark, unsigned colorLight, unsigned colorMiddle)
{
	//draw some lines
	VidDrawHLine (WINDOW_TEXT_COLOR, rect.left,rect.right-1,  rect.top);
	VidDrawHLine (WINDOW_TEXT_COLOR, rect.left,rect.right-1,  rect.bottom-1);
	VidDrawVLine (WINDOW_TEXT_COLOR, rect.top, rect.bottom-1, rect.left);
	VidDrawVLine (WINDOW_TEXT_COLOR, rect.top, rect.bottom-1, rect.right-1);
	
	rect.left++, rect.right--, rect.top++, rect.bottom--;
	RenderButtonShapeNoRounding(rect, colorDark, colorLight, colorMiddle);
	//RenderButtonShapeSmall(rect, colorDark, colorLight, colorMiddle);
}
