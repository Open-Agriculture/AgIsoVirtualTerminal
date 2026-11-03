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
					auto childLineAttributes = get_object_by_id(get_line_attributes(), parentWorkingSet->get_object_tree());

					if ((nullptr != childLineAttributes) && (isobus::VirtualTerminalObjectType::LineAttributes == childLineAttributes->get_object_type()))
					{
						auto line = std::static_pointer_cast<isobus::LineAttributes>(childLineAttributes);
						vtColour = parentWorkingSet->get_colour(line->get_background_color());
						g.setColour(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0));
						g.fillAll(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f));
						break;
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
				const int lineWidth = line->get_width();
				const int width = static_cast<int>(get_width());
				const int height = static_cast<int>(get_height());
				const auto isSuppressed = [this](LineSuppressionOption side) {
					return 0 != ((0x01 << static_cast<std::uint8_t>(side)) & get_line_suppression_bitfield());
				};
				vtColour = parentWorkingSet->get_colour(line->get_background_color());
				g.setColour(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0));

				// ISO 11783-6 draws shapes with a square paintbrush of the line width that stays inside the object's
				// box, so each side is a strip of the line width along the inside of its edge
				if (!isSuppressed(LineSuppressionOption::SuppressTopLine))
				{
					g.fillRect(0, 0, width, lineWidth);
				}
				if (!isSuppressed(LineSuppressionOption::SuppressBottomLine))
				{
					g.fillRect(0, height - lineWidth, width, lineWidth);
				}
				if (!isSuppressed(LineSuppressionOption::SuppressLeftSideLine))
				{
					g.fillRect(0, 0, lineWidth, height);
				}
				if (!isSuppressed(LineSuppressionOption::SuppressRightSideLine))
				{
					g.fillRect(width - lineWidth, 0, lineWidth, height);
				}
			}
		}
	}
}
