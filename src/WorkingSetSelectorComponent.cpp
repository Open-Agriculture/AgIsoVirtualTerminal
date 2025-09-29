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
	setBounds(0, 0, 100, 600);
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
				g.drawRoundedRectangle(4 + 15 - 2, 10 + 7 - 2 + (numberOfSquares * 80), parentServer.get_soft_key_descriptor_x_pixel_width() + 4, parentServer.get_soft_key_descriptor_y_pixel_height() + 4, 4, 4);
			}
		}
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
		workingSetComponent = std::make_shared<WorkingSetLoadingIndicatorComponent>(
		  workingSet,
		  parentServer.get_soft_key_descriptor_x_pixel_width(),
		  parentServer.get_soft_key_descriptor_y_pixel_height());
	}
	workingSetComponent->setTopLeftPosition(4 + 15, workingSetIndex * 80 + 10 + 7);
	addAndMakeVisible(*workingSetComponent);
	return workingSetComponent;
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
		redraw();
	}
}
