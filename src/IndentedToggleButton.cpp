/*******************************************************************************
** @file       IndentedToggleButton.cpp
** @author     Sujan Dumaru
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "IndentedToggleButton.hpp"

#include <cmath>

IndentedToggleButton::IndentedToggleButton()
{
	addAndMakeVisible(toggle);
}

void IndentedToggleButton::moved()
{
	layout_toggle();
}

void IndentedToggleButton::resized()
{
	layout_toggle();
}

void IndentedToggleButton::layout_toggle()
{
	int tickLeft = 90;
	if (auto *alert = getParentComponent())
	{
		const int iconSize = juce::jmin(130, alert->getHeight() + 18);
		const float centre = static_cast<float>(iconSize / -10 + iconSize / 2);
		const float radius = iconSize * 0.5f;
		const float y = static_cast<float>(juce::jlimit(getY() + 4, getY() + getHeight() - 4, static_cast<int>(centre)));
		const float dy = y - centre;
		const float circleRight = (std::abs(dy) < radius) ? centre + std::sqrt(radius * radius - dy * dy) : centre;
		tickLeft = static_cast<int>(std::ceil(circleRight)) + 6;
	}

	const int tickInset = 4; // LookAndFeel_V4 draws the tick box this far inside the button
	const int indent = juce::jmax(0, tickLeft - getX() - tickInset);
	toggle.setBounds(indent, 0, juce::jmax(0, getWidth() - indent), getHeight());
}
