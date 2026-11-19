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
			if (font->get_style(isobus::FontAttributes::FontStyleBits::FlashingHidden) && !show)
			{
				return;
			}
			auto colour = parentWorkingSet->get_colour(font->get_colour());
			drawColour = Colour::fromFloatRGBA(colour.r, colour.g, colour.b, 1.0f);
			fontHeight = prepare_text_painting(g, font, 'a', drawColour, backgroundColour);
			strikeThrough = font->get_style(isobus::FontAttributes::FontStyleBits::CrossedOut);
			fontType = font->get_type();
		}
	}

	std::string value = text;
	String decodedValue(value);

	if ((value.length() >= 2) &&
	    (0xFF == static_cast<std::uint8_t>(value.at(0))) &&
	    (0xFE == static_cast<std::uint8_t>(value.at(1))))
	{
		std::size_t i = 0;
		for (; i + 1 < value.size(); i += 2)
		{
			if (value[i] == '\0' && value[i + 1] == '\0')
			{
				break;
			}
		}
		value = value.substr(0, i);
		decodedValue = String::createStringFromData(value.c_str(), value.size());
	}
	else
	{
		value = value.substr(0, value.find('\0'));
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
	// Popups such as Input List resize this component, so clip to the object itself
	g.reduceClipRegion(0, 0, sourceString->get_width(), sourceString->get_height());
	g.setColour(drawColour);

	// JUCE Graphics::drawFittedText will throw an exception if the font's horizontal width multiplied by
	// the texts minimumHorizontalScale is larger or equal to 1.0
	float drawTextHorizontalScale = 0.8f;
	if ((drawTextHorizontalScale * g.getCurrentFont().getHorizontalScale()) > 1.0f)
	{
		drawTextHorizontalScale = 0.999999f / g.getCurrentFont().getHorizontalScale();
	}

	const auto horizontalJustification = sourceString->get_horizontal_justification();

	if (sourceString->get_option(isobus::StringVTObject::Options::AutoWrap))
	{
		g.drawFittedText(decodedValue, 0, 0, sourceString->get_width(), sourceString->get_height(), convert_justification(sourceString->get_horizontal_justification(), sourceString->get_vertical_justification()), static_cast<int>(std::floor((static_cast<float>(sourceString->get_height()) + 0.1f) / fontHeight)), drawTextHorizontalScale);
	}
	else
	{
		std::vector<String> lines;
		// The standard ends a line at CR, LF or CRLF, so LFCR is two. A whole CRLF for
		// U+2028/2029 keeps them from pairing with a CR or LF next to them.
		const auto paragraphs = StringArray::fromLines(decodedValue.replace(String::charToString(0x2028), "\r\n").replace(String::charToString(0x2029), "\r\n"));

		for (int i = 0; i < paragraphs.size(); i++)
		{
			// A line end after the last line starts no new line
			if (paragraphs[i].isEmpty() && ((i + 1) == paragraphs.size()) && (i > 0))
			{
				break;
			}

			// The standard never counts trailing spaces, and middle justification ignores leading ones too
			const String trimmed = paragraphs[i].trimCharactersAtEnd(" ");
			lines.push_back((isobus::TextualVTObject::HorizontalJustification::PositionMiddle == horizontalJustification) ? trimmed.trimCharactersAtStart(" ") : trimmed);
		}
		draw_lines(g, lines, sourceString->get_width(), sourceString->get_height(), horizontalJustification, sourceString->get_vertical_justification());
	}

	if (strikeThrough)
	{
		// Juce does not support Strikethrough text drawing, draw the line manually
		drawStrikeThrough(g, sourceString->get_width(), sourceString->get_height(), decodedValue, horizontalJustification);
	}

	// If disabled, try and show that by drawing some semi-transparent grey
	if (!enabled)
	{
		g.fillAll(Colour::fromFloatRGBA(0.5f, 0.5f, 0.5f, 0.5f));
	}
}

void StringDrawingComponent::draw_lines(Graphics &g,
                                        const std::vector<String> &lines,
                                        int width,
                                        int height,
                                        isobus::TextualVTObject::HorizontalJustification horizontalJustification,
                                        isobus::TextualVTObject::VerticalJustification verticalJustification)
{
	// Full font size, no ellipsis: the standard clips what doesn't fit by pixel
	const Font font = g.getCurrentFont();
	const float lineHeight = font.getHeight();
	const float blockHeight = lineHeight * static_cast<float>(lines.size());
	float y = 0.0f;

	if (isobus::TextualVTObject::VerticalJustification::PositionMiddle == verticalJustification)
	{
		y = (static_cast<float>(height) - blockHeight) / 2.0f;
	}
	else if (isobus::TextualVTObject::VerticalJustification::PositionBottom == verticalJustification)
	{
		y = static_cast<float>(height) - blockHeight;
	}

	GlyphArrangement glyphs;
	for (const auto &line : lines)
	{
		const float lineWidth = GlyphArrangement::getStringWidth(font, line);
		float x = 0.0f;

		if (isobus::TextualVTObject::HorizontalJustification::PositionMiddle == horizontalJustification)
		{
			x = (static_cast<float>(width) - lineWidth) / 2.0f;
		}
		else if (isobus::TextualVTObject::HorizontalJustification::PositionRight == horizontalJustification)
		{
			x = static_cast<float>(width) - lineWidth;
		}
		glyphs.addLineOfText(font, line, x, y + font.getAscent());
		y += lineHeight;
	}
	glyphs.draw(g);
}
