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

	// Get font data
	if (isobus::NULL_OBJECT_ID != sourceNumber->get_font_attributes())
	{
		auto child = parentWorkingSet->get_object_by_id(fontAttrID);

		if ((nullptr != child) && (isobus::VirtualTerminalObjectType::FontAttributes == child->get_object_type()))
		{
			auto font = std::static_pointer_cast<isobus::FontAttributes>(child);
			// Bit 6 (Flashing) has priority over bit 5 (FlashingHidden)
			if (font->get_style(isobus::FontAttributes::FontStyleBits::FlashingHidden) && !show && !font->get_style(isobus::FontAttributes::FontStyleBits::Flashing))
			{
				return;
			}
			strikeThrough = font->get_style(isobus::FontAttributes::FontStyleBits::CrossedOut);
			prepare_text_painting(g, font, '1', drawColour, backgroundColour);
		}
	}

	float scaledValue = (sourceNumber->get_value() + sourceNumber->get_offset()) * sourceNumber->get_scale();
	if (isobus::NULL_OBJECT_ID != sourceNumber->get_variable_reference())
	{
		auto child = sourceNumber->get_object_by_id(sourceNumber->get_variable_reference(), parentWorkingSet->get_object_tree());

		if ((nullptr != child) &&
		    (isobus::VirtualTerminalObjectType::NumberVariable == child->get_object_type()))
		{
			scaledValue = (std::static_pointer_cast<isobus::NumberVariable>(child)->get_value() + sourceNumber->get_offset()) * sourceNumber->get_scale();
		}
	}

	if (sourceNumber->get_option(isobus::NumberVTObject::Options::DisplayZeroAsBlank) &&
	    scaledValue == 0.0)
	{
		// When this option bit is set, a blank field is displayed if and only if
		// the displayed value of the object is exactly zero.
		return;
	}

	if (!sourceNumber->get_option(isobus::NumberVTObject::Options::Transparent))
	{
		g.fillAll(backgroundColour);
	}
	g.setColour(drawColour);

	std::ostringstream valueText;
	valueText << std::fixed << std::setprecision(sourceNumber->get_number_of_decimals()) << scaledValue;
	g.drawText(valueText.str(), 0, 0, sourceNumber->get_width(), sourceNumber->get_height(), convert_justification(sourceNumber->get_horizontal_justification(), sourceNumber->get_vertical_justification()), false);

	if (strikeThrough)
	{
		drawStrikeThrough(g, sourceNumber->get_width(), sourceNumber->get_height(), valueText.str(), sourceNumber->get_horizontal_justification());
	}

	// If disabled, try and show that by drawing some semi-transparent grey
	if (!enabled)
	{
		g.fillAll(Colour::fromFloatRGBA(0.5f, 0.5f, 0.5f, 0.5f));
	}
}
