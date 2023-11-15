/*******************************************************************************
** @file       WorkingSetSelectorComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "WorkingSetSelectorComponent.hpp"
#include "JuceManagedWorkingSetCache.hpp"

WorkingSetSelectorComponent::WorkingSetSelectorComponent()
{
	setOpaque(true);
	setBounds(0, 0, 100, 600);
}

void WorkingSetSelectorComponent::add_working_set_to_draw(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet)
{
	children.push_back({ workingSet });

	auto workingSetObject = workingSet->get_working_set_object();

	for (std::uint16_t i = 0; i < workingSetObject->get_number_children(); i++)
	{
		auto childObject = JuceManagedWorkingSetCache::create_component(workingSet, workingSetObject->get_object_by_id(workingSetObject->get_child_id(i)));
		children.back().childComponents.push_back(childObject);
		childObject->setTopLeftPosition(4 + 15, 10 + 7);
		addAndMakeVisible(*childObject);
	}
	repaint();
}

void WorkingSetSelectorComponent::remove_working_set(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet)
{
	for (auto child = children.begin(); child != children.end(); child++)
	{
		if (workingSet == child->workingSet)
		{
			children.erase(child);
			break;
		}
	}
	repaint();
}

void WorkingSetSelectorComponent::paint(Graphics &g)
{
	g.setColour(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
	g.fillAll();

	for (const auto &ws : children)
	{
		g.setColour(getLookAndFeel().findColour(ResizableWindow::backgroundColourId).brighter());
		g.drawRoundedRectangle(8.0, 8.0, 80, 80, 6, 1);
	}
}

void WorkingSetSelectorComponent::resized()
{
	auto parentBounds = getLocalBounds();
	setBounds(0, 0, 100, parentBounds.getHeight());
}

void WorkingSetSelectorComponent::redraw()
{
	for (auto& workingSet : children)
	{
		workingSet.childComponents.clear();
		auto workingSetObject = workingSet.workingSet->get_working_set_object();

		for (std::uint16_t i = 0; i < workingSetObject->get_number_children(); i++)
		{
			auto childObject = JuceManagedWorkingSetCache::create_component(workingSet.workingSet, workingSetObject->get_object_by_id(workingSetObject->get_child_id(i)));
			children.back().childComponents.push_back(childObject);
			childObject->setTopLeftPosition(4 + 15 + workingSetObject->get_child_x(i), 10 + 7 + workingSetObject->get_child_y(i));
			addAndMakeVisible(*childObject);
		}
	}
	repaint();
}
