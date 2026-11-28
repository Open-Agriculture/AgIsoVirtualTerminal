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

	if (true == get_option(Options::Disabled))
	{
		setAlpha(0.5f);
	}

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
}

void ButtonComponent::paintOverChildren(Graphics &g)
{
	if (false == get_option(Options::NoBorder) && false == get_option(Options::SuppressBorder))
	{
		auto vtColour = parentWorkingSet->get_colour(get_border_colour());
		const auto borderColour = Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f);
		g.setColour(borderColour);
		g.drawRect(0, 0, get_width(), get_height(), 4);

		// Buttons without a border show no pressed or latched state. While pressed or latched the border is drawn
		// sunken (dark top and left, light bottom and right), so the button visibly goes down under the finger.
		if ((true == pressed) || ((true == get_option(Options::Latchable)) && (true == get_option(Options::CurrentButtonStateIfLatchable))))
		{
			const float width = static_cast<float>(get_width());
			const float height = static_cast<float>(get_height());
			constexpr float BEVEL = 4.0f;

			Path topLeft;
			topLeft.startNewSubPath(0.0f, 0.0f);
			topLeft.lineTo(width, 0.0f);
			topLeft.lineTo(width - BEVEL, BEVEL);
			topLeft.lineTo(BEVEL, BEVEL);
			topLeft.lineTo(BEVEL, height - BEVEL);
			topLeft.lineTo(0.0f, height);
			topLeft.closeSubPath();

			Path bottomRight;
			bottomRight.startNewSubPath(width, 0.0f);
			bottomRight.lineTo(width, height);
			bottomRight.lineTo(0.0f, height);
			bottomRight.lineTo(BEVEL, height - BEVEL);
			bottomRight.lineTo(width - BEVEL, height - BEVEL);
			bottomRight.lineTo(width - BEVEL, BEVEL);
			bottomRight.closeSubPath();

			g.setColour(borderColour.interpolatedWith(Colours::black, 0.45f));
			g.fillPath(topLeft);
			g.setColour(borderColour.interpolatedWith(Colours::white, 0.55f));
			g.fillPath(bottomRight);
		}

		// ISO 11783-6 leaves the border's look to the VT. A pool may give the border the button's own colour,
		// so a thin dark outline shows that it is a button.
		g.setColour(borderColour.interpolatedWith(Colours::black, 0.65f));
		g.drawRoundedRectangle(0.75f, 0.75f, static_cast<float>(get_width()) - 1.5f, static_cast<float>(get_height()) - 1.5f, 3.0f, 1.5f);
	}

	// The last button the operator pressed gets a blue highlight from the moment it is pressed and keeps it,
	// so the operator can see which button the last touch went to
	if (selected)
	{
		g.setColour(Colour(0xff3d5cff).withAlpha(0.28f));
		g.fillRect(getLocalBounds());
		g.setColour(Colour(0xff3d5cff));
		g.drawRect(getLocalBounds(), 3);
	}
}

void ButtonComponent::paintButton(Graphics &, bool, bool)
{
}

void ButtonComponent::set_selected(bool isSelected)
{
	if (selected != isSelected)
	{
		selected = isSelected;
		repaint();
	}
}

void ButtonComponent::set_pressed(bool isPressed)
{
	pressed = isPressed;
	repaint();
}

void ButtonComponent::set_options(std::uint8_t value)
{
	// adjust the position of the childs to the button face area if the NoBorder attribute is changed
	if (((value & static_cast<std::uint8_t>(Options::NoBorder)) != 0) != get_option(Options::NoBorder))
	{
		auto borderOffset = (0 != (value & static_cast<std::uint8_t>(Options::NoBorder))) ? -4 : 4;
		int i = 0;
		for (auto &child : childComponents)
		{
			child->setTopLeftPosition(get_child_x(i) + borderOffset, get_child_y(i) + borderOffset);
			i++;
		}
	}
	return isobus::Button::set_options(value);
}
