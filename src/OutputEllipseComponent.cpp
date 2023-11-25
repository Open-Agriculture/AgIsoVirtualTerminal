/*******************************************************************************
** @file       OutputEllipseComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "OutputEllipseComponent.hpp"
#include "JuceManagedWorkingSetCache.hpp"

OutputEllipseComponent::OutputEllipseComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::OutputEllipse sourceObject) :
  isobus::OutputEllipse(sourceObject),
  parentWorkingSet(workingSet)
{
	setSize(get_width(), get_height());
}

void OutputEllipseComponent::paint(Graphics &g)
{
	// Ensure we fill first, then draw the outline if needed
	if (isobus::NULL_OBJECT_ID != get_fill_attributes())
	{
		auto child = get_object_by_id(get_fill_attributes(), parentWorkingSet->get_object_tree());

		if ((nullptr != child) && (isobus::VirtualTerminalObjectType::FillAttributes == child->get_object_type()))
		{
			auto fill = std::static_pointer_cast<isobus::FillAttributes>(child);

			auto vtColour = parentWorkingSet->get_colour(fill->get_background_color());
			g.setColour(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f));
			g.fillEllipse(0, 0, get_width(), get_height());
		}
	}

	if (isobus::NULL_OBJECT_ID != get_line_attributes())
	{
		auto child = get_object_by_id(get_line_attributes(), parentWorkingSet->get_object_tree());

		if ((nullptr != child) && (isobus::VirtualTerminalObjectType::LineAttributes == child->get_object_type()))
		{
			auto line = std::static_pointer_cast<isobus::LineAttributes>(child);

			auto vtColour = parentWorkingSet->get_colour(line->get_background_color());
			g.setColour(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f));
			g.drawEllipse(0, 0, get_width(), get_height(), line->get_width());
		}
	}
}
