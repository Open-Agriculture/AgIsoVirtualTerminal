//================================================================================================
/// @file ObjectPointerComponent.hpp
///
/// @brief Defines a GUI component to draw an object pointer.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef OBJECT_POINTER_COMPONENT_HPP
#define OBJECT_POINTER_COMPONENT_HPP

#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "JuceHeader.h"

class ObjectPointerComponent : public isobus::ObjectPointer
  , public Component
{
public:
	ObjectPointerComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::ObjectPointer sourceObject);

	void on_content_changed(bool initial = false);

	void paint(Graphics &g) override;

private:
	std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> parentWorkingSet;
	std::shared_ptr<Component> childComponent;
	void getChildSizeRecursive(int &w, int &h) const;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ObjectPointerComponent)
};

#endif // OBJECT_POINTER_COMPONENT_HPP
