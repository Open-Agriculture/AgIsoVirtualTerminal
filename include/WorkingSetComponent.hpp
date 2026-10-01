//================================================================================================
/// @file WorkingSetComponent.hpp
///
/// @brief Defines a GUI component to draw a working set designator.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef WORKING_SET_COMPONENT_HPP
#define WORKING_SET_COMPONENT_HPP

#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "JuceHeader.h"

class WorkingSetComponent : public isobus::WorkingSet
  , public Component
{
public:
	WorkingSetComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::WorkingSet sourceObject, int keyHeight, int keyWidth);

	void paint(Graphics &g) override;

	/// @brief Scales a designator (or any stand-in for one) so that what it draws fits the selector button
	static void fit_designator_to_button(Component &designator, juce::Rectangle<int> button);

	/// @brief Draws the highlight that marks the active working set's selector button
	static void paint_active_highlight(Graphics &g, juce::Rectangle<int> button);

private:
	std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> parentWorkingSet;
	std::vector<std::shared_ptr<Component>> childComponents;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WorkingSetComponent)
};

#endif // WORKING_SET_COMPONENT_HPP
