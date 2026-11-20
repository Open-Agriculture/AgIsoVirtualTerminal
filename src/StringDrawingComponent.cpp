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
			prepare_text_painting(g, font, 'a', drawColour, backgroundColour);
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
			convert_string_to_utf_8(it->second, value, utf8String);
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

	const auto horizontalJustification = sourceString->get_horizontal_justification();
	const bool autoWrap = sourceString->get_option(isobus::StringVTObject::Options::AutoWrap);
	const String softHyphen = String::charToString(SOFT_HYPHEN);
	// The standard ends a line at CR, LF or CRLF, so LFCR is two. A whole CRLF for
	// U+2028/2029 keeps them from pairing with a CR or LF next to them.
	const auto paragraphs = StringArray::fromLines(decodedValue.replace(String::charToString(0x2028), "\r\n").replace(String::charToString(0x2029), "\r\n"));
	std::vector<String> lines;

	for (int i = 0; i < paragraphs.size(); i++)
	{
		String line = paragraphs[i];
		const bool forcedLineEnd = (i + 1) < paragraphs.size();

		// A line end after the last line starts no new line, but wrap_line still needs to know it was there
		if (line.removeCharacters(softHyphen).isEmpty() && !forcedLineEnd && (i > 0))
		{
			break;
		}

		// The standard has middle justification ignore leading spaces (and invisible soft hyphens)
		if (isobus::TextualVTObject::HorizontalJustification::PositionMiddle == horizontalJustification)
		{
			line = line.trimCharactersAtStart(" " + softHyphen);
		}

		if (autoWrap)
		{
			const auto wrapped = wrap_line(line, g.getCurrentFont(), static_cast<float>(sourceString->get_width()), sourceString->get_option(isobus::StringVTObject::Options::WrapOnHyphen), forcedLineEnd);
			lines.insert(lines.end(), wrapped.begin(), wrapped.end());
		}
		else
		{
			// The standard never counts trailing spaces
			lines.push_back(line.removeCharacters(softHyphen).trimCharactersAtEnd(" "));
		}
	}
	draw_lines(g, lines, sourceString->get_width(), sourceString->get_height(), horizontalJustification, sourceString->get_vertical_justification());

	if (strikeThrough)
	{
		// Juce does not support Strikethrough text drawing, draw the line manually
		drawStrikeThrough(g, sourceString->get_width(), sourceString->get_height(), decodedValue.removeCharacters(softHyphen), horizontalJustification);
	}

	// If disabled, try and show that by drawing some semi-transparent grey
	if (!enabled)
	{
		g.fillAll(Colour::fromFloatRGBA(0.5f, 0.5f, 0.5f, 0.5f));
	}
}

std::vector<String> StringDrawingComponent::wrap_line(const String &line, const Font &font, float width, bool wrapOnHyphen, bool forcedLineEnd)
{
	// The standard breaks at a space, a soft hyphen (drawn only there) or, with Wrap on Hyphen, after a hyphen,
	// else after the last character that fits. Trailing spaces aren't drawn but still take room
	const String softHyphen = String::charToString(SOFT_HYPHEN);
	// JUCE measures an 8 px cell as 8.000015 px, and a real overflow is a whole cell
	const auto fits = [&font, width](const String &text) {
		return GlyphArrangement::getStringWidth(font, text) <= (width + 0.5f);
	};
	std::vector<String> lines;
	String remaining = line.trimCharactersAtStart(softHyphen);

	while (true)
	{
		const String visible = remaining.removeCharacters(softHyphen);

		if (fits(visible))
		{
			lines.push_back(visible.trimCharactersAtEnd(" "));
			break;
		}

		String bestLine;
		int bestNext = -1;

		for (int i = 0; i < remaining.length(); i++)
		{
			const juce_wchar character = remaining[i];
			const bool breaksAfter = wrapOnHyphen && ('-' == character);

			if ((' ' != character) && (SOFT_HYPHEN != character) && !breaksAfter)
			{
				continue;
			}

			String candidate = remaining.substring(0, breaksAfter ? (i + 1) : i).removeCharacters(softHyphen);

			if (!fits(candidate))
			{
				break;
			}
			if (SOFT_HYPHEN == character)
			{
				// Skip it if nothing precedes it or only its dash overflows; a later break may still fit
				if (candidate.trimCharactersAtEnd(" ").isEmpty() || !fits(candidate + "-"))
				{
					continue;
				}
				candidate += "-";
			}
			candidate = candidate.trimCharactersAtEnd(" ");

			if (candidate.isNotEmpty())
			{
				bestLine = candidate;
				bestNext = i + 1;
			}
		}

		if (bestNext < 0)
		{
			int count = 1;
			while ((count < remaining.length()) && fits(remaining.substring(0, count + 1).removeCharacters(softHyphen)))
			{
				count++;
			}
			bestLine = remaining.substring(0, count).removeCharacters(softHyphen);
			bestNext = count;
		}

		lines.push_back(bestLine);
		remaining = remaining.substring(bestNext);
		const String next = remaining.trimCharactersAtStart(" " + softHyphen);

		if (next.isEmpty())
		{
			// The standard draws spaces left between the wrap and a forced line end as an empty line
			if (forcedLineEnd && remaining.containsChar(' '))
			{
				lines.push_back(String());
			}
			break;
		}
		remaining = next;
	}
	return lines;
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
