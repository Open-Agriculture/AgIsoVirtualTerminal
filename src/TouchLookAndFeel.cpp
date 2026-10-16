/*******************************************************************************
** @file       TouchLookAndFeel.cpp
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "TouchLookAndFeel.hpp"

int TouchLookAndFeel::getDefaultMenuBarHeight()
{
	return TOUCH_SCALE * LookAndFeel_V4::getDefaultMenuBarHeight();
}

void TouchLookAndFeel::getIdealPopupMenuItemSize(const String &text,
                                                 bool isSeparator,
                                                 int standardMenuItemHeight,
                                                 int &idealWidth,
                                                 int &idealHeight)
{
	LookAndFeel_V4::getIdealPopupMenuItemSize(text, isSeparator, standardMenuItemHeight, idealWidth, idealHeight);

	if (!isSeparator)
	{
		// Separators are left alone, since a thicker line does not make anything easier to hit
		idealHeight *= TOUCH_SCALE;
	}
}
