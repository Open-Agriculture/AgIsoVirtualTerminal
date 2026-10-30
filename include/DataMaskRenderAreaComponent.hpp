//================================================================================================
/// @file DataMaskRenderAreaComponent.hpp
///
/// @brief A component to hold all the data mask render components.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================

#ifndef DATA_MASK_RENDER_AREA_COMPONENT_HPP
#define DATA_MASK_RENDER_AREA_COMPONENT_HPP

#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "NumericKeypadComponent.hpp"

#include "JuceHeader.h"

#include <mutex>

class ServerMainComponent;

class DataMaskRenderAreaComponent : public Component
{
public:
	DataMaskRenderAreaComponent(ServerMainComponent &parentServer);

	void on_change_active_mask(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet);

	void on_working_set_disconnect(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet);

	std::uint16_t abort_input(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet);

	void close_aborted_input_dialogs();

	void paint(Graphics &g) override;

	// Used to calculate button press events
	void mouseDown(const MouseEvent &event) override;

	void mouseDrag(const MouseEvent &event) override;

	// Used to calculate button release events
	void mouseUp(const MouseEvent &event) override;

	bool needsRepaint() const;

	void set_has_started(bool started);

private:
	/// @brief Holds the object being edited and the value the keypad produced for it
	class InputNumberListener
	{
	public:
		InputNumberListener() = default;

		std::uint32_t get_last_value() const;
		void set_last_value(std::uint32_t value);

		std::shared_ptr<isobus::InputNumber> get_target();
		void set_target(std::shared_ptr<isobus::InputNumber> objectBeingModified);

	private:
		std::shared_ptr<isobus::InputNumber> targetObject;
		std::uint32_t lastValue = 0;
	};

	int to_pool_coordinate(float renderedCoordinate) const;

	void close_input_dialogs();
	void begin_open_input(std::uint16_t objectID);
	bool end_open_input();

	std::shared_ptr<isobus::VTObject> getClickedChildRecursive(std::shared_ptr<isobus::VTObject> object, int x, int y);
	static bool objectCanBeClicked(std::shared_ptr<isobus::VTObject> object);
	static bool isClickWithinBounds(int clickXRelative, int clickYRelative, int objectX, int objectY, int objectWidth, int objectHeight);

	std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> parentWorkingSet;
	std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> openInputWorkingSet; ///< Cleared when the VT closes the dialog, so its callback sends nothing
	std::uint16_t openInputObjectID = isobus::NULL_OBJECT_ID;
	std::mutex openInputMutex; ///< abort_input() runs on the CAN thread
	std::unique_ptr<AlertWindow> inputListModal;
	std::unique_ptr<AlertWindow> inputNumberModal;
	std::unique_ptr<AlertWindow> inputStringModal;
	std::unique_ptr<NumericKeypadComponent> inputNumberKeypad;
	std::vector<std::shared_ptr<Component>> childComponents;
	std::vector<std::shared_ptr<Component>> currentModalComponentCache;
	ServerMainComponent &ownerServer;
	InputNumberListener inputNumberListener;
	std::shared_ptr<isobus::WorkingSet> measuredWorkingSetObject;
	int medianMaskExtent = 0;
	int designedMaskSize = 0;
	std::uint16_t activeMaskID = isobus::NULL_OBJECT_ID;
	std::uint16_t pressedButtonID = isobus::NULL_OBJECT_ID;
	bool needToRepaintActiveArea = false;
	bool hasStarted = false;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DataMaskRenderAreaComponent)
};

#endif // DATA_MASK_RENDER_AREA_COMPONENT_HPP
