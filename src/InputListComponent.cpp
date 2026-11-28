/*******************************************************************************
** @file       InputListComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "InputListComponent.hpp"
#include "JuceManagedWorkingSetCache.hpp"

#include <algorithm>

InputListComponent::InputListComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::InputList sourceObject) :
  isobus::InputList(sourceObject),
  parentWorkingSet(workingSet)
{
	setSize(get_width(), get_height());
	setOpaque(false);
	onChanged(true);
}

void InputListComponent::paint(Graphics &)
{
}

void InputListComponent::paintOverChildren(Graphics &g)
{
	if (!get_option(Options::Enabled))
	{
		g.fillAll(Colour::fromFloatRGBA(0.5f, 0.5f, 0.5f, 0.5f));
	}
	else if (get_number_children() > 0)
	{
		// ISO 11783-6 leaves the look of an input list to the VT. Without a marker an enabled list looks like the
		// plain picture or text of its selected item, so the operator cannot tell that it can be opened. Show up and
		// down arrows at its right edge and a thin outline, as other VTs do. The arrows sit on a light patch and the
		// outline has a dark and a light line, so both stay visible on any background colour.
		const float width = static_cast<float>(getWidth());
		const float height = static_cast<float>(getHeight());
		const float column = std::min(16.0f, width / 4.0f);
		const auto dark = Colour(0xff303030);
		const auto light = Colours::white;

		if ((column >= 6.0f) && (height >= 12.0f))
		{
			const float centreX = width - (column / 2.0f) - 2.0f;
			const float centreY = height / 2.0f;
			const float half = std::min(5.0f, column / 2.0f - 1.0f);
			const float tall = std::min(8.0f, centreY - 4.0f);

			g.setColour(light.withAlpha(0.75f));
			g.fillRoundedRectangle(width - column - 3.0f, 2.0f, column + 1.0f, height - 4.0f, 2.0f);

			Path arrows;
			arrows.addTriangle(centreX - half, centreY - 2.0f, centreX + half, centreY - 2.0f, centreX, centreY - 2.0f - tall);
			arrows.addTriangle(centreX - half, centreY + 2.0f, centreX + half, centreY + 2.0f, centreX, centreY + 2.0f + tall);
			g.setColour(dark);
			g.fillPath(arrows);
		}

		g.setColour(dark.withAlpha(0.7f));
		g.drawRoundedRectangle(0.75f, 0.75f, width - 1.5f, height - 1.5f, 3.0f, 1.5f);
		g.setColour(light.withAlpha(0.5f));
		g.drawRoundedRectangle(2.0f, 2.0f, width - 4.0f, height - 4.0f, 2.0f, 1.0f);
	}
}

void InputListComponent::onChanged(bool initial)
{
	childComponent.reset();

	std::uint32_t selectedIndex = get_value();

	if (isobus::NULL_OBJECT_ID != get_variable_reference())
	{
		auto child = get_object_by_id(get_variable_reference(), parentWorkingSet->get_object_tree());

		if (nullptr != child)
		{
			if (isobus::VirtualTerminalObjectType::NumberVariable == child->get_object_type())
			{
				selectedIndex = std::static_pointer_cast<isobus::NumberVariable>(child)->get_value();
			}
		}
	}

	if ((get_number_children() > 0) &&
	    (selectedIndex < static_cast<std::uint32_t>(get_number_children())))
	{
		// The number variable will always be the first one
		auto listItem = get_object_by_id(get_child_id(static_cast<std::uint16_t>(selectedIndex)), parentWorkingSet->get_object_tree());
		childComponent = JuceManagedWorkingSetCache::create_component(parentWorkingSet, listItem);

		if (nullptr != childComponent)
		{
			addAndMakeVisible(*childComponent);
			childComponent->setTopLeftPosition(0, 0);
		}
	}

	if (!initial)
	{
		repaint();
	}
}
