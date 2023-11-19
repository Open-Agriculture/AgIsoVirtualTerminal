//================================================================================================
/// @file SoftKeyMaskRenderAreaComponent.hpp
///
/// @brief A component to hold all the data mask render components.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef SOFT_KEY_MASK_RENDER_AREA_COMPONENT_HPP
#define SOFT_KEY_MASK_RENDER_AREA_COMPONENT_HPP

#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "JuceHeader.h"

class ServerMainComponent;

class SoftKeyMaskRenderAreaComponent : public Component
{
public:
	SoftKeyMaskRenderAreaComponent(ServerMainComponent &parentServer);

	void on_change_active_mask(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet);

	void paint(Graphics &g) override;

	// Used to calculate button press events
	void mouseDown(const MouseEvent &event) override;

	// Used to calculate button release events
	void mouseUp(const MouseEvent &event) override;

private:
	std::shared_ptr<isobus::VTObject> getClickedChildRecursive(std::shared_ptr<isobus::VTObject> object, int x, int y);
	static bool objectCanBeClicked(std::shared_ptr<isobus::VTObject> object);
	static bool isClickWithinBounds(int clickXRelative, int clickYRelative, int objectX, int objectY, int objectWidth, int objectHeight);

	std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> parentWorkingSet;
	std::vector<std::shared_ptr<Component>> childComponents;
	ServerMainComponent &ownerServer;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoftKeyMaskRenderAreaComponent)
};

#endif // SOFT_KEY_MASK_RENDER_AREA_COMPONENT_HPP
