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

		// fromLines gives an empty last line after a final line end, and the standard starts no new line there
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

std::vector<float> StringDrawingComponent::character_offsets(const String &line, const Font &font, std::unordered_map<juce_wchar, float> &characterWidths)
{
	// JUCE shapes the text on every measurement, which is slow for long strings.
	// The VT font is monospaced, so the widths of single characters add up the same.
	// JUCE strings are UTF-8, so indexing one walks from its start each time
	const auto characters = line.toUTF32();
	const int length = line.length();
	std::vector<float> offsets(static_cast<std::size_t>(length) + 1, 0.0f);

	for (int i = 0; i < length; i++)
	{
		const juce_wchar character = characters[i];
		float characterWidth = 0.0f;

		if (SOFT_HYPHEN != character)
		{
			auto cached = characterWidths.find(character);
			if (characterWidths.end() == cached)
			{
				cached = characterWidths.emplace(character, GlyphArrangement::getStringWidth(font, String::charToString(character))).first;
			}
			characterWidth = cached->second;
		}
		offsets[static_cast<std::size_t>(i) + 1] = offsets[static_cast<std::size_t>(i)] + characterWidth;
	}
	return offsets;
}

std::vector<String> StringDrawingComponent::wrap_line(const String &line, const Font &font, float width, bool wrapOnHyphen, bool forcedLineEnd)
{
	// The standard breaks at a space, a soft hyphen (drawn only there) or, with Wrap on Hyphen, after a hyphen,
	// else after the last character that fits. Trailing spaces aren't drawn but still take room
	const String softHyphen = String::charToString(SOFT_HYPHEN);
	std::unordered_map<juce_wchar, float> characterWidths;
	const std::vector<float> offsets = character_offsets(line, font, characterWidths);
	const auto characters = line.toUTF32();
	const float hyphenWidth = GlyphArrangement::getStringWidth(font, "-");
	// JUCE measures an 8 px cell as 8.000015 px, and a real overflow is a whole cell
	const auto fits = [&offsets, width](int start, int end, float extra = 0.0f) {
		return (offsets[static_cast<std::size_t>(end)] - offsets[static_cast<std::size_t>(start)] + extra) <= (width + 0.5f);
	};
	std::vector<String> lines;
	const int end = line.length();
	int start = 0;

	while ((start < end) && (SOFT_HYPHEN == characters[start]))
	{
		start++;
	}

	while (true)
	{
		if (fits(start, end))
		{
			lines.push_back(String(characters + start, characters + end).removeCharacters(softHyphen).trimCharactersAtEnd(" "));
			break;
		}

		int bestEnd = -1;
		int bestNext = -1;
		bool bestHyphen = false;
		bool hasText = false;

		for (int i = start; i < end; i++)
		{
			// Any break here ends at i or i + 1, so once the text up to i overflows, no later break fits
			if (!fits(start, i))
			{
				break;
			}

			const juce_wchar character = characters[i];
			const bool breaksAfter = wrapOnHyphen && ('-' == character);
			hasText = hasText || ((' ' != character) && (SOFT_HYPHEN != character));

			if ((' ' != character) && (SOFT_HYPHEN != character) && !breaksAfter)
			{
				continue;
			}

			const int candidateEnd = breaksAfter ? (i + 1) : i;

			if (!fits(start, candidateEnd))
			{
				break;
			}

			// A soft hyphen whose dash overflows doesn't end the scan, as a later space may still fit.
			// Only the break is kept here: building each candidate's text is quadratic in a run of soft hyphens
			if (hasText && ((SOFT_HYPHEN != character) || fits(start, candidateEnd, hyphenWidth)))
			{
				bestEnd = candidateEnd;
				bestNext = i + 1;
				bestHyphen = (SOFT_HYPHEN == character);
			}
		}

		if (bestNext < 0)
		{
			int count = 1;
			while (((start + count) < end) && fits(start, start + count + 1))
			{
				count++;
			}
			lines.push_back(String(characters + start, characters + start + count).removeCharacters(softHyphen));
			bestNext = start + count;
		}
		else
		{
			lines.push_back((String(characters + start, characters + bestEnd).removeCharacters(softHyphen) + (bestHyphen ? "-" : "")).trimCharactersAtEnd(" "));
		}
		int next = bestNext;

		while ((next < end) && ((' ' == characters[next]) || (SOFT_HYPHEN == characters[next])))
		{
			next++;
		}

		if (end == next)
		{
			// The standard draws spaces left between the wrap and a forced line end as an empty line
			if (forcedLineEnd && String(characters + bestNext, characters + end).containsChar(' '))
			{
				lines.push_back(String());
			}
			break;
		}
		start = next;
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

	std::unordered_map<juce_wchar, float> characterWidths;
	GlyphArrangement glyphs;
	for (const auto &line : lines)
	{
		const std::vector<float> offsets = character_offsets(line, font, characterWidths);
		const float lineWidth = offsets.back();
		float x = 0.0f;

		if (isobus::TextualVTObject::HorizontalJustification::PositionMiddle == horizontalJustification)
		{
			x = (static_cast<float>(width) - lineWidth) / 2.0f;
		}
		else if (isobus::TextualVTObject::HorizontalJustification::PositionRight == horizontalJustification)
		{
			x = static_cast<float>(width) - lineWidth;
		}

		if (((y + lineHeight) > 0.0f) && (y < static_cast<float>(height)))
		{
			glyphs.addLineOfText(font, line, x, y + font.getAscent());
		}
		y += lineHeight;
	}
	glyphs.draw(g);
}
