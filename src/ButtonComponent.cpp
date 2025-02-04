/*******************************************************************************
** @file       ButtonComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "ButtonComponent.hpp"
#include "JuceManagedWorkingSetCache.hpp"

ButtonComponent::ButtonComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::Button sourceObject) :
  isobus::Button(sourceObject),
  juce::Button(""),
  parentWorkingSet(workingSet)
{
	setOpaque(false);
	setSize(get_width(), get_height());

	auto borderOffset = get_option(Options::NoBorder) ? 0 : 4;

	for (std::uint16_t i = 0; i < this->get_number_children(); i++)
	{
		auto child = get_object_by_id(get_child_id(i), parentWorkingSet->get_object_tree());

		if (nullptr != child)
		{
			childComponents.push_back(JuceManagedWorkingSetCache::create_component(parentWorkingSet, child));

			if (nullptr != childComponents.back())
			{
				addAndMakeVisible(*childComponents.back());
				childComponents.back()->setTopLeftPosition(get_child_x(i) + borderOffset, get_child_y(i) + borderOffset);
			}
		}
	}
}

void ButtonComponent::paint(Graphics &g)
{
	auto vtColour = parentWorkingSet->get_colour(backgroundColor);

	if (true == get_option(Options::TransparentBackground))
	{
		g.fillAll(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 0.0f));
	}
	else
	{
		g.fillAll(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f));
	}

	if (false == get_option(Options::NoBorder) && false == get_option(Options::SuppressBorder))
	{
		vtColour = parentWorkingSet->get_colour(get_border_colour());
		g.setColour(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f));
		g.drawRect(0, 0, get_width(), get_height(), 4);
	}
}

void ButtonComponent::paintButton(Graphics &, bool, bool)
{
}

void ButtonComponent::set_options(uint8_t value)
{
	// adjust the position of the childs to the button face area if the NoBorder attribute is changed
	if ((value & static_cast<uint8_t>(Options::NoBorder)) != get_option(Options::NoBorder))
	{
		auto borderOffset = (0 != (value & static_cast<uint8_t>(Options::NoBorder))) ? -4 : 4;
		int i = 0;
		for (auto &child : childComponents)
		{
			child->setTopLeftPosition(get_child_x(i) + borderOffset, get_child_y(i) + borderOffset);
			i++;
		}
	}
	return isobus::Button::set_options(value);
}
