//================================================================================================
/// @file CollapsedButton.hpp
///
/// @brief The small button that stands in for the VT window while it is collapsed
/// @author The Open-Agriculture Developers
///
/// @copyright The Open-Agriculture Developers
//================================================================================================

#ifndef COLLAPSED_BUTTON_HPP
#define COLLAPSED_BUTTON_HPP

#include "JuceHeader.h"

class ServerMainComponent;

/// @brief Restores the VT window when tapped, and can be dragged out of the way
class CollapsedButton : public juce::Component
{
public:
	static constexpr int SIZE = 64; ///< Large enough to tap with a finger

	explicit CollapsedButton(ServerMainComponent &server);

	void paint(juce::Graphics &g) override;
	void mouseDown(const juce::MouseEvent &event) override;
	void mouseDrag(const juce::MouseEvent &event) override;
	void mouseUp(const juce::MouseEvent &event) override;

private:
	static constexpr int DRAG_THRESHOLD = 10; ///< A finger moves a little during a tap

	ServerMainComponent &parentServer;
	juce::ComponentDragger dragger;
	juce::Image icon;
	bool dragged = false;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CollapsedButton)
};

#endif // COLLAPSED_BUTTON_HPP
