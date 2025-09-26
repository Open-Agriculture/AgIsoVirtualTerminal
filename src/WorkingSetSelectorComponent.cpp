/*******************************************************************************
** @file       WorkingSetSelectorComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "WorkingSetSelectorComponent.hpp"
#include "JuceManagedWorkingSetCache.hpp"
#include "ServerMainComponent.hpp"
#include "WorkingSetLoadingIndicatorComponent.hpp"
#include "isobus/utility/system_timing.hpp"

WorkingSetSelectorComponent::WorkingSetSelectorComponent(ServerMainComponent &server) :
  parentServer(server)
{
	setOpaque(false);
	setBounds(0, 0, WIDTH, server.minimumHeight());
}

void WorkingSetSelectorComponent::update_drawn_working_sets(std::vector<std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet>> &managedWorkingSetList)
{
	children.clear();

	for (std::size_t i = 0; i < managedWorkingSetList.size(); i++)
	{
		children.push_back({ managedWorkingSetList.at(i) });

		if ((
		      (isobus::VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState::Joined == managedWorkingSetList.at(i)->get_object_pool_processing_state()) ||
		      managedWorkingSetList.at(i)->is_object_pool_transfer_in_progress()) &&
		    (!isobus::SystemTiming::time_expired_ms(managedWorkingSetList.at(i)->get_working_set_maintenance_message_timestamp_ms(), 3000)) &&
		    (!managedWorkingSetList.at(i)->is_deletion_requested()))
		{
			children.back().childComponents.push_back(getWorkingSetChildComponent(managedWorkingSetList.at(i), i));
		}
	}

	repaint();
}

void WorkingSetSelectorComponent::paint(Graphics &g)
{
	g.setColour(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
	g.fillAll();

	// draw rounded rectangle around the active working set selector
	int numberOfSquares = 0;
	for (auto ws = children.begin(); ws != children.end(); ws++)
	{
		if (ws->workingSet->get_control_function() && parentServer.get_active_working_set() && parentServer.get_active_working_set()->get_control_function())
		{
			if (ws->workingSet->get_control_function()->get_NAME().get_full_name() == parentServer.get_active_working_set()->get_control_function()->get_NAME().get_full_name())
			{
				g.setColour(juce::Colours::yellow.withAlpha(0.4f));
				g.drawRoundedRectangle(buttonPadding() - 2, buttonPadding() + (numberOfSquares * (BUTTON_HEIGHT + buttonPadding())) - 2, BUTTON_WIDTH + 4, BUTTON_HEIGHT + 4, 4, 4);
			}
		}
		numberOfSquares++;
	}
}

void WorkingSetSelectorComponent::resized()
{
	setBounds(0, 0, WIDTH, parentServer.minimumHeight());
}

void WorkingSetSelectorComponent::redraw()
{
	int workingSetIndex = 0;
	for (auto &workingSet : children)
	{
		workingSet.childComponents.clear();
		workingSet.childComponents.push_back(getWorkingSetChildComponent(workingSet.workingSet, workingSetIndex));
		workingSetIndex++;
	}
	repaint();
}

void WorkingSetSelectorComponent::updateIopLoadIndicators()
{
	for (auto &child : children)
	{
		if (child.workingSet->is_object_pool_transfer_in_progress() &&
		    child.workingSet->get_object_pool_processing_state() == isobus::VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState::None)
		{
			for (auto &subChild : child.childComponents)
			{
				subChild->repaint();
			}
		}
	}
}

int WorkingSetSelectorComponent::buttonPadding()
{
	return (WIDTH - BUTTON_WIDTH) / 2.0;
}

std::shared_ptr<Component> WorkingSetSelectorComponent::getWorkingSetChildComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, int workingSetIndex)
{
	auto workingSetObject = workingSet->get_working_set_object();
	std::shared_ptr<Component> workingSetComponent;
	if (nullptr != workingSetObject)
	{
		workingSetComponent = JuceManagedWorkingSetCache::create_component(workingSet, workingSetObject);
	}
	else
	{
		workingSetComponent = std::make_shared<WorkingSetLoadingIndicatorComponent>(workingSet, BUTTON_WIDTH, BUTTON_HEIGHT);
	}
	workingSetComponent->setTopLeftPosition(buttonPadding(), buttonPadding() + workingSetIndex * (BUTTON_HEIGHT + buttonPadding()));
	addAndMakeVisible(*workingSetComponent);
	return workingSetComponent;
}

void WorkingSetSelectorComponent::mouseUp(const MouseEvent &event)
{
	auto relativeEvent = event.getEventRelativeTo(this);

	if ((buttonPadding() <= relativeEvent.getMouseDownX()) && (relativeEvent.getMouseDownX() < buttonPadding() + BUTTON_WIDTH) && (buttonPadding() <= relativeEvent.getMouseDownY()) && (relativeEvent.getMouseDownY() < buttonPadding() + (buttonPadding() + BUTTON_HEIGHT) * children.size()))
	{
		int workingSetIndex = (relativeEvent.getMouseDownY() - buttonPadding()) / (BUTTON_HEIGHT + buttonPadding());

		if (workingSetIndex <= 255)
		{
			parentServer.change_selected_working_set(static_cast<std::uint8_t>(workingSetIndex));
		}
		redraw();
	}
}
