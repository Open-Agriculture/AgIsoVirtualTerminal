//================================================================================================
/// @file StringDrawingComponent.hpp
///
/// @brief Common functions for drawing strings
/// @author Miklos Marton
///
/// @copyright 2024 The Open-Agriculture Developers
//================================================================================================

#ifndef STRINGDRAWING_COMPONENT_HPP
#define STRINGDRAWING_COMPONENT_HPP

#include "StringEncodingConversions.hpp"
#include "TextDrawingComponent.hpp"

#include <vector>

class StringDrawingComponent : public TextDrawingComponent
{
public:
	StringDrawingComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet);

	void paintString(Graphics &g, const std::string &text, bool enabled = true);

private:
	static void draw_lines(Graphics &g,
	                       const std::vector<String> &lines,
	                       int width,
	                       int height,
	                       isobus::TextualVTObject::HorizontalJustification horizontalJustification,
	                       isobus::TextualVTObject::VerticalJustification verticalJustification);
	static const std::unordered_map<isobus::FontAttributes::FontType, SourceEncoding> fontTypeToEncodingMap;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StringDrawingComponent)
};

#endif
