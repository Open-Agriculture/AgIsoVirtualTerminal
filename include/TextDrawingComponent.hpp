//================================================================================================
/// @file TextDrawingComponent.hpp
///
/// @brief Common functions for drawing numbers
/// @author Miklos Marton
///
/// @copyright 2024 The Open-Agriculture Developers
//================================================================================================

#ifndef TEXTDRAWING_COMPONENT_HPP
#define TEXTDRAWING_COMPONENT_HPP

#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "JuceHeader.h"

class TextDrawingComponent : public Component
  , private juce::Timer
{
public:
	TextDrawingComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet);

	/// @brief Sets whether characters missing from the font are drawn with a system fallback font (the default).
	/// Turning it off keeps text independent of the fonts installed on the machine.
	static void set_font_fallback_enabled(bool enabled);

protected:
	virtual const isobus::VTObject *vtObject() const = 0;

	static Justification convert_justification(isobus::TextualVTObject::HorizontalJustification horizontalJustification,
	                                           isobus::TextualVTObject::VerticalJustification verticalJustification);
	std::uint8_t prepare_text_painting(Graphics &g,
	                                   std::shared_ptr<isobus::FontAttributes> font_attributes,
	                                   char referenceCharForWidthCalc,
	                                   Colour &drawColour,
	                                   Colour &backgroundColor);

	std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> parentWorkingSet;

	void drawStrikeThrough(Graphics &g, int w, int h, const String &str, isobus::TextualVTObject::HorizontalJustification justification);

	void visibilityChanged() override;

	void timerCallback() override;

	/**
   * @brief isFlashing
   * @return true if the TextDrawingComponent's font attribute has any of the flashing mode set, false otherwise
   */
	bool isFlashing() const;
	bool visible = false;
	/**
   * @brief show - a boolean idicating that the TextDrawingComponent needs to be drawn based on the flashing state
   */
	bool show = true;

private:
	static bool fontFallbackEnabled;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TextDrawingComponent)
};

#endif // TEXTDRAWING_COMPONENT_HPP
