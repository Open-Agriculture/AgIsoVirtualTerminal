//================================================================================================
/// @file SoftKeyMaskComponent.hpp
///
/// @brief Defines a GUI component to draw a soft key mask.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef SOFT_KEY_MASK_COMPONENT_HPP
#define SOFT_KEY_MASK_COMPONENT_HPP

#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "JuceHeader.h"

class SoftKeyMaskDimensions
{
public:
	SoftKeyMaskDimensions() = default;

	int keyHeight = 60;
	int keyWidth = 60;
	int keyCount() const;
	int rowCount = 6;
	int columnCount = 1;
	int height = 480;
	static const std::uint8_t padding = 10;

	/**
	 * @brief totalWidth
	 * @return The total width of the softkeymask (including inner and outer column paddings)
	 */
	int totalWidth() const;
};

class SoftKeyMaskComponent : public isobus::SoftKeyMask
  , public Component
{
public:
	SoftKeyMaskComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::SoftKeyMask sourceObject, SoftKeyMaskDimensions dimensions);

	void on_content_changed(bool initial = false);

	void paint(Graphics &g) override;

private:
	std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> parentWorkingSet;
	std::vector<std::shared_ptr<Component>> childComponents;
	SoftKeyMaskDimensions dimensionInfo;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoftKeyMaskComponent)
};

#endif // SOFT_KEY_MASK_COMPONENT_HPP
