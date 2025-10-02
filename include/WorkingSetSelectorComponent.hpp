//================================================================================================
/// @file WorkingSetSelectorComponent.hpp
///
/// @brief Defines a GUI component allow selecting the active working set.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef WORKING_SET_SELECTOR_COMPONENT_HPP
#define WORKING_SET_SELECTOR_COMPONENT_HPP

#include "WorkingSetComponent.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "JuceHeader.h"

#include <vector>

class ServerMainComponent;

class WorkingSetSelectorComponent : public Component
{
public:
	explicit WorkingSetSelectorComponent(ServerMainComponent &server);

	void update_drawn_working_sets(std::vector<std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet>> &managedWorkingSetList);

	void paint(Graphics &g) override;
	void resized() override;
	void mouseUp(const MouseEvent &event) override;

	void redraw();
	void update_iop_load_indicators();

	static constexpr int WIDTH = 96;
	static constexpr int BUTTON_WIDTH = 72;
	static constexpr int BUTTON_HEIGHT = 72;
	static constexpr int button_padding();

private:
	struct SELECTOR_CHILD_OBJECTS_STRUCT
	{
		std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet;
		std::vector<std::shared_ptr<Component>> childComponents;
	};
	std::vector<SELECTOR_CHILD_OBJECTS_STRUCT> children;
	ServerMainComponent &parentServer;

	std::shared_ptr<Component> getWorkingSetChildComponent(
	  std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet,
	  int workingSetIndex);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WorkingSetSelectorComponent)
};

#endif // WORKING_SET_SELECTOR_COMPONENT_HPP
