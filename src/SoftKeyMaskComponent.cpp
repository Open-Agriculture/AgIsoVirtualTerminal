/*******************************************************************************
** @file       SoftKeyMaskComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "SoftKeyMaskComponent.hpp"
#include "JuceManagedWorkingSetCache.hpp"

#include "SoftKeyMaskRenderAreaComponent.hpp"

SoftKeyMaskComponent::SoftKeyMaskComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::SoftKeyMask sourceObject, SoftKeyMaskDimensions dimensions, std::uint16_t firstKeyIndex) :
  isobus::SoftKeyMask(sourceObject),
  parentWorkingSet(workingSet),
  dimensionInfo(dimensions),
  firstKey(firstKeyIndex)
{
	setOpaque(true);
	setBounds(0, 0, dimensions.total_width(), dimensions.height);
	on_content_changed(true);
}

static int count_used_soft_keys(std::shared_ptr<isobus::VTObject> softKeyMask, std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet)
{
	const auto &objectTree = workingSet->get_object_tree();
	int usedKeyCount = softKeyMask->get_number_children();

	while (0 < usedKeyCount)
	{
		auto child = isobus::VTObject::get_object_by_id(softKeyMask->get_child_id(static_cast<std::uint16_t>(usedKeyCount - 1)), objectTree);

		// Capped, since pointers can form a cycle
		for (std::size_t hops = 0; (nullptr != child) && (isobus::VirtualTerminalObjectType::ObjectPointer == child->get_object_type()) && (hops < objectTree.size()); hops++)
		{
			child = isobus::VTObject::get_object_by_id(std::static_pointer_cast<isobus::ObjectPointer>(child)->get_value(), objectTree);
		}

		if ((nullptr != child) && (isobus::VirtualTerminalObjectType::ObjectPointer != child->get_object_type()))
		{
			break;
		}
		usedKeyCount--;
	}
	return usedKeyCount;
}

int SoftKeyMaskComponent::get_page_count(std::shared_ptr<isobus::VTObject> softKeyMask, std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, const SoftKeyMaskDimensions &dimensions)
{
	const int slots = dimensions.key_count();
	return (0 < slots) ? (count_used_soft_keys(softKeyMask, workingSet) + slots - 1) / slots : 0;
}

void SoftKeyMaskComponent::on_content_changed(bool initial)
{
	const int lastKey = firstKey + dimensionInfo.key_count();

	for (int i = firstKey; (i < get_number_children()) && (i < lastKey); i++)
	{
		const auto position = dimensionInfo.key_position(i - firstKey);
		auto child = get_object_by_id(get_child_id(static_cast<std::uint16_t>(i)), parentWorkingSet->get_object_tree());

		if (nullptr != child)
		{
			childComponents.push_back(JuceManagedWorkingSetCache::create_component(parentWorkingSet, child));

			if (isobus::VirtualTerminalObjectType::ObjectPointer == child->get_object_type())
			{
				childComponents.back()->setSize(dimensionInfo.keyWidth, dimensionInfo.keyHeight);
			}

			if (nullptr != childComponents.back())
			{
				addAndMakeVisible(*childComponents.back());
				childComponents.back()->setTopLeftPosition(position);
			}
		}
	}

	if (!initial)
	{
		repaint();
	}
}

void SoftKeyMaskComponent::paint(Graphics &g)
{
	auto vtColour = parentWorkingSet->get_colour(backgroundColor);

	auto background = Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f);
	g.fillAll(background);

	// Ring every slot, empty ones too, just outside the key where no child draws.
	g.setColour(background.contrasting());
	for (int slot = 0; slot < dimensionInfo.key_count(); slot++)
	{
		const auto position = dimensionInfo.key_position(slot);
		g.drawRect(position.x - 1, position.y - 1, dimensionInfo.keyWidth + 2, dimensionInfo.keyHeight + 2);
	}
}

int SoftKeyMaskDimensions::key_count() const
{
	return columnCount * rowCount;
}

juce::Point<int> SoftKeyMaskDimensions::key_position(int slot) const
{
	return { PADDING + (columnCount - 1 - (slot / rowCount)) * (PADDING + keyWidth),
		       PADDING + (slot % rowCount) * (PADDING + keyHeight) };
}

int SoftKeyMaskDimensions::total_width() const
{
	return PADDING + (columnCount * (keyWidth + PADDING));
}

int SoftKeyMaskDimensions::total_height() const
{
	return PADDING + (rowCount * (keyHeight + PADDING));
}
