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

	/// @brief Shows the AUX button while a working set has auxiliary functions or inputs, or its screen is shown
	void update_auxiliary_button();

	/// @brief Marks the AUX button until the operator opens its screen
	/// @param[in] hasNotice True if an assignment changed without the operator
	void set_auxiliary_notice(bool hasNotice);

	static constexpr int WIDTH = 96;
	static constexpr int BUTTON_WIDTH = 72;
	static constexpr int BUTTON_HEIGHT = 72;
	static constexpr int button_padding();

private:
	/// @brief A button of the VT's own in the column, such as ACK and AUX
	class ColumnButton : public juce::TextButton
	{
	public:
		explicit ColumnButton(const juce::String &label);

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
	ColumnButton ackButton;
	ColumnButton auxButton;
	bool ackButtonPressed = false;
	bool hasAuxiliaryNotice = false;

	std::shared_ptr<Component> getWorkingSetChildComponent(
	  std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet,
	  int workingSetIndex);
	static juce::Rectangle<int> button_bounds(int index);
	void update_column_button_bounds();

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WorkingSetSelectorComponent)
};

#endif // WORKING_SET_SELECTOR_COMPONENT_HPP
