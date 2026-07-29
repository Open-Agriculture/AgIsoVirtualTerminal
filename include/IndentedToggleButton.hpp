//================================================================================================
/// @file       IndentedToggleButton.hpp
///
/// @brief 		A toggle button that indents itself to line up with an AlertWindow's icon
/// @author     Sujan Dumaru
///
/// @copyright  The Open-Agriculture Developers
//================================================================================================

#pragma once

#include "JuceHeader.h"

class IndentedToggleButton : public juce::Component
{
public:
	IndentedToggleButton();

	juce::ToggleButton toggle;

private:
	void moved() override;
	void resized() override;
	void layout_toggle();
};
