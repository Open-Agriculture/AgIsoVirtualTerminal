//================================================================================================
/// @file TouchLookAndFeel.hpp
///
/// @brief Defines a look and feel which enlarges the menu bar and its menus, so that they can be
/// operated on a touch screen. This VT is normally run on a tablet in a cab, where the stock
/// desktop sized menu bar is too small a target to hit reliably.
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#ifndef TOUCH_LOOK_AND_FEEL_HPP
#define TOUCH_LOOK_AND_FEEL_HPP

#include "JuceHeader.h"

class TouchLookAndFeel : public LookAndFeel_V4
{
public:
	TouchLookAndFeel() = default;

	/// @brief Returns the height of the menu bar. Everything else about the menu bar is derived
	/// from this by JUCE, so the font and the item widths grow along with it.
	/// @returns Twice the standard menu bar height
	int getDefaultMenuBarHeight() override;

	/// @brief Returns the size of one item of an opened menu. A taller menu bar which opens
	/// desktop sized menu items would only solve half of the problem.
	/// @param[in] text The text of the item
	/// @param[in] isSeparator True if the item is a separator
	/// @param[in] standardMenuItemHeight The height the item would normally have
	/// @param[out] idealWidth The width the item should have
	/// @param[out] idealHeight The height the item should have
	void getIdealPopupMenuItemSize(const String &text,
	                               bool isSeparator,
	                               int standardMenuItemHeight,
	                               int &idealWidth,
	                               int &idealHeight) override;

	/// @brief The factor by which the menu bar and its menus are enlarged
	static constexpr int TOUCH_SCALE = 2;

private:
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TouchLookAndFeel)
};

#endif // TOUCH_LOOK_AND_FEEL_HPP
