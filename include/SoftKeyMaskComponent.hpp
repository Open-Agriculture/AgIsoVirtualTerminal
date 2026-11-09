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

	/**
	 * @brief total_width
	 * @return The total width of the softkeymask (including inner and outer column paddings)
	 */
	int total_width() const;

	/**
   * @brief total_height
   * @return The total height of the softkeymask (including inner and outer column paddings)
   */
	int total_height() const;

	/**
	 * @brief key_count
	 * @return the number of the possible key positions in the softkey mask
	 */
	int key_count() const;

	/**
	 * @brief key_position
	 * @param slot The key position, counted down each column from the right
	 * @return The top left corner of that key position
	 */
	juce::Point<int> key_position(int slot) const;

	int keyHeight = 60;
	int keyWidth = 60;
	int rowCount = 6;
	int columnCount = 2;
	int height = 480;
	static constexpr std::uint8_t PADDING = 10;
};

class SoftKeyMaskComponent : public isobus::SoftKeyMask
  , public Component
{
public:
	SoftKeyMaskComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::SoftKeyMask sourceObject, SoftKeyMaskDimensions dimensions, std::uint16_t firstKeyIndex = 0);

	/// @brief How many pages of key positions a soft key mask needs. Trailing children that are
	/// empty object pointers take no position
	/// @param[in] softKeyMask The soft key mask
	/// @param[in] workingSet The working set that owns it
	/// @param[in] dimensions The soft key layout of the VT
	/// @returns The number of pages, 0 if there are no key positions
	static int get_page_count(std::shared_ptr<isobus::VTObject> softKeyMask, std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, const SoftKeyMaskDimensions &dimensions);

	void on_content_changed(bool initial = false);

	void paint(Graphics &g) override;

private:
	std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> parentWorkingSet;
	std::vector<std::shared_ptr<Component>> childComponents;
	SoftKeyMaskDimensions dimensionInfo;
	std::uint16_t firstKey;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoftKeyMaskComponent)
};

#endif // SOFT_KEY_MASK_COMPONENT_HPP
