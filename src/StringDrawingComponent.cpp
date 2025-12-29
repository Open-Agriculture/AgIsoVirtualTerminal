/*******************************************************************************
** @file       StringDrawingComponent.cpp
** @author     Miklos Marton
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "StringDrawingComponent.hpp"

const std::unordered_map<isobus::FontAttributes::FontType, SourceEncoding> StringDrawingComponent::fontTypeToEncodingMap = {
	{ isobus::FontAttributes::FontType::ISO8859_1, SourceEncoding::ISO8859_1 },
	{ isobus::FontAttributes::FontType::ISO8859_15, SourceEncoding::ISO8859_15 },
	{ isobus::FontAttributes::FontType::ISO8859_2, SourceEncoding::ISO8859_2 },
	{ isobus::FontAttributes::FontType::ISO8859_4, SourceEncoding::ISO8859_4 },
	{ isobus::FontAttributes::FontType::ISO8859_5, SourceEncoding::ISO8859_5 },
	{ isobus::FontAttributes::FontType::ISO8859_7, SourceEncoding::ISO8859_7 },
};

StringDrawingComponent::StringDrawingComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet) :
  TextDrawingComponent(workingSet)
{
}

void StringDrawingComponent::paintString(Graphics &g, const std::string &text, bool enabled)
{
	bool strikeThrough = false;
	auto vtColour = parentWorkingSet->get_colour(vtObject()->get_background_color());
	juce::Colour backgroundColour = Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f);
	juce::Colour drawColour = getLookAndFeel().findColour(ListBox::textColourId);

	g.setColour(getLookAndFeel().findColour(ListBox::textColourId));

	auto sourceString = static_cast<const isobus::StringVTObject *>(vtObject());

	std::uint8_t fontHeight = 8;
	auto fontType = isobus::FontAttributes::FontType::ISO8859_1;
	auto fontAttrID = sourceString->get_font_attributes();

	// Get font data
	if (isobus::NULL_OBJECT_ID != fontAttrID)
	{
		auto child = parentWorkingSet->get_object_by_id(fontAttrID);

		if (child != nullptr && isobus::VirtualTerminalObjectType::FontAttributes == child->get_object_type())
		{
			auto font = std::static_pointer_cast<isobus::FontAttributes>(child);
			// Bit 6 (Flashing) has priority over bit 5 (FlashingHidden)
			if (font->get_style(isobus::FontAttributes::FontStyleBits::FlashingHidden) && !show && !font->get_style(isobus::FontAttributes::FontStyleBits::Flashing))
			{
				return;
			}
			fontHeight = prepare_text_painting(g, font, 'a', drawColour, backgroundColour);
			strikeThrough = font->get_style(isobus::FontAttributes::FontStyleBits::CrossedOut);
			auto colour = parentWorkingSet->get_colour(font->get_colour());
			drawColour = Colour::fromFloatRGBA(colour.r, colour.g, colour.b, 1.0f);
			fontType = font->get_type();
		}
	}

	std::string value = text;
	String decodedValue(value);

	if ((value.length() >= 2) &&
	    (0xFF == static_cast<std::uint8_t>(value.at(0))) &&
	    (0xFE == static_cast<std::uint8_t>(value.at(1))))
	{
		// String is UTF-16 encoded, font type is ignored.
		if (0 != (value.length() % 2))
		{
			// If the length attribute does not indicate an even number of bytes the last byte is ignored
			value.pop_back();
		}
		decodedValue = String::createStringFromData(value.c_str(), value.size());
	}
	else
	{
		auto it = fontTypeToEncodingMap.find(fontType);
		if (it != fontTypeToEncodingMap.end())
		{
			std::string utf8String;
			convert_string_to_utf_8(it->second, value, utf8String, sourceString->get_option(isobus::StringVTObject::Options::AutoWrap));
			decodedValue = utf8String;
		}
	}

	if (!sourceString->get_option(isobus::StringVTObject::Options::Transparent))
	{
		g.fillAll(backgroundColour);
	}
	g.setColour(drawColour);

	if (sourceString->get_option(isobus::StringVTObject::Options::AutoWrap)) // TODO need to figure out proper font clipping
	{
		g.drawFittedText(decodedValue, 0, 0, sourceString->get_width(), sourceString->get_height(), convert_justification(sourceString->get_horizontal_justification(), sourceString->get_vertical_justification()), static_cast<int>(std::floor((static_cast<float>(sourceString->get_height()) + 0.1f) / fontHeight)), 0.8f);
	}
	else
	{
		g.drawFittedText(decodedValue, 0, 0, sourceString->get_width(), sourceString->get_height(), convert_justification(sourceString->get_horizontal_justification(), sourceString->get_vertical_justification()), static_cast<int>(std::floor((static_cast<float>(sourceString->get_height()) + 0.1f) / fontHeight)), 0.8f);
	}

	if (strikeThrough)
	{
		// Juce does not support Strikethrough text drawing, draw the line manually
		drawStrikeThrough(g, sourceString->get_width(), sourceString->get_height(), decodedValue, sourceString->get_horizontal_justification());
	}

	// If disabled, try and show that by drawing some semi-transparent grey
	if (!enabled)
	{
		g.fillAll(Colour::fromFloatRGBA(0.5f, 0.5f, 0.5f, 0.5f));
	}
}
