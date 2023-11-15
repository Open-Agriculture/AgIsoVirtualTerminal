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
	for (std::uint16_t i = 0; i < get_number_children(); i++)
	{
		auto child = get_object_by_id(get_child_id(i));

		if ((nullptr != child) && (isobus::VirtualTerminalObjectType::LineAttributes == child->get_object_type()))
		{
			if ((0 != get_width()) && (0 != get_height()))
			{
				auto line = std::static_pointer_cast<isobus::LineAttributes>(child);

				auto vtColour = colourTable.get_colour(line->get_background_color());
				g.setColour(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f));

				if (LineDirection::BottomLeftToTopRight == get_line_direction())
				{
					g.drawLine(0, get_height(), get_width(), 0, line->get_width() + 0.5f);
				}
				else // LineDirection::TopLeftToBottomRight
				{
					g.drawLine(0, 0, get_width(), get_height(), line->get_width() + 0.5f);
				}
			}
			break;
		}
	}
}
