//================================================================================================
/// @file StringDrawingComponent.hpp
///
/// @brief Common functions for drawing strings
/// @author Miklos Marton
///
//================================================================================================
#ifndef STRINGDRAWING_COMPONENT_HPP
#define STRINGDRAWING_COMPONENT_HPP

#include "StringEncodingConversions.hpp"
#include "TextDrawingComponent.hpp"

class StringDrawingComponent : public TextDrawingComponent
{
public:
	StringDrawingComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet);

	void paintString(Graphics &g, const std::string &text, bool enabled = true);

private:
	static const std::unordered_map<isobus::FontAttributes::FontType, SourceEncoding> fontTypeToEncodingMap;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StringDrawingComponent)
};

#endif
