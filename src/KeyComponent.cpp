/*******************************************************************************
** @file       KeyComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "KeyComponent.hpp"

#include "JuceManagedWorkingSetCache.hpp"

#include <algorithm>
#include <iterator>
#include <vector>

/// How far a designator's artwork may overhang the designator before it is scaled to fit. Pools
/// authored against a slightly different designator size overhang by a few percent and are meant
/// to be clipped.
constexpr float MAXIMUM_UNSCALED_OVERHANG = 1.1f;

/// What share of a pool's designators must measure the same to read that measurement as the size
/// the pool was authored for. A pool that merely overhangs scatters its measurements; a pool
/// authored for a larger designator repeats the same one.
constexpr float MINIMUM_DESIGNATOR_AGREEMENT = 0.5f;

constexpr std::uint8_t MAXIMUM_POINTER_DEPTH = 4;

/// @brief Resolves an object pointer to the object it ultimately points at
/// @details A designator's artwork is usually reached through an object pointer, and a pointer
/// carries no size of its own, so measuring the pointer reports the designator as empty.
/// @param[in] workingSet The working set to resolve against
/// @param[in] object The object to resolve
/// @returns The pointed-at object, or the object itself if it is not a pointer
static std::shared_ptr<isobus::VTObject> resolve_object_pointer(const std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet, std::shared_ptr<isobus::VTObject> object)
{
	// Bounded rather than recursive, because a malformed pool can point a chain back at itself.
	for (std::uint8_t depth = 0;
	     (nullptr != object) &&
	     (isobus::VirtualTerminalObjectType::ObjectPointer == object->get_object_type()) &&
	     (depth < MAXIMUM_POINTER_DEPTH);
	     depth++)
	{
		object = workingSet->get_object_by_id(std::static_pointer_cast<isobus::ObjectPointer>(object)->get_value());
	}
	return object;
}

/// @brief Finds the value that most of a set of measurements agree on
/// @param[in] values The measurements to look through
/// @param[in] minimumAgreement The share of the measurements that must share a value
/// @returns The agreed value, or 0 if too few of the measurements agree
static int find_agreed_value(std::vector<int> values, float minimumAgreement)
{
	int retVal = 0;
	std::size_t bestCount = 0;

	std::sort(values.begin(), values.end());

	for (auto runStart = values.begin(); runStart != values.end();)
	{
		auto runEnd = std::upper_bound(runStart, values.end(), *runStart);
		auto count = static_cast<std::size_t>(std::distance(runStart, runEnd));

		if (count > bestCount)
		{
			bestCount = count;
			retVal = *runStart;
		}
		runStart = runEnd;
	}

	if (static_cast<float>(bestCount) < (static_cast<float>(values.size()) * minimumAgreement))
	{
		retVal = 0;
	}
	return retVal;
}

/// @brief Estimates how much a pool's soft key designators need to be scaled to fit this VT's designators
/// @details A Key has no size of its own in ISO 11783-6 - the VT supplies it - so a pool authored
/// for larger designators just draws off the edge of ours and we clip it down to an unreadable
/// fragment. The whole pool is measured rather than this one Key, and the size the designators
/// agree on is taken rather than the largest or the middle one, because individual designators
/// legitimately park a shared background outside the designator and rely on the VT clipping it.
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
			auto child = resolve_object_pointer(workingSet, object->get_object_by_id(object->get_child_id(i), workingSet->get_object_tree()));

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
	auto agreedX = find_agreed_value(extentsX, MINIMUM_DESIGNATOR_AGREEMENT);
	auto agreedY = find_agreed_value(extentsY, MINIMUM_DESIGNATOR_AGREEMENT);

	if ((0 != agreedX) &&
	    (0 != agreedY) &&
	    ((agreedX > (keyWidth * MAXIMUM_UNSCALED_OVERHANG)) ||
	     (agreedY > (keyHeight * MAXIMUM_UNSCALED_OVERHANG))))
	{
		retVal = std::min(keyWidth / static_cast<float>(agreedX), keyHeight / static_cast<float>(agreedY));
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
