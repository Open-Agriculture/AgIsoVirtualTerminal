/*******************************************************************************
** @file       DataMaskComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "DataMaskComponent.hpp"
#include "JuceManagedWorkingSetCache.hpp"

DataMaskComponent::DataMaskComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::DataMask sourceObject) :
  isobus::DataMask(sourceObject),
  parentWorkingSet(workingSet)
{
	setOpaque(true);
	setBounds(0, 0, 480, 480);
	on_content_changed(true);
}

void DataMaskComponent::on_content_changed(bool initial)
{
	for (std::uint16_t i = 0; i < this->get_number_children(); i++)
	{
		auto child = get_object_by_id(get_child_id(i), parentWorkingSet->get_object_tree());

		if ((nullptr != child) && (isobus::VirtualTerminalObjectType::SoftKeyMask != child->get_object_type()))
		{
			childComponents.push_back(JuceManagedWorkingSetCache::create_component(parentWorkingSet, child));

			if (nullptr != childComponents.back())
			{
				addAndMakeVisible(*childComponents.back());
				childComponents.back()->setTopLeftPosition(get_child_x(i), get_child_y(i));
			}
		}
	}

	if (!initial)
	{
		repaint();
	}
}

void DataMaskComponent::paint(Graphics &g)
{
	auto vtColour = parentWorkingSet->get_colour(backgroundColor);

	g.fillAll(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f));
}
