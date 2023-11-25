/*******************************************************************************
** @file       OutputLineComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "OutputLineComponent.hpp"

OutputLineComponent::OutputLineComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::OutputLine sourceObject) :
  isobus::OutputLine(sourceObject),
  parentWorkingSet(workingSet)
{
	setSize(get_width(), get_height());
}

void OutputLineComponent::paint(Graphics &g)
{
	if (isobus::NULL_OBJECT_ID != get_line_attributes())
	{
		auto child = get_object_by_id(get_line_attributes(), parentWorkingSet->get_object_tree());

		if ((nullptr != child) && (isobus::VirtualTerminalObjectType::LineAttributes == child->get_object_type()))
		{
			if ((0 != get_width()) && (0 != get_height()))
			{
				auto line = std::static_pointer_cast<isobus::LineAttributes>(child);

				auto vtColour = parentWorkingSet->get_colour(line->get_background_color());
				g.setColour(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f));

				if (1 == get_height())
				{
					g.drawHorizontalLine(0, 0, get_width());
				}
				else if (1 == get_width())
				{
					g.drawVerticalLine(0, 0, get_height());
				}
				else if (LineDirection::BottomLeftToTopRight == get_line_direction())
				{
					g.drawLine(0, get_height(), get_width(), 0, line->get_width() + 0.5f);
				}
				else // LineDirection::TopLeftToBottomRight
				{
					g.drawLine(0, 0, get_width(), get_height(), line->get_width() + 0.5f);
				}
			}
		}
	}
}
