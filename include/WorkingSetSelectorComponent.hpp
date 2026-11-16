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
	void paintOverChildren(Graphics &g) override;
	void resized() override;
	void mouseUp(const MouseEvent &event) override;

	void redraw();
	void update_iop_load_indicators();
	void set_ack_button_visible(bool shouldBeVisible);

	static constexpr int WIDTH = 96;
	static constexpr int BUTTON_WIDTH = 72;
	static constexpr int BUTTON_HEIGHT = 72;
	static constexpr int button_padding();

	/// @brief Gets the bounds of a working set button
	/// @param index The button's position in the column
	/// @returns The button's bounds
	static juce::Rectangle<int> button_bounds(int index);

	/// @brief Fills a button with its working set's background colour
	/// @param g The graphics context
	/// @param bounds The button's bounds
	/// @param workingSet The button's working set
	static void paint_button_background(Graphics &g, juce::Rectangle<int> bounds, const std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet);

	/// @brief Positions and scales a designator in its button
	/// @param designator The designator's component
	/// @param bounds The button's bounds
	static void place_designator(Component &designator, juce::Rectangle<int> bounds);

private:
	class AckButton : public juce::TextButton
	{
	public:
		AckButton();

		void paintButton(juce::Graphics &g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

	private:
		static constexpr float LABEL_WIDTH_RATIO = 0.8f;
	};

	struct SELECTOR_CHILD_OBJECTS_STRUCT
	{
		std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet;
		std::vector<std::shared_ptr<Component>> childComponents;
	};
	std::vector<SELECTOR_CHILD_OBJECTS_STRUCT> children;
	ServerMainComponent &parentServer;
	AckButton ackButton;
	bool ackButtonPressed = false;

	std::shared_ptr<Component> getWorkingSetChildComponent(
	  std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet,
	  int workingSetIndex);
	void update_ack_button_bounds();

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WorkingSetSelectorComponent)
};

#endif // WORKING_SET_SELECTOR_COMPONENT_HPP
