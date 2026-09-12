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

#include <functional>
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

	/// @brief Colours the dot in the centre of the settings cogwheel to reflect CAN status,
	/// mirroring the text indicator ServerMainComponent currently draws in the menu bar.
	/// @param[in] interfaceRunning True if the CAN hardware interface has been started
	/// @param[in] adapterConnected True if the underlying CAN adapter is currently valid/connected
	void set_can_status(bool interfaceRunning, bool adapterConnected);

	/// @brief Sets the callback invoked when the settings cogwheel is pressed
	/// @param[in] callback The callback to invoke
	void set_on_settings_clicked(std::function<void()> callback);

	static constexpr int WIDTH = 96;
	static constexpr int BUTTON_WIDTH = 72;
	static constexpr int BUTTON_HEIGHT = 72;
	static constexpr int button_padding();

private:
	class AckButton : public juce::TextButton
	{
	public:
		AckButton();

		void paintButton(juce::Graphics &g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

	private:
		static constexpr float LABEL_WIDTH_RATIO = 0.8f;
	};

	/// @brief The button that opens the touch settings page (see docs/touch-settings-page.md).
	/// Unlike the ACK button this is always visible, and doubles as the CAN connection status
	/// indicator via a coloured dot in the centre of the gear - see set_can_status().
	class CogwheelButton : public juce::Button
	{
	public:
		CogwheelButton();

		void paintButton(juce::Graphics &g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

		void set_status_colour(juce::Colour colour);

	private:
		juce::Colour statusColour = juce::Colours::grey;
	};

	struct SELECTOR_CHILD_OBJECTS_STRUCT
	{
		std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet;
		std::vector<std::shared_ptr<Component>> childComponents;
	};
	std::vector<SELECTOR_CHILD_OBJECTS_STRUCT> children;
	ServerMainComponent &parentServer;
	AckButton ackButton;
	CogwheelButton settingsButton;
	bool ackButtonPressed = false;

	std::shared_ptr<Component> getWorkingSetChildComponent(
	  std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet,
	  int workingSetIndex);
	static juce::Rectangle<int> button_bounds(int index);

	/// @brief Positions the ACK button and the settings cogwheel. The cogwheel is anchored to the
	/// bottom of the column so it has a stable position regardless of whether the ACK button is
	/// currently shown; the ACK button, when visible, sits directly above it.
	void update_button_bounds();

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WorkingSetSelectorComponent)
};

#endif // WORKING_SET_SELECTOR_COMPONENT_HPP
