/*******************************************************************************
** @file       WorkingSetComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "WorkingSetComponent.hpp"
#include "JuceManagedWorkingSetCache.hpp"

WorkingSetComponent::WorkingSetComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::WorkingSet sourceObject, int keyHeight, int keyWidth) :
  isobus::WorkingSet(sourceObject),
  parentWorkingSet(workingSet)
{
	setSize(keyWidth, keyHeight);
	setOpaque(false);

	for (std::uint16_t i = 0; i < this->get_number_children(); i++)
	{
		auto child = get_object_by_id(get_child_id(i), parentWorkingSet->get_object_tree());

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

void WorkingSetComponent::paint(Graphics &g)
{
	auto vtColour = parentWorkingSet->get_colour(get_background_color());
	auto background = Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f);
	g.setColour(background);
	g.fillAll();
}

// Clients author designators against whatever soft key size they assume, which across real pools runs from
// 28x26 up to 240x192, so the designator is scaled to fit the button rather than clipped to it. getUnion
// ignores empty rectangles, so children that resolved to nothing drop out of the measurement on their own.
void WorkingSetComponent::fit_designator_to_button(Component &designator, juce::Rectangle<int> button)
{
	auto children = designator.getChildren();
	juce::Rectangle<int> drawnArea;

	for (auto *child : children)
	{
		drawnArea = drawnArea.getUnion(child->getBounds());
	}

	if (drawnArea.isEmpty())
	{
		// nothing to measure, so leave the component at the button-sized bounds it was built with
		return;
	}

	for (auto *child : children)
	{
		child->setTopLeftPosition(child->getPosition() - drawnArea.getPosition());
	}
	designator.setSize(drawnArea.getWidth(), drawnArea.getHeight());
	designator.setTransform(juce::RectanglePlacement(juce::RectanglePlacement::centred)
	                          .getTransformToFit(designator.getBounds().toFloat(), button.toFloat()));
}

void WorkingSetComponent::paint_active_highlight(Graphics &g, juce::Rectangle<int> button)
{
	g.setColour(juce::Colours::yellow.withAlpha(0.4f));
	g.drawRoundedRectangle(button.toFloat().expanded(2.0f), 4.0f, 4.0f);
}
