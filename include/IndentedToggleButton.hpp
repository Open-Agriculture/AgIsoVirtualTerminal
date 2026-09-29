//================================================================================================
/// @file       IndentedToggleButton.hpp
///
/// @brief 		A toggle button that indents itself to line up with an AlertWindow's icon
/// @author     Sujan Dumaru
///
/// @copyright  The Open-Agriculture Developers
//================================================================================================

#ifndef INDENTED_TOGGLE_BUTTON_HPP
#define INDENTED_TOGGLE_BUTTON_HPP

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

#endif // INDENTED_TOGGLE_BUTTON_HPP
