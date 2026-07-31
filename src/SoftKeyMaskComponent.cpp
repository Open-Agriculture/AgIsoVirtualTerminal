/*******************************************************************************
** @file       SoftKeyMaskComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "SoftKeyMaskComponent.hpp"
#include "JuceManagedWorkingSetCache.hpp"
#include "KeyComponent.hpp"
#include "ObjectPointerComponent.hpp"

SoftKeyMaskComponent::SoftKeyMaskComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::SoftKeyMask sourceObject, SoftKeyMaskDimensions dimensions) :
  isobus::SoftKeyMask(sourceObject),
  parentWorkingSet(workingSet),
  dimensionInfo(dimensions)
{
	setOpaque(true);
	setBounds(0, 0, dimensions.total_width(), dimensions.height);
	on_content_changed(true);
}

void SoftKeyMaskComponent::on_content_changed(bool initial)
{
	int row = 0;
	int x = dimensionInfo.PADDING + (dimensionInfo.columnCount - 1) * (dimensionInfo.PADDING + dimensionInfo.keyWidth);
	int y = dimensionInfo.PADDING;

	for (std::uint16_t i = 0; i < this->get_number_children(); i++)
	{
		auto child = get_object_by_id(get_child_id(i), parentWorkingSet->get_object_tree());

		if (nullptr != child)
		{
			childComponents.push_back(JuceManagedWorkingSetCache::create_component(parentWorkingSet, child));

			if (isobus::VirtualTerminalObjectType::ObjectPointer == child->get_object_type())
			{
				childComponents.back()->setSize(dimensionInfo.keyWidth, dimensionInfo.keyHeight);
				auto keyReference = get_object_by_id(std::static_pointer_cast<ObjectPointerComponent>(childComponents.back())->get_value(), parentWorkingSet->get_object_tree());
				if (nullptr != keyReference && isobus::VirtualTerminalObjectType::Key == keyReference->get_object_type())
				{
					std::static_pointer_cast<KeyComponent>(keyReference)->setKeyPosition(i);
				}
			}
			else if (isobus::VirtualTerminalObjectType::Key == child->get_object_type())
			{
				std::static_pointer_cast<KeyComponent>(childComponents.back())->setKeyPosition(i);
			}

			if (nullptr != childComponents.back())
			{
				addAndMakeVisible(*childComponents.back());
				childComponents.back()->setTopLeftPosition(x, y);
				y += (dimensionInfo.PADDING + dimensionInfo.keyWidth);

				row++;
				if (row >= dimensionInfo.rowCount)
				{
					row = 0;
					x -= (dimensionInfo.PADDING + dimensionInfo.keyWidth);
					y = dimensionInfo.PADDING;
				}
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

	g.fillAll(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f));
}

int SoftKeyMaskDimensions::key_count() const
{
	return columnCount * rowCount;
}

int SoftKeyMaskDimensions::total_width() const
{
	return PADDING + (columnCount * (keyWidth + PADDING));
}

int SoftKeyMaskDimensions::total_height() const
{
	return PADDING + (rowCount * (keyHeight + PADDING));
}
