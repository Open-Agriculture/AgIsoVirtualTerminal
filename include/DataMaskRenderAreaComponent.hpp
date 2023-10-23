//================================================================================================
/// @file DataMaskRenderArea.hpp
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

#include "JuceHeader.h"

class ServerMainComponent;

class DataMaskRenderAreaComponent : public Component
{
public:
	DataMaskRenderAreaComponent(ServerMainComponent &parentServer);

	void on_change_active_mask(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet);

	void paint(Graphics &g) override;

	// Used to calculate button press events
	void mouseDown(const MouseEvent &event) override;

	// Used to calculate button release events
	void mouseUp(const MouseEvent &event) override;

	bool needsRepaint() const;

private:
	class InputNumberListener : public Slider::Listener
	{
	public:
		InputNumberListener() = default;
		void sliderValueChanged(Slider *slider) override;

		std::uint32_t get_last_value() const;
		void set_last_value(std::uint32_t value);

		std::shared_ptr<isobus::InputNumber> get_target();
		void set_target(std::shared_ptr<isobus::InputNumber> objectBeingModified);

	private:
		std::shared_ptr<isobus::InputNumber> targetObject;
		std::uint32_t lastValue = 0;
	};

	std::shared_ptr<isobus::VTObject> getClickedChildRecursive(std::shared_ptr<isobus::VTObject> object, int x, int y);
	static bool objectCanBeClicked(std::shared_ptr<isobus::VTObject> object);
	static bool isClickWithinBounds(int clickXRelative, int clickYRelative, int objectX, int objectY, int objectWidth, int objectHeight);

	std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> parentWorkingSet;
	std::unique_ptr<AlertWindow> inputListModal;
	std::unique_ptr<AlertWindow> inputNumberModal;
	std::unique_ptr<Slider> inputNumberSlider;
	std::vector<std::shared_ptr<Component>> childComponents;
	std::vector<std::shared_ptr<Component>> currentModalComponentCache;
	ServerMainComponent &ownerServer;
	InputNumberListener inputNumberListener;
	bool needToRepaintActiveArea = false;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DataMaskRenderAreaComponent);
};

#endif // DATA_MASK_RENDER_AREA_COMPONENT_HPP
