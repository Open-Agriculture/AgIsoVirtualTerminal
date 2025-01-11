//================================================================================================
/// @file TextDawingComponent.hpp
///
/// @brief Common functions for drawing numbers
/// @author Miklos Marton
///
//================================================================================================
#ifndef TEXTDRAWING_COMPONENT_HPP
#define TEXTDRAWING_COMPONENT_HPP

#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "JuceHeader.h"

class TextDrawingComponent : public Component
{
public:
	TextDrawingComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet);

	void setSourceObject(isobus::VTObject *newSourceObject);

protected:
	static Justification convert_justification(isobus::TextualVTObject::HorizontalJustification horizontalJustification,
	                                           isobus::TextualVTObject::VerticalJustification verticalJustification);
	uint8_t prepare_text_painting(Graphics &g,
	                              std::shared_ptr<isobus::FontAttributes> font_attributes,
	                              char referenceCharForWidthCalc);

	void paintText(Graphics &g, const std::string &text, bool enabled = true);
	std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> parentWorkingSet;
	isobus::VTObject *sourceObject;

	void drawStrikeThrough(Graphics &g, int w, int h, const String &str, isobus::TextualVTObject::HorizontalJustification justification);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TextDrawingComponent)
};

#endif // TEXTDRAWING_COMPONENT_HPP
