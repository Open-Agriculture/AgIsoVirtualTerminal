/*******************************************************************************
** @file       KeyComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "KeyComponent.hpp"

#include "JuceManagedWorkingSetCache.hpp"

#include <algorithm>
#include <vector>

/// How far a designator's artwork may overhang the designator before it is scaled to fit. Pools
/// authored against a slightly different designator size overhang by a few percent and are meant
/// to be clipped, so only a gross overhang is treated as evidence of a different designator size.
constexpr float MAXIMUM_UNSCALED_OVERHANG = 1.5f;

/// @brief Estimates how much a pool's soft key designators need to be scaled to fit this VT's designators
/// @details A Key has no size of its own in ISO 11783-6 - the VT supplies it - so a pool authored
/// for larger designators just draws off the edge of ours and we clip it down to an unreadable
/// fragment. The whole pool is measured rather than this one Key, and the median is taken rather
/// than the largest, because individual designators legitimately park a shared background outside
/// the designator and rely on the VT clipping it.
/// @param[in] workingSet The working set whose object pool should be measured
/// @param[in] keyWidth The width in pixels of a soft key designator on this VT
/// @param[in] keyHeight The height in pixels of a soft key designator on this VT
/// @returns The scale factor to render designator content at, or 1.0 to leave the pool alone
static float detect_designator_content_scale(const std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet, int keyWidth, int keyHeight)
{
	std::vector<int> extentsX, extentsY;

	for (const auto &currentObject : workingSet->get_object_tree())
	{
		auto object = currentObject.second;

		if ((nullptr == object) ||
		    (isobus::VirtualTerminalObjectType::Key != object->get_object_type()))
		{
			continue;
		}

		int extentX = 0;
		int extentY = 0;

		for (std::uint16_t i = 0; i < object->get_number_children(); i++)
		{
			auto child = object->get_object_by_id(object->get_child_id(i), workingSet->get_object_tree());

			if (nullptr == child)
			{
				continue;
			}
			extentX = std::max(extentX, object->get_child_x(i) + static_cast<int>(child->get_width()));
			extentY = std::max(extentY, object->get_child_y(i) + static_cast<int>(child->get_height()));
		}

		// Designators with nothing sizable in them carry no signal, and pools have plenty of those.
		if ((0 != extentX) && (0 != extentY))
		{
			extentsX.push_back(extentX);
			extentsY.push_back(extentY);
		}
	}

	auto retVal = 1.0f;

	if (!extentsX.empty())
	{
		auto medianX = extentsX.begin() + (extentsX.size() / 2);
		auto medianY = extentsY.begin() + (extentsY.size() / 2);

		std::nth_element(extentsX.begin(), medianX, extentsX.end());
		std::nth_element(extentsY.begin(), medianY, extentsY.end());

		if ((*medianX > (keyWidth * MAXIMUM_UNSCALED_OVERHANG)) ||
		    (*medianY > (keyHeight * MAXIMUM_UNSCALED_OVERHANG)))
		{
			retVal = std::min(keyWidth / static_cast<float>(*medianX), keyHeight / static_cast<float>(*medianY));
		}
	}
	return retVal;
}

KeyComponent::KeyComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::Key sourceObject, int keyWidth, int keyHeight) :
  isobus::Key(sourceObject),
  parentWorkingSet(workingSet)
{
	setSize(keyWidth, keyHeight);
	setOpaque(true);

	auto contentScale = detect_designator_content_scale(parentWorkingSet, keyWidth, keyHeight);

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
				childComponents.back()->setTransform(AffineTransform::scale(contentScale));
			}
		}
	}
}

void KeyComponent::paint(Graphics &g)
{
	auto vtColour = parentWorkingSet->get_colour(backgroundColor);

	g.fillAll(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f));
}
