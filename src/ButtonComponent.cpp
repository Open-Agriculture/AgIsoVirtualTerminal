#include "ButtonComponent.hpp"
#include "JuceManagedWorkingSetCache.hpp"

ButtonComponent::ButtonComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::Button sourceObject) :
  isobus::Button(sourceObject),
  juce::Button(""),
  parentWorkingSet(workingSet)
{
	setOpaque(false);
	setSize(get_width(), get_height());

	for (std::uint16_t i = 0; i < this->get_number_children(); i++)
	{
		auto child = get_object_by_id(get_child_id(i));

		if (nullptr != child)
		{
			childComponents.push_back(JuceManagedWorkingSetCache::create_component(parentWorkingSet, child));

			if (nullptr != childComponents.back())
			{
				addAndMakeVisible(*childComponents.back());
				childComponents.back()->setTopLeftPosition(get_child_x(i), get_child_y(i));
			}
		}
	}
}

void ButtonComponent::paint(Graphics &g)
{
	auto vtColour = colourTable.get_colour(backgroundColor);

	if (true == get_option(Options::TransparentBackground))
	{
		g.fillAll(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 0.0f));
	}
	else
	{
		g.fillAll(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f));
	}

	if (false == get_option(Options::NoBorder))
	{
		vtColour = colourTable.get_colour(get_border_colour());
		g.drawRect(0, 0, get_width(), get_height(), 4);
	}
}

void ButtonComponent::paintButton(Graphics &g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
}
