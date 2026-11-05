/*******************************************************************************
** @file       KeyComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "KeyComponent.hpp"

#include "DesignatorFit.hpp"
#include "JuceManagedWorkingSetCache.hpp"

KeyComponent::KeyComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::Key sourceObject, int keyWidth, int keyHeight) :
  isobus::Key(sourceObject),
  parentWorkingSet(workingSet)
{
	setSize(keyWidth, keyHeight);
	setOpaque(true);

	// The artwork is scaled inside a holder rather than on this component, so that paint() keeps
	// filling the whole designator and the letterboxing shows the key's colour.
	contentHolder.setSize(keyWidth, keyHeight);
	addAndMakeVisible(contentHolder);

	for (std::uint16_t i = 0; i < this->get_number_children(); i++)
	{
		auto child = get_object_by_id(get_child_id(i), parentWorkingSet->get_object_tree());

		if (nullptr != child)
		{
			childComponents.push_back(JuceManagedWorkingSetCache::create_component(parentWorkingSet, child));

			if (nullptr != childComponents.back())
			{
				contentHolder.addAndMakeVisible(*childComponents.back());
				childComponents.back()->setTopLeftPosition(get_child_x(i), get_child_y(i));
			}
		}
	}

	fit_designator_to_button(contentHolder, getLocalBounds(), juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize);
}

void KeyComponent::paint(Graphics &g)
{
	auto vtColour = parentWorkingSet->get_colour(backgroundColor);

	g.fillAll(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f));
}
