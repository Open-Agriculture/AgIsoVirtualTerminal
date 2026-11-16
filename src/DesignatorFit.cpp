/*******************************************************************************
** @file       DesignatorFit.cpp
** @author     Sujan Dumaru
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "DesignatorFit.hpp"
#include "ContainerComponent.hpp"
#include "ObjectPointerComponent.hpp"

#include <vector>

// A container or an object pointer only groups other objects, so its visible contents are measured
// instead of its declared extent, clipped to it the way the renderer draws them.
static juce::Rectangle<int> drawn_bounds(juce::Component &component)
{
	auto bounds = component.getBounds();

	if ((nullptr != dynamic_cast<ContainerComponent *>(&component)) ||
	    (nullptr != dynamic_cast<ObjectPointerComponent *>(&component)))
	{
		juce::Rectangle<int> contents;

		for (auto *child : component.getChildren())
		{
			if (child->isVisible())
			{
				contents = contents.getUnion(drawn_bounds(*child));
			}
		}
		contents = contents.getIntersection(component.getLocalBounds());
		bounds = contents.isEmpty() ? contents : contents + component.getPosition();
	}
	return bounds;
}

// Clients author designators against whatever button size they assume, so the artwork is scaled to fit
// the button rather than clipped to it.
void fit_designator_to_button(juce::Component &designator, juce::Rectangle<int> button, juce::RectanglePlacement placement)
{
	auto children = designator.getChildren();
	std::vector<juce::Rectangle<int>> bounds;
	juce::Rectangle<int> total;
	juce::Rectangle<int> drawnArea;
	std::size_t measured = 0;

	for (auto *child : children)
	{
		bounds.push_back(drawn_bounds(*child));
		total = total.getUnion(bounds.back());
		measured += bounds.back().isEmpty() ? 0 : 1;
	}

	// Artwork is authored from the designator's own origin, so a child starting before it that spans the
	// union of them all is a slice of a backdrop shared across the mask, and is measured as the button.
	for (const auto &childBounds : bounds)
	{
		const bool backdrop = (1 < measured) && (childBounds == total) &&
		  ((0 > childBounds.getX()) || (0 > childBounds.getY()));

		drawnArea = drawnArea.getUnion(backdrop ? button.withZeroOrigin() : childBounds);
	}

	// Artwork that cannot be measured or placed, or that already fits where a placement may not enlarge
	// it, keeps the bounds it was built with.
	const bool keepAsAuthored = drawnArea.isEmpty() || button.isEmpty() ||
	  (placement.testFlags(juce::RectanglePlacement::onlyReduceInSize) && button.contains(drawnArea));

	if (!keepAsAuthored)
	{
		for (auto *child : children)
		{
			child->setTopLeftPosition(child->getPosition() - drawnArea.getPosition());
		}
		designator.setSize(drawnArea.getWidth(), drawnArea.getHeight());
		designator.setTransform(placement.getTransformToFit(designator.getBounds().toFloat(), button.toFloat()));
	}
}
