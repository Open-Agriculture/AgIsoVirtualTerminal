/*******************************************************************************
** @file       WorkingSetSelectorComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "WorkingSetSelectorComponent.hpp"
#include "JuceManagedWorkingSetCache.hpp"
#include "ServerMainComponent.hpp"

WorkingSetSelectorComponent::WorkingSetSelectorComponent(ServerMainComponent &server) :
  parentServer(server)
{
	setOpaque(false);
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
		childObject->setTopLeftPosition(4 + 15 + workingSetObject->get_child_x(i), (static_cast<int>(children.size()) - 1) * 80 + 10 + 7 + workingSetObject->get_child_y(i));
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

	int numberOfSquares = 0;

	for (auto ws = children.begin(); ws != children.end(); ws++)
	{
		g.setColour(getLookAndFeel().findColour(ResizableWindow::backgroundColourId).brighter());
		g.drawRoundedRectangle(8.0f, 8.0f + (numberOfSquares * 80), 80, 80, 6, 1);
		numberOfSquares++;
	}
}

void WorkingSetSelectorComponent::resized()
{
	auto parentBounds = getLocalBounds();
	setBounds(0, 0, 100, parentBounds.getHeight());
}

void WorkingSetSelectorComponent::redraw()
{
	int workingSetIndex = 0;
	for (auto &workingSet : children)
	{
		workingSet.childComponents.clear();
		auto workingSetObject = workingSet.workingSet->get_working_set_object();

		for (std::uint16_t i = 0; i < workingSetObject->get_number_children(); i++)
		{
			auto childObject = JuceManagedWorkingSetCache::create_component(workingSet.workingSet, workingSetObject->get_object_by_id(workingSetObject->get_child_id(i)));
			workingSet.childComponents.push_back(childObject);
			childObject->setTopLeftPosition(4 + 15 + workingSetObject->get_child_x(i), workingSetIndex * 80 + 10 + 7 + workingSetObject->get_child_y(i));
			addAndMakeVisible(*childObject);
		}
		workingSetIndex++;
	}
	repaint();
}

void WorkingSetSelectorComponent::mouseUp(const MouseEvent &event)
{
	auto relativeEvent = event.getEventRelativeTo(this);

	if ((relativeEvent.getMouseDownX() >= 19) && (relativeEvent.getMouseDownX() < 99) && (relativeEvent.getMouseDownY() > 17) && (relativeEvent.getMouseDownY() < 17 + children.size() * 80))
	{
		int workingSetIndex = (relativeEvent.getMouseDownY() - 17) / 80;

		if (workingSetIndex <= 255)
		{
			parentServer.change_selected_working_set(static_cast<std::uint8_t>(workingSetIndex));
		}
	}
}
