/*******************************************************************************
** @file       NumberComponent.cpp
** @author     Miklos Marton
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "NumberComponent.hpp"

#include <iomanip>
#include <sstream>

NumberComponent::NumberComponent(
  std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet) :
  TextDrawingComponent(workingSet)
{
}

void NumberComponent::paintNumber(Graphics &g, bool enabled)
{
	bool strikeThrough = false;
	auto vtColour = parentWorkingSet->get_colour(vtObject()->get_background_color());
	juce::Colour backgroundColour = Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f);
	juce::Colour drawColour = getLookAndFeel().findColour(ListBox::textColourId);

	g.setColour(getLookAndFeel().findColour(ListBox::textColourId));

	auto sourceNumber = static_cast<const isobus::NumberVTObject *>(vtObject());
	auto fontAttrID = sourceNumber->get_font_attributes();

	if (isobus::NULL_OBJECT_ID != sourceNumber->get_font_attributes())
	{
		auto child = parentWorkingSet->get_object_by_id(fontAttrID);

		if ((nullptr != child) && (isobus::VirtualTerminalObjectType::FontAttributes == child->get_object_type()))
		{
			auto font = std::static_pointer_cast<isobus::FontAttributes>(child);
			if (font->get_style(isobus::FontAttributes::FontStyleBits::FlashingHidden) && !show)
			{
				return;
			}
			auto colour = parentWorkingSet->get_colour(font->get_colour());
			drawColour = Colour::fromFloatRGBA(colour.r, colour.g, colour.b, 1.0f);
			strikeThrough = font->get_style(isobus::FontAttributes::FontStyleBits::CrossedOut);
			prepare_text_painting(g, font, '1', drawColour, backgroundColour);
		}
	}

	double scaledValue = (static_cast<double>(sourceNumber->get_value()) + sourceNumber->get_offset()) * sourceNumber->get_scale();
	if (isobus::NULL_OBJECT_ID != sourceNumber->get_variable_reference())
	{
		auto child = sourceNumber->get_object_by_id(sourceNumber->get_variable_reference(), parentWorkingSet->get_object_tree());

		if ((nullptr != child) &&
		    (isobus::VirtualTerminalObjectType::NumberVariable == child->get_object_type()))
		{
			scaledValue = (static_cast<double>(std::static_pointer_cast<isobus::NumberVariable>(child)->get_value()) + sourceNumber->get_offset()) * sourceNumber->get_scale();
		}
	}

	if (!sourceNumber->get_option(isobus::NumberVTObject::Options::Transparent))
	{
		g.fillAll(backgroundColour);
	}
	g.setColour(drawColour);

	if ((0.0 != scaledValue) || !sourceNumber->get_option(isobus::NumberVTObject::Options::DisplayZeroAsBlank))
	{
		std::ostringstream valueText;
		valueText << std::fixed << std::setprecision(sourceNumber->get_number_of_decimals()) << scaledValue;
		// Not drawText, which drops glyphs that don't fit; ISO 11783-6 4.6.19.2 clips overflow by pixel
		GlyphArrangement glyphs;
		glyphs.addLineOfText(g.getCurrentFont(), valueText.str(), 0.0f, 0.0f);
		glyphs.justifyGlyphs(0, glyphs.getNumGlyphs(), 0.0f, 0.0f, static_cast<float>(sourceNumber->get_width()), static_cast<float>(sourceNumber->get_height()), convert_justification(sourceNumber->get_horizontal_justification(), sourceNumber->get_vertical_justification()));
		glyphs.draw(g);

		if (strikeThrough)
		{
			drawStrikeThrough(g, sourceNumber->get_width(), sourceNumber->get_height(), valueText.str(), sourceNumber->get_horizontal_justification());
		}
	}

	if (!enabled)
	{
		g.fillAll(Colour::fromFloatRGBA(0.5f, 0.5f, 0.5f, 0.5f));
	}
}
