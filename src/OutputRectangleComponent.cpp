/*******************************************************************************
** @file       OutputRectangleComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "OutputRectangleComponent.hpp"

OutputRectangleComponent::OutputRectangleComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::OutputRectangle sourceObject) :
  isobus::OutputRectangle(sourceObject),
  parentWorkingSet(workingSet)
{
	setSize(get_width(), get_height());
}

void OutputRectangleComponent::paint(Graphics &g)
{
	auto vtColour = parentWorkingSet->get_colour(backgroundColor);
	bool isOpaque = false;

	if (isobus::NULL_OBJECT_ID != get_fill_attributes())
	{
		auto child = get_object_by_id(get_fill_attributes(), parentWorkingSet->get_object_tree());

		if ((nullptr != child) && (isobus::VirtualTerminalObjectType::FillAttributes == child->get_object_type()))
		{
			auto fill = std::static_pointer_cast<isobus::FillAttributes>(child);

			vtColour = parentWorkingSet->get_colour(fill->get_background_color());
			switch (std::static_pointer_cast<isobus::FillAttributes>(child)->get_type())
			{
				case isobus::FillAttributes::FillType::FillWithPatternGivenByFillPatternAttribute:
				{
					// @todo
					isOpaque = true;
				}
				break;

				case isobus::FillAttributes::FillType::FillWithLineColor:
				{
					for (std::uint16_t j = 0; j < get_number_children(); j++)
					{
						auto childLineAttributes = get_object_by_id(get_child_id(j), parentWorkingSet->get_object_tree());

						if ((nullptr != childLineAttributes) && (isobus::VirtualTerminalObjectType::LineAttributes == childLineAttributes->get_object_type()))
						{
							auto line = std::static_pointer_cast<isobus::LineAttributes>(childLineAttributes);
							vtColour = parentWorkingSet->get_colour(line->get_background_color());
							g.setColour(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0));
							g.fillAll(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f));
							break;
						}
					}
					isOpaque = true;
				}
				break;

				case isobus::FillAttributes::FillType::FillWithSpecifiedColorInFillColorAttribute:
				{
					g.fillAll(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f));
					isOpaque = true;
				}
				break;

				case isobus::FillAttributes::FillType::NoFill:
				default:
				{
					// No fill
					isOpaque = false;
				}
				break;
			}
		}
	}

	setOpaque(isOpaque);

	if (isobus::NULL_OBJECT_ID != get_line_attributes())
	{
		auto child = get_object_by_id(get_line_attributes(), parentWorkingSet->get_object_tree());

		if ((nullptr != child) && (isobus::VirtualTerminalObjectType::LineAttributes == child->get_object_type()))
		{
			auto line = std::static_pointer_cast<isobus::LineAttributes>(child);

			if (0 != line->get_width())
			{
				bool anyLineSuppressed = (0 != get_line_suppression_bitfield());
				vtColour = parentWorkingSet->get_colour(line->get_background_color());
				g.setColour(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0));

				if (!anyLineSuppressed)
				{
					g.drawRect(0, 0, static_cast<int>(get_width()), static_cast<int>(get_height()), line->get_width());
				}
				else // Something is suppressed
				{
					if (0 == ((0x01 << static_cast<std::uint8_t>(LineSuppressionOption::SuppressTopLine)) & get_line_suppression_bitfield()))
					{
						g.drawLine(0, 0, get_width(), 0, line->get_width());
					}
					if (0 == ((0x01 << static_cast<std::uint8_t>(LineSuppressionOption::SuppressLeftSideLine)) & get_line_suppression_bitfield()))
					{
						g.drawLine(0, 0, 0, get_height(), line->get_width());
					}
					if (0 == ((0x01 << static_cast<std::uint8_t>(LineSuppressionOption::SuppressRightSideLine)) & get_line_suppression_bitfield()))
					{
						g.drawLine(get_width(), 0, get_width(), get_height(), line->get_width());
					}
					if (0 == ((0x01 << static_cast<std::uint8_t>(LineSuppressionOption::SuppressBottomLine)) & get_line_suppression_bitfield()))
					{
						g.drawLine(0, get_height(), get_width(), get_height(), line->get_width());
					}
				}
			}
		}
	}
}
