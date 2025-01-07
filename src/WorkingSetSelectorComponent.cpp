/*******************************************************************************
** @file       WorkingSetSelectorComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "WorkingSetSelectorComponent.hpp"
#include "JuceManagedWorkingSetCache.hpp"
#include "ServerMainComponent.hpp"
#include "isobus/utility/system_timing.hpp"

WorkingSetSelectorComponent::WorkingSetSelectorComponent(ServerMainComponent &server) :
  parentServer(server)
{
	setOpaque(false);
	setBounds(0, 0, 100, 600);
}

void WorkingSetSelectorComponent::update_drawn_working_sets(std::vector<std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet>> &managedWorkingSetList)
{
	children.clear();

	for (std::size_t i = 0; i < managedWorkingSetList.size(); i++)
	{
		children.push_back({ managedWorkingSetList.at(i) });

		if ((isobus::VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState::Joined == managedWorkingSetList.at(i)->get_object_pool_processing_state()) &&
		    (!isobus::SystemTiming::time_expired_ms(managedWorkingSetList.at(i)->get_working_set_maintenance_message_timestamp_ms(), 3000)) &&
		    (!managedWorkingSetList.at(i)->is_deletion_requested()))
		{
			auto workingSetObject = managedWorkingSetList.at(i)->get_working_set_object();

			if (nullptr != workingSetObject)
			{
				auto workingSetComponent = JuceManagedWorkingSetCache::create_component(managedWorkingSetList.at(i), workingSetObject);
				children.back().childComponents.push_back(workingSetComponent);
				workingSetComponent->setTopLeftPosition(4 + 15, (static_cast<int>(i)) * 80 + 10 + 7);
				addAndMakeVisible(*workingSetComponent);
			}
		}
	}

	repaint();
}

void WorkingSetSelectorComponent::paint(Graphics &g)
{
	g.setColour(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
	g.fillAll();

	// Not sure if this is helpful...
	/*int numberOfSquares = 0;

	for (auto ws = children.begin(); ws != children.end(); ws++)
	{
		g.setColour(getLookAndFeel().findColour(ResizableWindow::backgroundColourId).brighter());
		g.drawRoundedRectangle(8.0f, 8.0f + (numberOfSquares * 80), 80, 80, 6, 1);
		numberOfSquares++;
	}*/
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

		if (nullptr != workingSetObject)
		{
			auto workingSetComponent = JuceManagedWorkingSetCache::create_component(workingSet.workingSet, workingSetObject);
			workingSet.childComponents.push_back(workingSetComponent);
			workingSetComponent->setTopLeftPosition(4 + 15, (static_cast<int>(workingSetIndex)) * 80 + 10 + 7);
			addAndMakeVisible(*workingSetComponent);
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
