//================================================================================================
/// @file TouchInputListMenu.hpp
///
/// @brief Defines a touch friendly drop down used to select an input list item.
///
/// @details The list opens next to the input list field and applies an item with a single tap.
/// Items keep the content the working set drew for them, which can be graphics rather than text,
/// scaled like the field. Long lists get large scroll arrows and a wide scroll bar instead of the
/// small hover zones of a standard popup menu, and can be dragged with a finger.
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#ifndef TOUCH_INPUT_LIST_MENU_HPP
#define TOUCH_INPUT_LIST_MENU_HPP

#include "JuceHeader.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

/// @brief One row of the drop down, wrapping the component the working set's list item renders as
class TouchInputListItem : public PopupMenu::CustomComponent
{
public:
	/// @brief Constructor
	/// @param[in] itemContent The rendered list item
	/// @param[in] width The width of the row in pixels
	/// @param[in] height The height of the row in pixels, raised to the minimum touch height
	/// @param[in] contentScale The scale the field is drawn at, so the item matches it
	TouchInputListItem(std::shared_ptr<Component> itemContent, int width, int height, float contentScale);

	void getIdealSize(int &idealWidth, int &idealHeight) override;

	void resized() override;

	void paint(Graphics &graphics) override;

	void paintOverChildren(Graphics &graphics) override;

	void mouseDown(const MouseEvent &event) override;

	void mouseDrag(const MouseEvent &event) override;

	void mouseUp(const MouseEvent &event) override;

	std::unique_ptr<AccessibilityHandler> createAccessibilityHandler() override;

	std::function<void()> onTap; ///< Called when the row is tapped, but not at the end of a drag

	static constexpr int MINIMUM_HEIGHT = 56; ///< Rows are at least this tall so a finger can hit them

private:
	std::shared_ptr<Component> content; ///< The rendered list item
	int rowWidth; ///< The ideal width of the row
	int rowHeight; ///< The ideal height of the row
	float scale; ///< The scale of the field the list belongs to
	bool dragged = false; ///< Set when the finger moved, so releasing it does not select the row

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TouchInputListItem)
};

/// @brief A drop down for input lists, built from rendered list items
class TouchInputListMenu
{
public:
	/// @brief Adds a row
	/// @param[in] itemID The ID passed to the callback when this row is chosen. Use the list index + 1,
	/// so NULL items, which get no row, cannot shift the selection.
	/// @param[in] row The row to show
	/// @param[in] title The text accessibility tools announce for the row
	void add_item(int itemID, std::unique_ptr<TouchInputListItem> row, const String &title);

	/// @brief Returns how many rows have been added
	/// @returns The number of rows
	int get_number_of_items() const;

	/// @brief Shows the drop down next to an input list field
	/// @param[in] fieldScreenBounds Where the field is on the screen
	/// @param[in] owner The component the drop down belongs to
	/// @param[in] selectedItemID The ID of the row to highlight and scroll to, or 0 for none
	/// @param[in] callback Called with the chosen row's ID, or 0 if the drop down was dismissed
	void show(Rectangle<int> fieldScreenBounds, Component *owner, int selectedItemID, std::function<void(int)> callback);

	static constexpr int SCROLL_COLUMN_WIDTH = 72; ///< Width of the scroll arrows and bar shown for long lists

private:
	/// @brief A row and the ID it reports
	struct Entry
	{
		int itemID;
		std::unique_ptr<TouchInputListItem> component;
	};

	class Panel;

	std::vector<Entry> entries; ///< The rows, in list order
};

#endif // TOUCH_INPUT_LIST_MENU_HPP
